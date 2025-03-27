#ifndef SOS_KERNEL_PIXEL_WRITER
#define SOS_KERNEL_PIXEL_WRITER

#include <cstddef>
#include <cstdint>
#include "fonts.hpp"
#include "frameBufConfig.hpp"

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
};

class rgbWriter:public pxWriter{
    public:
    using pxWriter::pxWriter;
    using pxWriter::writeAscii;
    using pxWriter::writeString;
    virtual void write(int32_t x,int32_t y,const Pixel&c) const override;
};

class bgrWriter:public pxWriter{
    public:
    using pxWriter::pxWriter;
    using pxWriter::writeAscii;
    using pxWriter::writeString;
    virtual void write(int32_t x,int32_t y,const Pixel&c) const override;
};

class Console{
    public:
    const int32_t height,width;
    private:
    const pxWriter&px;
    int32_t cury,curx;
    constexpr static Pixel default_fg{255,255,255},default_bg{20,20,20};
    Pixel fg_color,bg_color;
    struct pannel{
        uint8_t data;
        Pixel color;
        pannel()=default;
        pannel(uint8_t ch,const Pixel&color):data(ch),color(color){}
        pannel&operator=(const pannel&rhs)=default;
    };
    pannel buf[67][240]; // limit:1920x1080
    private:
    void LineClear(int32_t line);
    void NewLine();
    public:
    Console()=default;
    Console(const pxWriter&pxWr,const Pixel&fg=default_fg,const Pixel&bg=default_bg);
    void Put(const char*s);
    void clear()const;
};

#endif