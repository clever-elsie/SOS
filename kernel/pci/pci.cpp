#include "pci.hpp"

namespace {
using namespace pci;

inline constexpr uint16_t gPciConfigAddress=0x0cf8;
inline constexpr uint16_t gPciConfigData=0x0cfc;

inline void IoOut32(uint16_t addr,uint32_t data){
    asm volatile(
        "outl %1, %0"
        :
        :"dN"(addr), "a"(data)
    );
}

inline uint32_t IoIn32(uint16_t addr){
    uint32_t res;
    asm volatile(
        "inl %1, %0"
        : "=a"(res)
        : "dN"(addr)
    );
    return res;
}

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
inline uint8_t getHeaderType(uint8_t bus,uint8_t device,uint8_t func){
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

namespace pci{
inline std::array<Device,>devs;
Error AddDevice(uint8_t bus,uint8_t device,uint8_t func,uint8_t header_type){
    if(devs_cnt==devs.size())return Error::Code::full;
    devs[devs_cnt++]=Device{bus,device,func,header_type};
    return Error::Code::success;
}

Error ScanFunction(uint8_t bus,uint8_t device,uint8_t func){
    auto header_type=getHeaderType(bus,device,func);
    if(auto err=AddDevice(bus,device,func,header_type))
        return err;
    uint32_t class_code=getClassCode(bus,device,func);
    uint8_t base=(class_code>>24)&0xFFu;
    uint8_t sub=(class_code>>16)&0xFFu;
    if(base==0x06u && sub==0x04u){
        // standard PCI-PCI bridge
        uint32_t bus_numbers=getBusNumbers(bus,device,func);
        uint8_t secondary_bus=(bus_numbers>>8)&0xFFu;
        return ScanBus(secondary_bus);
    }
    return Error::Code::success;
}

Error ScanDevice(uint8_t bus,uint8_t device){
    if(auto err=ScanFunction(bus,device,0))return err;
    if(is_single_func_device(getHeaderType(bus,device,0)))
        return Error::Code::success;
    for(uint8_t func=1;func<8;++func){
        if(getVendorID(bus,device,func)==0xFFFFu)continue;
        if(auto err=ScanFunction(bus,device,func))return err;
    }
    return Error::Code::success;
}

Error ScanBus(uint8_t bus){
    for(uint8_t device=0;device<32;++device){
        if(getVendorID(bus,device,0)==0xFFFFu)continue;
        if(auto err=ScanDevice(bus,device))return err;
    }
    return Error::Code::success;
}

Error ScanAllBus(){
    devs_cnt=0;
    uint8_t header_type=getHeaderType(0,0,0);
    if(is_single_func_device(header_type))return ScanBus(0);
    for(uint8_t func=1;func<8;++func){
        if(getVendorID(0,0,func)==0xFFFFu)continue;
        if(Error err=ScanBus(func)) return err;
    }
    return Error::Code::success;
}
}