#ifndef SOS_KERNEL_GRAPHICS
#define SOS_KERNEL_GRAPHICS
#include <cstdint>
#include <cstddef>
#include <cstdio>
#include <stdarg.h>
#include "logger.hpp"
#include "mouse.hpp"
#include "pxWriter.hpp"
#include "console.hpp"

inline char g_pixel_writer_buffer[MAX_DISPLAY_COUNT][sizeof(rgbWriter)];
inline char g_console_buffer[MAX_DISPLAY_COUNT][sizeof(Console)];
inline char g_mouse_cursor_buf[sizeof(MouseCursor)];
inline pxWriter* pxWr[MAX_DISPLAY_COUNT];
inline Console*console[MAX_DISPLAY_COUNT];
inline MouseCursor*mouse_cursor;

void setup_console(const int32_t display_count,const FrameBufConfig*fbc);
int32_t printk(const char*const format,...);

#endif