#ifndef SOS_KERNEL_CONSOLE
#define SOS_KERNEL_CONSOLE

#include <cstddef>
#include <cstdint>
#include "pxWriter.hpp"

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