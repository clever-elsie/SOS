#ifndef SOS_KERNEL_PIXEL_WRITER
#define SOS_KERNEL_PIXEL_WRITER
#include <cstddef>
#include <cstdint>
#include "fonts.hpp"
#include "frameBufConfig.hpp"
#include "include/placement_new.hpp"

template<class T> struct Vec2D{
    T x,y;
    Vec2D(const T&x_,const T&y_):x(x_),y(y_){}
    template<class U>
    Vec2D<T>& operator+=(const Vec2D<U>&rhs){
        x+=rhs.x,y+=rhs.y;
        return*this;
    }
};

struct Pixel{
    uint8_t r,g,b;
    Pixel()=default;
    Pixel&operator=(const Pixel&rhs);
};

class pxWriter{
    const FrameBufConfig&conf;
    friend class Console;
    protected:
    inline uint8_t* px_at(int32_t x,int32_t y)const;
    public:
    pxWriter()=default;
    pxWriter(const FrameBufConfig&Conf_):conf(Conf_){}
    virtual ~pxWriter()=default;
    virtual void write(int32_t x,int32_t y,const Pixel&c)const=0;
    void writeAscii(int32_t x,int32_t y,char c,const Pixel&color)const;
    void writeString(int32_t x,int32_t y,const char*s,const Pixel&color)const;
    void fillRect(int32_t x,int32_t y,int32_t width,int32_t height,const Pixel&color)const;
    void drawRect(int32_t x,int32_t y,int32_t width,int32_t height,const Pixel&color)const;

    uint32_t width()const{return conf.hres;}
    uint32_t height()const{return conf.vres;}
    virtual Pixel color_at(int32_t x,int32_t y)const=0;
};

class rgbWriter:public pxWriter{
    public:
    using pxWriter::pxWriter;
    using pxWriter::writeAscii;
    using pxWriter::writeString;
    virtual void write(int32_t x,int32_t y,const Pixel&c) const override;
    virtual Pixel color_at(int32_t x,int32_t y)const override;
};

class bgrWriter:public pxWriter{
    public:
    using pxWriter::pxWriter;
    using pxWriter::writeAscii;
    using pxWriter::writeString;
    virtual void write(int32_t x,int32_t y,const Pixel&c) const override;
    virtual Pixel color_at(int32_t x,int32_t y)const override;
};

#endif