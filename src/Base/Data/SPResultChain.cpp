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
        std::unique_lock<std::mutex> lock(mtx);
        if (!finished) {
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

    void resetResult() {
        finished = false;
    }
    
    std::optional<SPResultChain::RESULT_CODE> get()
    {
        if (!finished)
            return std::nullopt;
        return result;
    }
    
    void set(SPResultChain::RESULT_CODE code)
    {
        result = code;
        finished = true;
        cv.notify_all();
    }
    
public:
    std::vector<SPResultChain> prev_chains;
    
    std::mutex mtx;
    std::atomic_bool finished;
    std::condition_variable cv;
    SPResultChain::RESULT_CODE result;
    
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
    std::unique_lock<std::mutex> lock(_impl->mtx);
    _impl->resetResult();
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
    std::unique_lock<std::mutex> lock(_impl->mtx);
    _impl->resetResult();
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
    std::unique_lock<std::mutex> lock(_impl->mtx);
    _impl->resetResult();
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
    std::unique_lock<std::mutex> lock(_impl->mtx);
    _impl->resetResult();
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
    // 计算超时时间点(仅对有限超时有效)
    auto timeout_time = std::chrono::steady_clock::now();
    if (timeout >= 0) {
        timeout_time += std::chrono::milliseconds(timeout);
    }
    
#ifdef DEBUG
    if (_impl->create_func) {
        SPDLOG_DEBUG("[{0}:{1}] Start waiting, timeout={2}us",
                     _impl->create_func, _impl->create_line, timeout);
    }
#endif
    
    // 等待所有前序任务完成
    std::unique_lock<std::mutex> lock(_impl->mtx);
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
            remaining_timeout = std::max<int64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(timeout_time - now).count(), 0);
        }
        
        // 等待前序任务完成
        std::unique_lock<std::mutex> prevLock(prevImpl->mtx);
        if (prevImpl->finished)
            continue;
        
        bool status = true;
        if (timeout > 0) {
//            status = prev._impl->cv.wait_until(prevLock, timeout_time);
            for (int i = 0; i < timeout; i++) {
                status = prevImpl->cv.wait_for(prevLock, 1ms, [&finished = prevImpl->finished]() {return finished == true;});
                if (status == true)
                    break;
            }
        } else
            prevImpl->cv.wait(prevLock);
        
        // 仅有超时退出，否则也需要等待所有上游任务完成
        if (status == false) {
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
    return finalResult;
}

SPResultChain::RESULT_CODE SPResultChain::waitAll(int64_t timeout) {
    auto timeout_time = std::chrono::steady_clock::now();
    if (timeout >= 0) {
        timeout_time += std::chrono::milliseconds(timeout);
    }
    if (auto code = wait(timeout); code != SPResultChain::RESULT_CODE::OK)
        return code;
    // TODO: 这里有锁会释放一次，有风险，待后续抽出独立的方法
    std::unique_lock<std::mutex> lock(_impl->mtx);
    if (timeout >= 0)
        _impl->cv.wait_until(lock, timeout_time, [&finished = _impl->finished]() { return finished == true; });
    else
        _impl->cv.wait(lock, [&finished = _impl->finished]() { return finished == true; });
    return _impl->result;
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
    _impl->resetResult();
    _impl->prev_chains[index] = std::move(chain);
    return true;
}

// getResult方法实现
std::optional<SPResultChain::RESULT_CODE> SPResultChain::getResult(int index) const {
    std::lock_guard<std::mutex> lock(_impl->mtx);
    if (index < 0 || static_cast<size_t>(index) >= _impl->prev_chains.size()) {
        return _impl->get();
    }
    return _impl->prev_chains[index].getResult(-1);
}

// getResults方法实现
std::vector<std::optional<SPResultChain::RESULT_CODE>> SPResultChain::getResults() const {
    std::lock_guard<std::mutex> lock(_impl->mtx);
    std::vector<std::optional<SPResultChain::RESULT_CODE>> results;
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


#if DEBUG
#include <spdlog/spdlog.h>

#include "SPResultChain.h"
#include <thread>
#include <iostream>

using namespace std;



SPResultChain test1() {
    SPResultChain fc = SP_RESULT_CHAIN();
    
    thread t([fc]() mutable {
        for (int i = 0; i < 1000; i++) // 执行耗时任务
            this_thread::sleep_for(1ms);
        if (rand() % 2)
            fc.finish(); // 标记执行成功
        else
            fc.finish(SPResultChain::RESULT_CODE::FAIL); // 标记执行失败
    });
    t.detach();
    return fc;
}

SPResultChain test2() {
    SPResultChain fc = SP_RESULT_CHAIN();
    fc.after(test1());
    
    thread t([fc]() mutable {
        for (int i = 0; i < 500; i++) // 执行耗时任务
            this_thread::sleep_for(1ms);
        
        if (fc.wait(500) == SPResultChain::RESULT_CODE::FAIL
            && fc.getResult(0) == SPResultChain::RESULT_CODE::FAIL) {
            fc.retry(0, test1());
            fc.wait();
        }
            
        fc.finish();
    });
    t.detach();
    return fc;
}

SPResultChain test3() {
    SPResultChain fc = SP_RESULT_CHAIN();
    fc.after({test1(), test2()});
    
    thread t([fc]() mutable {
        for (int i = 0; i < 500; i++) // 执行耗时任务
            this_thread::sleep_for(1ms);
        
        fc.wait();
        fc.finish();
    });
    t.detach();
    return fc;
}


void _test_SPResultChain() {
    srand(static_cast<uint32_t>(time(nullptr)));
    
    SPResultChain fc = test3();
    
    cout << static_cast<int>(fc.waitAll()) << endl;
}
#endif // DEBUG
