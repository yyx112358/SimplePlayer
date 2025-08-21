#include "SPResultChain.h"
#include <mutex>
#include <condition_variable>
#include <vector>
#include <spdlog/spdlog.h>
#include <chrono>
using namespace std::chrono_literals;

#undef SPDLOG_DEBUG
#define SPDLOG_DEBUG SPDLOG_INFO

// 实现Impl结构体
struct SPResultChainImpl {
    SPResultChainImpl() = default;
    ~SPResultChainImpl()
    {
        std::lock_guard<std::mutex> lock(mtx);
        if (!finished) {
            result = SPResultChain::RESULT_CODE::OK;
            finished = true;
            cv.notify_all();
#ifdef DEBUG
            if (prev_chains.size()) {
                SPDLOG_DEBUG("SPResultChain destroyed before finish, auto-finishing with OK");
            }
#endif
        }
    }
    SPResultChainImpl(const SPResultChainImpl &) = delete;
    SPResultChainImpl& operator = (const SPResultChainImpl &) = delete;

    std::vector<SPResultChain> prev_chains;
    SPResultChain::RESULT_CODE result = SPResultChain::RESULT_CODE::FAIL;
    std::atomic_bool finished;
    std::mutex mtx; // 保护所有成员变量的访问
    std::condition_variable cv; // 用于等待任务完成
    
#ifdef DEBUG
    // 调试用工具
    SPResultChainImpl(const char* create_func, int create_line)
        : create_func(create_func), create_line(create_line)
    {
        SPDLOG_DEBUG("Start result chain [{0}:{1}]", create_func, create_line);
    }
    const char* const create_func = nullptr; // 创建位置函数名
    const int create_line = 0; // 创建位置行号
#endif
};

const char * to_string(SPResultChain::RESULT_CODE code) {
    switch (code) {
        case SPResultChain::RESULT_CODE::OK:
            return "OK";
        case SPResultChain::RESULT_CODE::FAIL:
            return "FAIL";
        case SPResultChain::RESULT_CODE::TIMEOUT:
            return "TIMEOUT";
        default:
            return "Unknown";
    }
}

SPResultChain::SPResultChain() : _impl(std::make_shared<SPResultChainImpl>()) {}

// 构造函数实现
#ifdef DEBUG
SPResultChain::SPResultChain(const char* create_func, int create_line)
    : _impl(std::make_shared<SPResultChainImpl>(create_func, create_line)) {}
#endif

// after方法实现
void SPResultChain::after(const SPResultChain& prev) {
    std::lock_guard<std::mutex> lock(_impl->mtx);
#ifdef DEBUG
    if (_impl->create_func) {
        SPDLOG_DEBUG("[{0}:{1}] Add dependency from {2}:{3}",
                     _impl->create_func, _impl->create_line,
                     prev._impl->create_func, prev._impl->create_line);
    }
#endif
    _impl->prev_chains.push_back(prev);
}

void SPResultChain::after(SPResultChain&& prev) {
    std::lock_guard<std::mutex> lock(_impl->mtx);
#ifdef DEBUG
    if (_impl->create_func) {
        SPDLOG_DEBUG("[{0}:{1}] Add dependency from {2}:{3}",
                     _impl->create_func, _impl->create_line,
                     prev._impl->create_func, prev._impl->create_line);
    }
#endif
    _impl->prev_chains.push_back(std::move(prev));
}

void SPResultChain::after(const std::vector<SPResultChain>& prev) {
    std::lock_guard<std::mutex> lock(_impl->mtx);
    for (const auto& p : prev) {
#ifdef DEBUG
        if (_impl->create_func) {
            SPDLOG_DEBUG("[{0}:{1}] Add dependency from {2}:{3}",
                         _impl->create_func, _impl->create_line,
                         p._impl->create_func, p._impl->create_line);
        }
#endif
        _impl->prev_chains.push_back(p);
    }
}

void SPResultChain::after(std::vector<SPResultChain>&& prev) {
    std::lock_guard<std::mutex> lock(_impl->mtx);
    for (auto& p : prev) {
#ifdef DEBUG
        if (_impl->create_func) {
            SPDLOG_DEBUG("[{0}:{1}] Add dependency from {2}:{3}",
                         _impl->create_func, _impl->create_line,
                         p._impl->create_func, p._impl->create_line);
        }
#endif
        _impl->prev_chains.push_back(std::move(p));
    }
}

// wait方法实现
SPResultChain::RESULT_CODE SPResultChain::wait(int64_t timeout) {
    std::unique_lock<std::mutex> lock(_impl->mtx);
    
#ifdef DEBUG
    if (_impl->create_func) {
        SPDLOG_DEBUG("[{0}:{1}] Start waiting, timeout={2}us",
                     _impl->create_func, _impl->create_line, timeout);
    }
#endif
    
    // 如果已经完成，直接返回结果
    if (_impl->finished) {
#ifdef DEBUG
        if (_impl->create_func) {
            SPDLOG_DEBUG("[{0}:{1}] Already finished with result {2}",
                         _impl->create_func, _impl->create_line, to_string(_impl->result));
        }
#endif
        return _impl->result;
    }
    
    // 计算超时时间点(仅对有限超时有效)
    auto timeout_time = std::chrono::steady_clock::now();
    if (timeout >= 0) {
        timeout_time += std::chrono::microseconds(timeout);
    }
    
    // 等待所有前序任务完成
    for (auto& prev : _impl->prev_chains) {
        auto prevImpl = prev._impl;
        
        // 计算剩余超时时间
        int64_t remaining_timeout = -1;
        if (timeout >= 0) {
            auto now = std::chrono::steady_clock::now();
            if (now >= timeout_time) {
                // 已超时
                _impl->result = RESULT_CODE::TIMEOUT;
                _impl->finished = true;
                _impl->cv.notify_all();
                return _impl->result;
            }
            remaining_timeout = std::max<int64_t>(std::chrono::duration_cast<std::chrono::microseconds>(timeout_time - now).count(), 0);
        }
        
        // 等待前序任务完成
        std::unique_lock<std::mutex> prevLock(prevImpl->mtx);
        std::cv_status status = std::cv_status::no_timeout;
        if (timeout > 0) {
//            status = prev._impl->cv.wait_until(prevLock, timeout_time);
            for (int i = 0; i < timeout / 1000; i++) {
                status = prevImpl->cv.wait_for(prevLock, 1ms, [prevImpl]() -> bool {return prevImpl->finished;}) ? std::cv_status::no_timeout : std::cv_status::timeout;
                if (status == std::cv_status::no_timeout)
                    break;
            }
        } else
            prev._impl->cv.wait(prevLock, [prevImpl]() -> bool {return prevImpl->finished;});
        
        if (status == std::cv_status::timeout) {
            _impl->result = RESULT_CODE::TIMEOUT;
            _impl->finished = true;
            _impl->cv.notify_all();
            return _impl->result;
        }
        if (prev._impl->result == RESULT_CODE::FAIL) {
            _impl->result = prev._impl->result;
            _impl->finished = true;
            _impl->cv.notify_all();
            return _impl->result;
        }
        if (prev._impl->result == RESULT_CODE::TIMEOUT) {
            _impl->result = prev._impl->result;
        }
    }
    
#ifdef DEBUG
    if (_impl->create_func) {
        SPDLOG_DEBUG("[{0}:{1}] All dependencies completed",
                     _impl->create_func, _impl->create_line);
    }
#endif
    return RESULT_CODE::OK;
}

// finish方法实现
void SPResultChain::finish(RESULT_CODE result) {
    std::lock_guard<std::mutex> lock(_impl->mtx);
    if (_impl->finished) {
#ifdef DEBUG
        if (_impl->create_func) {
            spdlog::warn("[{0}:{1}] Double finish call ignored",
                         _impl->create_func, _impl->create_line);
        }
#endif
        return; // 避免重复调用
    }
    _impl->result = result;
    _impl->finished = true;
#ifdef DEBUG
    if (_impl->create_func) {
        SPDLOG_DEBUG("[{0}:{1}] Finished with result {2}",
                     _impl->create_func, _impl->create_line, to_string(result));
    }
#endif
    _impl->cv.notify_all(); // 通知所有等待的线程
}

// retry方法实现
bool SPResultChain::retry(int index, SPResultChain chain) {
    std::lock_guard<std::mutex> lock(_impl->mtx);
    if (index < 0 || static_cast<size_t>(index) >= _impl->prev_chains.size()) {
        return false;
    }
    _impl->prev_chains[index] = std::move(chain);
    return true;
}

// getResult方法实现
SPResultChain::RESULT_CODE SPResultChain::getResult(int index) const {
    std::lock_guard<std::mutex> lock(_impl->mtx);
    if (index < 0 || static_cast<size_t>(index) >= _impl->prev_chains.size()) {
        return _impl->result;
    }
    return _impl->prev_chains[index].getResult(-1);
}

// getResults方法实现
std::vector<SPResultChain::RESULT_CODE> SPResultChain::getResults() const {
    std::lock_guard<std::mutex> lock(_impl->mtx);
    std::vector<RESULT_CODE> results;
    for (auto& prev : _impl->prev_chains) {
        auto prev_results = prev.getResults();
        results.insert(results.end(), prev_results.begin(), prev_results.end());
    }
    results.push_back(_impl->result);
    return results;
}

// getPrevChains方法实现
std::vector<SPResultChain> SPResultChain::getPrevChains() const {
    std::lock_guard<std::mutex> lock(_impl->mtx);
    return _impl->prev_chains;
}
