#ifndef SOS_KERNEL_GRAPHICS
#define SOS_KERNEL_GRAPHICS
#include <cstdint>
#include <cstddef>
#include <cstdio>
#include <stdarg.h>
#include "console.hpp"

template<class T>
void* operator new(size_t size,T*buf){return buf;}
void operator delete(void*buf)noexcept{}

char g_pixel_writer_buffer[MAX_DISPLAY_COUNT][sizeof(rgbWriter)];
char g_console_buffer[MAX_DISPLAY_COUNT][sizeof(Console)];
pxWriter* pxWr[MAX_DISPLAY_COUNT];
Console*console[MAX_DISPLAY_COUNT];

void setup_console(const int32_t display_count,const FrameBufConfig*fbc){
    for(int32_t i=0;i<display_count;++i){
        switch((uint64_t)fbc[i].fmt){
            case kRGB_8bitPerColor:
                pxWr[i]=new(&g_pixel_writer_buffer[i]) rgbWriter(fbc[i]);
            break;
            case kBGR_8bitPerColor:
                pxWr[i]=new(&g_pixel_writer_buffer[i]) bgrWriter(fbc[i]);
            break;
        }
        console[i]=new(&g_console_buffer[i]) Console(*pxWr[i]);
        console[i]->clear();
    }
}

int32_t printk(const char*const format,...){
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

#endif