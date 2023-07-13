//
//  SPLog.cpp
//  SimplePlayer
//
//  Created by YangYixuan on 2023/7/9.
//

#include "SPLog.h"
#include <stdarg.h>
#include <stdio.h>
#include <chrono>

static ESPLogLevel LOG_LEVEL;

void SetLogLevel(const ESPLogLevel logLevel)
{
    LOG_LEVEL = logLevel;
}

ESPLogLevel GetLogLevel() {
    return LOG_LEVEL;
}

void SPLog(const ESPLogLevel level, const char *fuction, int line, const char *fmt, ...) {
    if (GetLogLevel() >= level)
        return;
    
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

    std::tm* localTime = std::localtime(&time);
    
    printf("%2d:%2d:%2d.%3lld %s[%d] [SimplePlayer] ", localTime->tm_hour, localTime->tm_min, localTime->tm_sec, milliseconds.count(), fuction, line);
    
    va_list args;
    va_start(args, fmt);

    vprintf(fmt, args);
    putchar('\n');
    
    va_end(args);
}

