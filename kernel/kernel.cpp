#include <cstdint>
#include <cstddef>
#include <climits>
#include <type_traits>
#include <utility>
#include <cstdio>
#include "graphics/graphics.hpp"

class Mouse{
    constexpr static char bitmap[24][16]={
        {"RR              "},
        {"RxR             "},
        {"RxxRR           "},
        {"RxxxxR          "},
        {"RxxxxxRR        "},
        {"RxxxxxxxR       "},
        {"RxxxxxxxxRR     "},
        {"RxxxxxxxxxxR    "},
        {"RxxxxxxxxxxxRR  "},
        {"RxxxxxxxxxxxxxR "},
        {"RxxxxxxxxxxxxxxR"},
        {"RxxxxxxxxxxxxRRR"},
        {"RxxxxxxxxxxRR   "},
        {"RxxxxxxxxxR     "},
        {"RxxxxxxxxR      "},
        {"RxxxxxRRxR      "},
        {"RxxxxR  RxR     "},
        {"RxxxR   RxR     "},
        {"RxxR     RxR    "},
        {"RxR      RxR    "},
        {"RR        RxR   "},
        {"          RxR   "},
        {"           RxR  "},
        {"           RRR  "},
    };
};


extern "C" void KernelMain(const uint64_t display_count,const FrameBufConfig*fbc){
    setup_console(display_count,fbc);
    char buf[1024];
    for(int32_t i=0;i<400;++i){
        sprintf(buf,"%d\n",i);
        console[0]->Put(buf);
    }
    while(1)__asm__("hlt");
}