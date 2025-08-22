#pragma once
#include <memory>
#include <optional>

// 前向声明
struct SPResultChainImpl;

/**
 *  @brief SPResultChain类可以相互串联构成一个有向无环图，其中每一个节点都需要上游所有节点执行完毕后才能继续执行。
 *  使用pimpl模式封装内部状态，支持安全复制和多线程操作
 */
class SPResultChain {
public:
    enum class RESULT_CODE : int {
        OK, // 成功执行
        FAIL, // 失败
        TIMEOUT, // 超时
    };
    
    /// @brief 默认构造函数
    SPResultChain();

#ifdef DEBUG
    /**
     * @brief 调试模式构造函数
     * @param create_func 创建所在函数名
     * @param create_line 创建所在行号
     */
    SPResultChain(const char* create_func, int create_line);
#endif
    
    /// @brief 如果还没有调用finish就析构，将调用finish(RESULT_CODE::OK)，并在Debug模式下打印日志
    ~SPResultChain() = default;
    
    /// @brief 新增前序任务，允许在wait()结束之后再次新增前序任务
    void after(const SPResultChain& prev);
    void after(SPResultChain&& prev);
    void after(const std::vector<SPResultChain>& prev);
    void after(std::vector<SPResultChain>&& prev);
    
    /// 阻塞等待前序任务完成，并获得前序任务的返回值
    /// 如果前序任务都已完成了，将立刻返回
    /// @param timeout 最长等待timeout毫秒，如果超时将调用finish(TIMEOUT)。当timeout < 0时，将无限等待下去
    /// @return 所有前序任务和当前任务的执行结果，仅有全部执行成功时，返回OK，否则返回FAIL。如果本任务超时，返回TIMEOUT
    RESULT_CODE wait(int64_t timeout = -1);

    /// 阻塞等待其它线程将本任务设置为finish（本任务和其它任务均完成），并获得最终的返回值
    RESULT_CODE waitAll(int64_t timeout = -1);
    
    /// 标记当前任务以及所有的前序任务执行完毕
    /// 当一个任务所有前序任务都已经完成并调用finish，此任务将解除阻塞
    /// @param result 当前任务的执行结果，默认值为OK
    void finish(RESULT_CODE result = RESULT_CODE::OK);
    
    /// @brief 重试前序任务
    /// @param index 前序任务的索引
    /// @param chain 重试后的前序任务
    /// @return 如果索引合法，返回true，否则返回false
    bool retry(int index, SPResultChain chain);
    
    /// @brief 获取所有前序任务以及当前任务的执行结果
    /// @return 所有前序任务以及当前任务的执行结果
    std::optional<RESULT_CODE> getResult(int index) const;
    std::vector<std::optional<RESULT_CODE>> getResults() const;
    
    /// @brief 获取所有前序任务
    /// @return 所有前序任务
    std::vector<SPResultChain> getPrevChains() const;

private:
    std::shared_ptr<SPResultChainImpl> _impl; // 使用前向声明的Impl结构体
};

// 创建宏定义
#ifdef DEBUG
#define SP_RESULT_CHAIN() SPResultChain(__PRETTY_FUNCTION__, __LINE__)
extern void _test_SPResultChain();
#else
#define SP_RESULT_CHAIN() SPResultChain()
#endif
