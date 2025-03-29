#include "mouse.hpp"
using namespace MouseCursorImage;

Vec2D<uint32_t>MouseCursor::draw_check_bound(uint32_t x,uint32_t y){
    uint32_t h=y+MouseCursorHeight;
    uint32_t w=x+MouseCursorWidth;
    if(auto th=px_writer->height();h>th)h=th;
    if(auto tw=px_writer->width();w>tw)w=tw;
    return {w,h};
}

void MouseCursor::copy_back(uint32_t x,uint32_t y){
    auto[w,h]=draw_check_bound(x,y);
    for(uint32_t i=y;i<h;++i)
    for(uint32_t j=x;j<w;++j)
        bg[i-y][j-x]=px_writer->color_at(j,i);
}

void MouseCursor::draw_at(uint32_t x,uint32_t y){
    auto[w,h]=draw_check_bound(x,y);
    for(uint32_t i=y;i<h;++i)
    for(uint32_t j=x;j<w;++j)
        switch(MouseCursorShape[i-y][j-x]){
            case '@':px_writer->write(j,i,{30,30,30}); break;
            case '.':px_writer->write(j,i,{255,255,255}); break;
            default:break;
        }
}

void MouseCursor::erase_at(uint32_t x,uint32_t y){
    auto[w,h]=draw_check_bound(x,y);
    for(uint32_t i=y;i<h;++i)
    for(uint32_t j=x;j<w;++j)
        px_writer->write(j,i,bg[i-y][j-x]);
}

MouseCursor::MouseCursor(pxWriter*writer,Vec2D<int32_t>init_pos)
:px_writer(writer),bg{},pos(init_pos){
    copy_back(init_pos.x,init_pos.y);
    draw_at(init_pos.x,init_pos.y);
}

void MouseCursor::MoveRelative(Vec2D<int32_t>displacement){
    erase_at(pos.x,pos.y);
    pos+=displacement;
    if(pos.x<0)pos.x=0;
    else if(pos.x>=px_writer->width())
        pos.x=px_writer->width()-1;
    if(pos.y<0)pos.y=0;
    else if(pos.y>=px_writer->height())
        pos.y=px_writer->width()-1;
    copy_back(pos.x,pos.y);
    draw_at(pos.x,pos.y);
}