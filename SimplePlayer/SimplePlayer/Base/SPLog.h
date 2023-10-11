//
//  SPLog.hpp
//  SimplePlayer
//
//  Created by YangYixuan on 2023/7/9.
//

#ifndef SPLog_hpp
#define SPLog_hpp

#ifdef __cplusplus
extern "C" {
#endif

enum ESPLogLevel {
    SP_LOG_VERBOSE = 0,
    SP_LOG_DEBUG,
    SP_LOG_INFO,
    SP_LOG_WARN,
    SP_LOG_ERROR,
    SP_LOG_OFF,
} ;

void SetLogLevel(const ESPLogLevel logLevel);

ESPLogLevel GetLogLevel();

void SPLog(const ESPLogLevel level, const char *fuction, int line, const char *fmt, ...);


#ifndef SPLOGV
#define SPLOGV(format, ...)  SPLog(SP_LOG_VERBOSE, __FUNCTION__, __LINE__, format, ##__VA_ARGS__)
#endif

#ifndef SPLOGD
#define SPLOGD(format, ...)  SPLog(SP_LOG_DEBUG,   __FUNCTION__, __LINE__, format, ##__VA_ARGS__)
#endif

#ifndef SPLOGI
#define SPLOGI(format, ...)  SPLog(SP_LOG_INFO,    __FUNCTION__, __LINE__, format, ##__VA_ARGS__)
#endif

#ifndef SPLOGW
#define SPLOGW(format, ...)  SPLog(SP_LOG_WARN,    __FUNCTION__, __LINE__, format, ##__VA_ARGS__)
#endif

#ifndef SPLOGE
#define SPLOGE(format, ...)  SPLog(SP_LOG_ERROR,   __FUNCTION__, __LINE__, format, ##__VA_ARGS__)
#endif


extern void SP_Assert(bool condition, const char *conditionDesc, const char *function, const char *file, int line, const char *description, ...);

#define SPASSERT0(condition) SP_Assert(condition, #condition, __PRETTY_FUNCTION__, __FILE_NAME__, __LINE__, NULL)
#define SPASSERT1(condition, desc, ...) SP_Assert(condition, #condition, __PRETTY_FUNCTION__, __FILE_NAME__, __LINE__, desc, ##__VA_ARGS__)

#define SPASSERT            SPASSERT0
#define SPASSERTEX          SPASSERT1
#define SPASSERT_NOT_IMPL   SPASSERT(0)

#ifdef __cplusplus
}
#endif

#endif /* SPLog_hpp */
