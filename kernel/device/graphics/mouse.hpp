#ifndef SOS_KERNEL_MOUSE
#define SOS_KERNEL_MOUSE
#include "pxWriter.hpp"

namespace MouseCursorImage{
constexpr static uint8_t MouseCursorWidth=15;
constexpr static uint8_t MouseCursorHeight=24;
constexpr static char MouseCursorShape[MouseCursorHeight][MouseCursorWidth+1]={
    "@              ",
    "@@             ",
    "@.@            ",
    "@..@           ",
    "@...@          ",
    "@....@         ",
    "@.....@        ",
    "@......@       ",
    "@.......@      ",
    "@........@     ",
    "@.........@    ",
    "@..........@   ",
    "@...........@  ",
    "@............@ ",
    "@......@@@@@@@@",
    "@......@       ",
    "@....@@.@      ",
    "@...@ @.@      ",
    "@..@   @.@     ",
    "@.@    @.@     ",
    "@@      @.@    ",
    "@       @.@    ",
    "         @.@   ",
    "         @@@   ",
};
}

class MouseCursor{
    private:
    Vec2D<uint32_t>draw_check_bound(uint32_t x,uint32_t y);
    void copy_back(uint32_t x,uint32_t y);
    void draw_at(uint32_t x,uint32_t y);
    void erase_at(uint32_t x,uint32_t y);
    public:
    MouseCursor(pxWriter*writer,Vec2D<int32_t>init_pos);
    void MoveRelative(Vec2D<int32_t>displacement);
    private:
    pxWriter*px_writer=nullptr;
    Pixel bg[MouseCursorImage::MouseCursorHeight][MouseCursorImage::MouseCursorWidth];
    Vec2D<int32_t>pos;
};

#endif