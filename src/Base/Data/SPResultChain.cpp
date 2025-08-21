#include "SPResultChain.h"
#include <future>
#include <vector>
#include <spdlog/spdlog.h>
#include <chrono>
using namespace std::chrono_literals;

#undef SPDLOG_DEBUG
#define SPDLOG_DEBUG SPDLOG_INFO

// 实现Impl结构体
struct SPResultChainImpl {
public:
    SPResultChainImpl() = default;
    ~SPResultChainImpl()
    {
        std::lock_guard<std::mutex> lock(mtx);
        if (!result) {
            set(SPResultChain::RESULT_CODE::OK);
#ifdef DEBUG
            if (prev_chains.size()) {
                SPDLOG_DEBUG("SPResultChain destroyed before finish, auto-finishing with OK");
            }
#endif
        }
    }
    SPResultChainImpl(const SPResultChainImpl &) = delete;
    SPResultChainImpl& operator = (const SPResultChainImpl &) = delete;

    SPResultChain::RESULT_CODE get()
    {
        if (!result)
            result = std::make_unique<std::promise<SPResultChain::RESULT_CODE>>();
        return result->get_future().get();
    }
    
    void set(SPResultChain::RESULT_CODE code)
    {
        if (!result)
            result = std::make_unique<std::promise<SPResultChain::RESULT_CODE>>();
        result->set_value(code);
    }
    
    std::future_status wait_for(int64_t timeout = -1) {
        if (!result)
            result = std::make_unique<std::promise<SPResultChain::RESULT_CODE>>();
        auto fu = result->get_future();
        std::future_status status = std::future_status::ready;
        if (timeout > 0) {
//            status = prev._impl->cv.wait_until(prevLock, timeout_time);
            for (int i = 0; i < timeout / 1000; i++) {
                status = fu.wait_for(1ms);
                if (status == std::future_status::timeout)
                    break;
            }
        } else
            fu.wait();
        return status;
    }
    
public:
    std::vector<SPResultChain> prev_chains;
    std::unique_ptr<std::promise<SPResultChain::RESULT_CODE>> result;
    std::mutex mtx;
    
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

const char * to_string(SPResultChain::RESULT_CODE code)
{
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
    _impl->result.reset();
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
    _impl->result.reset();
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
    _impl->result.reset();
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
    _impl->result.reset();
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
    
    // 计算超时时间点(仅对有限超时有效)
    auto timeout_time = std::chrono::steady_clock::now();
    if (timeout >= 0) {
        timeout_time += std::chrono::microseconds(timeout);
    }
    
    // 等待所有前序任务完成
    RESULT_CODE finalResult = RESULT_CODE::OK;
    for (auto& prev : _impl->prev_chains) {
        auto prevImpl = prev._impl;
        
        // 计算剩余超时时间
        int64_t remaining_timeout = -1;
        if (timeout >= 0) {
            auto now = std::chrono::steady_clock::now();
            if (now >= timeout_time) {
                // 已超时
                _impl->set(RESULT_CODE::TIMEOUT);
                return RESULT_CODE::TIMEOUT;
            }
            remaining_timeout = std::max<int64_t>(std::chrono::duration_cast<std::chrono::microseconds>(timeout_time - now).count(), 0);
        }
        
        // 等待前序任务完成
        std::future_status status = prevImpl->wait_for(timeout);
        
        // 仅有超时退出，否则也需要等待所有上游任务完成
        if (status == std::future_status::timeout) {
            _impl->set(RESULT_CODE::TIMEOUT);
            return RESULT_CODE::TIMEOUT;
        }
        if (prevImpl->get() != RESULT_CODE::OK) {
            finalResult = RESULT_CODE::FAIL;
        }
    }
    
#ifdef DEBUG
    if (_impl->create_func) {
        SPDLOG_DEBUG("[{0}:{1}] All dependencies completed",
                     _impl->create_func, _impl->create_line);
    }
#endif
//    _impl->set(finalResult);
    return finalResult;
}

// finish方法实现
void SPResultChain::finish(RESULT_CODE result) {
    std::lock_guard<std::mutex> lock(_impl->mtx);
    _impl->set(result);
#ifdef DEBUG
    if (_impl->create_func) {
        SPDLOG_DEBUG("[{0}:{1}] Finished with result {2}",
                     _impl->create_func, _impl->create_line, to_string(result));
    }
#endif
}

// retry方法实现
bool SPResultChain::retry(int index, SPResultChain chain) {
    std::lock_guard<std::mutex> lock(_impl->mtx);
    if (index < 0 || static_cast<size_t>(index) >= _impl->prev_chains.size()) {
        return false;
    }
    _impl->result.reset();
    _impl->prev_chains[index] = std::move(chain);
    _impl->result = std::make_unique<std::promise<SPResultChain::RESULT_CODE>>();
    return true;
}

// getResult方法实现
SPResultChain::RESULT_CODE SPResultChain::getResult(int index) const {
    std::lock_guard<std::mutex> lock(_impl->mtx);
    if (index < 0 || static_cast<size_t>(index) >= _impl->prev_chains.size()) {
        return _impl->get();
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
    results.push_back(_impl->get());
    return results;
}

// getPrevChains方法实现
std::vector<SPResultChain> SPResultChain::getPrevChains() const {
    std::lock_guard<std::mutex> lock(_impl->mtx);
    return _impl->prev_chains;
}
