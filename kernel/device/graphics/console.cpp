#include "console.hpp"

// Console
Console::Console(const pxWriter&pxWr,const Pixel&fg,const Pixel&bg)
    :height(pxWr.conf.vres/16),width(pxWr.conf.hres/8),
    px(pxWr),fg_color(fg),bg_color(bg),buf{}{
    cury=curx=0;
    for(int32_t i=0;i<67;++i)
    for(int32_t j=0;j<240;++j)
        buf[i][j]=pannel(0,bg_color);
}

void Console::LineClear(int32_t line){
    int32_t ofs=line*16;
    for(int32_t i=0;i<16;++i)
    for(int32_t j=0;j<px.conf.hres;++j)
        px.write(j,i+ofs,bg_color);
}

void Console::NewLine(){
    curx=0;
    if(cury!=height-1){ ++cury; return; }
    for(int32_t i=0;i<height-1;++i){
        LineClear(i);
        for(int32_t j=0;j<width;++j){
            buf[i][j]=buf[i+1][j];
            px.writeAscii(j*8,i*16,buf[i][j].data,buf[i][j].color);
        }
    }
    LineClear(height-1);
    for(int32_t j=0;j<width;++j)
        buf[height-1][j]=pannel(0,bg_color);
}

void Console::Put(const char*s){
    for(;*s!='\0';++s)
        if(*s=='\n') NewLine();
        else if(curx<width){
            px.writeAscii(curx*8,cury*16,*s,fg_color);
            buf[cury][curx++]=pannel(*s,fg_color);
        }
}

void Console::clear()const{
    for(int32_t i=0;i<px.conf.vres;++i)
    for(int32_t j=0;j<px.conf.hres;++j)
        px.write(j,i,bg_color);
}