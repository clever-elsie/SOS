#include "pci.hpp"

namespace {
using namespace pci;

inline constexpr uint16_t gPciConfigAddress=0x0cf8;
inline constexpr uint16_t gPciConfigData=0x0cfc;

inline uint32_t genAddr(uint8_t bus,uint8_t device,uint8_t func,uint8_t reg_addr){
    using uw=uint32_t;
    return 1u<<31 | (uw)buf<<16 | (uw)device<<11 | (uw)func<<8 | (reg_addr&0xfcu);
}

inline void setAddr(uint32_t addr){ IoOut32(gPciConfigAddress,addr); }
inline void setData(uint32_t val){ IoOut32(gPciConfigData,val);}
inline uint32_t getData(){ return IoIn32(gPciConfigData);}

inline uint16_t getVendorID(uint8_t bus,uint8_t device,uint8_t func){
    setAddr(genAddr(bus,device,func,0x00));
    return getData()&0xFFFFu;
}

inline uint16_t getDeviceID(uint8_t bus,uint8_t device,uint8_t func){
    setAddr(genAddr(bus,device,func,0x00));
    return getData()>>16;
}
inline uint16_t getHeaderType(uint8_t bus,uint8_t device,uint8_t func){
    setAddr(genAddr(bus,device,func,0x0c));
    return getData()>>16 & 0xFFu;
}
inline uint32_t getClassCode(uint8_t bus,uint8_t device,uint8_t func){
    setAddr(genAddr(bus,device,func,0x08));
    return getData();
}
uint32_t getBusNumbers(uint8_t bus,uint8_t device,uint8_t func){
    setAddr(genAddr(bus,device,func,0x18));
    return getData();
}
bool is_single_func_device(uint8_t header_type){
    return (header_type&0x80u)==0;
}

}