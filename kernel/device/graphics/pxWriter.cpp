#include "pxWriter.hpp"

// Pixel
Pixel& Pixel::operator=(const Pixel&rhs){
    r=rhs.r,g=rhs.g,b=rhs.b; return*this;
}

// pxWriter
uint8_t*pxWriter::px_at(int32_t x,int32_t y)const{
    return conf.frame_buf+4*(y*conf.linesize+x);
}

void pxWriter::writeAscii(int32_t x,int32_t y,char c,const Pixel&color)const{
    for(int32_t i=0;i<16;++i)
    for(int32_t j=7;j>=0;--j)
    if((uint32_t)(Font[(size_t)c][i])&1<<j)
        write(x+7-j,y+i,color);
}

void pxWriter::writeString(int32_t x,int32_t y,const char*s,const Pixel&color)const{
    for(int32_t i=0;s[i]!='\0';++i) writeAscii(x+(i<<3),y,s[i],color);
}

void pxWriter::fillRect(int32_t x,int32_t y,int32_t width,int32_t height,const Pixel&color)const{
    if(width==0||height==0) return;
    int32_t mx=(x+width<=conf.hres?x+width:conf.hres);
    int32_t my=(y+height<=conf.vres?y+height:conf.vres);
    for(int32_t i=y;i<my;++i)
    for(int32_t j=x;j<mx;++j)
        write(j,i,color);
}

void pxWriter::drawRect(int32_t x,int32_t y,int32_t width,int32_t height,const Pixel&color)const{
    if(width==0||height==0) return;
    int32_t mx=(x+width<=conf.hres?x+width:conf.hres);
    int32_t my=(y+height<=conf.vres?y+height:conf.vres);
    for(int32_t i=y;i<my;++i)
        write(x,i,color),write(mx-1,i,color);
    for(int32_t j=x;j<mx;++j)
        write(j,y,color),write(j,my-1,color);
}

void rgbWriter::write(int32_t x,int32_t y,const Pixel&c)const{
    uint8_t*p=px_at(x,y);
    p[0]=c.r,p[1]=c.g,p[2]=c.b;
}

Pixel rgbWriter::color_at(int32_t x,int32_t y)const{
    uint8_t*p=px_at(x,y);
    Pixel ret;
    ret.r=p[0],ret.g=p[1],ret.b=p[2];
    return ret;
}

void bgrWriter::write(int32_t x,int32_t y,const Pixel&c)const{
    uint8_t*p=px_at(x,y);
    p[0]=c.b,p[1]=c.g,p[2]=c.r;
}

Pixel bgrWriter::color_at(int32_t x,int32_t y)const{
    uint8_t*p=px_at(x,y);
    Pixel ret;
    ret.b=p[0],ret.g=p[1],ret.r=p[2];
    return ret;
}