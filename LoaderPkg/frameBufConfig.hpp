#ifndef PIXEL_FORMAT_HPP
#define PIXEL_FORMAT_HPP
#include <stdint.h>
enum PixelFormat{
    kRGB_8bitPerColor,
    kBGR_8bitPerColor,
    kBitmask, // unsupported
    kBltOnly, // unsupported
};

struct FrameBufConfig{
    uint8_t* frame_buf;
    uint32_t linesize;
    uint32_t hres;
    uint32_t vres;
    enum PixelFormat fmt;
};

#define MAX_DISPLAY_COUNT 4

#endif