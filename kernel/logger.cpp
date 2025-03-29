#include "logger.hpp"

inline static LogLevel log_level = LogLevel::kWarn;

void setLogLever(LogLevel level){ log_level=level; }

int32_t Log(LogLevel level, const char* format, ...){
    if(level>log_level)return 0;
    va_list ap;
    int32_t res=0;
    constexpr int32_t maxlen=4096;
    char s[maxlen];
    va_start(ap,format);
    res=vsnprintf(s,maxlen,format,ap);
    va_end(ap);
    console[0]->Put(s);
    return res;
}