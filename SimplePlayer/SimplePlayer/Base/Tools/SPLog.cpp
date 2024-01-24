//
//  SPLog.cpp
//  SimplePlayer
//
//  Created by YangYixuan on 2023/7/9.
//

#include "SPLog.h"
#include <stdlib.h>
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
    
    printf("%02d:%02d:%02d.%03lld %s[%d] [SimplePlayer] ", localTime->tm_hour, localTime->tm_min, localTime->tm_sec, milliseconds.count(), fuction, line);
    
    va_list args;
    va_start(args, fmt);

    vprintf(fmt, args);
    putchar('\n');
    
    va_end(args);
}


void SP_Assert(bool condition, const char *conditionDesc, const char *function, const char *file, int line, const char *description, ...) {
    if (condition == true)
        return;
    if (description == NULL)
        description = "";
    
    printf("\n=====[ASSERT FAILED]=====\n"
           "Assert Failed:(%s)\n"
           "Function:%s\n"
           "File:%s\n"
           "Line:%d\n"
           "Info:",
           conditionDesc, function, file, line);
    
    va_list args;
    va_start(args, description);

    vprintf(description, args);
    putchar('\n');
    
    va_end(args);
    
#if defined(_WIN32) && defined(_DEBUG) && defined(_MSC_VER)
    __debugbreak();
#elif defined(DEBUG)
    abort();
#else
#endif
}

