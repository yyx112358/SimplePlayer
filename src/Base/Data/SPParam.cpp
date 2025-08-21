//
//  SPParam.cpp
//  SimplePlayer
//
//  Created by YangYixuan on 2023/10/27.
//

#include "SPParam.hpp"

#include <thread>
#include <mutex>
#include <condition_variable>
#include <memory>
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


void _my_main() {
    srand(time(nullptr));
    
    SPResultChain fc = test3();
    
    cout << static_cast<int>(fc.wait()) << endl;
}
