#include "pci.hpp"

namespace pci{

void IoOut32(uint16_t addr,uint32_t data){
    asm volatile(
        "outl %1, %0"
        :
        :"dN"(addr), "a"(data)
    );
}

uint32_t IoIn32(uint16_t addr){
    uint32_t res;
    asm volatile(
        "inl %1, %0"
        : "=a"(res)
        : "dN"(addr)
    );
    return res;
}

uint32_t genAddr(uint8_t bus,uint8_t device,uint8_t func,uint8_t reg_addr){
    using uw=uint32_t;
    return 1u<<31 | (uw)bus<<16 | (uw)device<<11 | (uw)func<<8 | (reg_addr&0xfcu);
}

void setAddr(uint32_t addr){ IoOut32(gPciConfigAddress,addr); }
void setData(uint32_t val){ IoOut32(gPciConfigData,val);}
uint32_t getData(){ return IoIn32(gPciConfigData);}

uint16_t getVendorID(uint8_t bus,uint8_t device,uint8_t func){
    setAddr(genAddr(bus,device,func,0x00));
    return getData()&0xFFFFu;
}
uint16_t getVendorID(const Device&dev){ return getVendorID(dev.bus,dev.device,dev.func); }

uint16_t getDeviceID(uint8_t bus,uint8_t device,uint8_t func){
    setAddr(genAddr(bus,device,func,0x00));
    return getData()>>16;
}
uint16_t getDeviceID(const Device&dev){ return getDeviceID(dev.bus,dev.device,dev.func); }

uint8_t getHeaderType(uint8_t bus,uint8_t device,uint8_t func){
    setAddr(genAddr(bus,device,func,0x0c));
    return getData()>>16 & 0xFFu;
}
uint8_t getHeaderType(const Device&dev){ return getHeaderType(dev.bus,dev.device,dev.func); }

ClassCode getClassCode(uint8_t bus,uint8_t device,uint8_t func){
    setAddr(genAddr(bus,device,func,0x08));
    ClassCode cc;
    uint32_t v=getData();
    cc.base=v>>24&0xFFu;
    cc.sub=v>>16&0xFFu;
    cc.interface=v>>8&0xffu;
    return cc;
}
ClassCode getClassCode(const Device&dev){ return getClassCode(dev.bus,dev.device,dev.func); }

uint32_t getBusNumbers(uint8_t bus,uint8_t device,uint8_t func){
    setAddr(genAddr(bus,device,func,0x18));
    return getData();
}
uint32_t getBusNumbers(const Device&dev){ return getBusNumbers(dev.bus,dev.device,dev.func); }
bool is_single_func_device(uint8_t header_type){ return (header_type&0x80u)==0; }

}

namespace pci{
Error AddDevice(const Device&dev){
    if(devs_cnt==devs.size())return Error{Error::kFull,__FILE__,__LINE__};
    devs[devs_cnt++]=dev;
    return Error{Error::kSuccess,__FILE__,__LINE__};
}

// prototype declaration
Error ScanBus(uint8_t);

Error ScanFunction(uint8_t bus,uint8_t device,uint8_t func){
    uint8_t header_type=getHeaderType(bus,device,func);
    ClassCode class_code=getClassCode(bus,device,func);
    Device dev{bus,device,func,header_type,class_code};
    if(auto err=AddDevice(dev))
        return err;
    if(class_code.Match(0x06u,0x04u)){
        // standard PCI-PCI bridge
        uint32_t bus_numbers=getBusNumbers(bus,device,func);
        uint8_t secondary_bus=(bus_numbers>>8)&0xFFu;
        return ScanBus(secondary_bus);
    }
    return Error{Error::kSuccess,__FILE__,__LINE__};
}

Error ScanDevice(uint8_t bus,uint8_t device){
    if(auto err=ScanFunction(bus,device,0))return err;
    if(is_single_func_device(getHeaderType(bus,device,0)))
        return Error{Error::kSuccess,__FILE__,__LINE__};
    for(uint8_t func=1;func<8;++func){
        if(getVendorID(bus,device,func)==0xFFFFu)continue;
        if(auto err=ScanFunction(bus,device,func))return err;
    }
    return Error{Error::kSuccess,__FILE__,__LINE__};
}

Error ScanBus(uint8_t bus){
    for(uint8_t device=0;device<32;++device){
        if(getVendorID(bus,device,0)==0xFFFFu)continue;
        if(auto err=ScanDevice(bus,device))return err;
    }
    return Error{Error::kSuccess,__FILE__,__LINE__};
}

Error ScanAllBus(){
    devs_cnt=0;
    uint8_t header_type=getHeaderType(0,0,0);
    if(is_single_func_device(header_type))return ScanBus(0);
    for(uint8_t func=1;func<8;++func){
        if(getVendorID(0,0,func)==0xFFFFu)continue;
        if(Error err=ScanBus(func)) return err;
    }
    return Error{Error::kSuccess,__FILE__,__LINE__};
}
}

namespace pci{
    uint8_t CalcBarAddress(uint32_t bar_index){ return 0x10+4*bar_index; }
    uint32_t getConfReg(const Device&dev,uint8_t reg_addr){
        setAddr(genAddr(dev.bus,dev.device,dev.func,reg_addr));
        return getData();
    }
    void setConfReg(const Device&dev,uint8_t reg_addr,uint32_t val){
        setAddr(genAddr(dev.bus,dev.device,dev.func,reg_addr));
        setData(val);
    }
    withError<uint64_t>getBAR(const Device&device,uint32_t bar_index){
        if(bar_index>=6)return {0,Error{Error::kIndexOutOfRange,__FILE__,__LINE__}};
        const uint8_t addr=CalcBarAddress(bar_index);
        const uint32_t bar=getConfReg(device,addr);
        if((bar&4u)==0)return {bar,Error{Error::kSuccess,__FILE__,__LINE__}};
        const uint32_t bar_upper=getConfReg(device,addr+4);
        return{bar|((uint64_t)bar_upper<<32),Error{Error::kSuccess,__FILE__,__LINE__}};
    }
}

namespace pci{
    CapabilityHeader getCapabilityHeader(const Device&dev,uint8_t addr){
        CapabilityHeader header;
        header.data=pci::getConfReg(dev,addr);
        return header;
    }
    MSICapability getMSICapability(const Device& dev, uint8_t cap_addr) {
        MSICapability msi_cap{};
        msi_cap.header.data = getConfReg(dev, cap_addr);
        msi_cap.msg_addr = getConfReg(dev, cap_addr + 4);
        uint8_t msg_data_addr = cap_addr + 8;
        if (msi_cap.header.bits.addr_64_capable) {
            msi_cap.msg_upper_addr = getConfReg(dev, cap_addr + 8);
            msg_data_addr = cap_addr + 12;
        }
        msi_cap.msg_data = getConfReg(dev, msg_data_addr);
        if (msi_cap.header.bits.per_vector_mask_capable) {
          msi_cap.mask_bits = getConfReg(dev, msg_data_addr + 4);
          msi_cap.pending_bits = getConfReg(dev, msg_data_addr + 8);
        }
        return msi_cap;
    }
    void setMSICapability(const Device& dev, uint8_t cap_addr,const MSICapability& msi_cap) {
        setConfReg(dev, cap_addr, msi_cap.header.data);
        setConfReg(dev, cap_addr + 4, msi_cap.msg_addr);
        uint8_t msg_data_addr = cap_addr + 8;
        if (msi_cap.header.bits.addr_64_capable) {
          setConfReg(dev, cap_addr + 8, msi_cap.msg_upper_addr);
          msg_data_addr = cap_addr + 12;
        }
        setConfReg(dev, msg_data_addr, msi_cap.msg_data);
        if (msi_cap.header.bits.per_vector_mask_capable) {
          setConfReg(dev, msg_data_addr + 4, msi_cap.mask_bits);
          setConfReg(dev, msg_data_addr + 8, msi_cap.pending_bits);
        }
    }

    Error ConfigureMSIRegister(const Device& dev, uint8_t cap_addr,uint32_t msg_addr, uint32_t msg_data,unsigned int num_vector_exponent) {
        auto msi_cap = getMSICapability(dev, cap_addr);
        if (msi_cap.header.bits.multi_msg_capable <= num_vector_exponent)
            msi_cap.header.bits.multi_msg_enable =msi_cap.header.bits.multi_msg_capable;
         else msi_cap.header.bits.multi_msg_enable = num_vector_exponent;

        msi_cap.header.bits.msi_enable = 1;
        msi_cap.msg_addr = msg_addr;
        msi_cap.msg_data = msg_data;

        setMSICapability(dev, cap_addr, msi_cap);
        return MAKE_ERROR(Error::kSuccess);
    }

  /** @brief 指定された MSI レジスタを設定する */
    Error ConfigureMSIXRegister(const Device& dev, uint8_t cap_addr,uint32_t msg_addr, uint32_t msg_data,unsigned int num_vector_exponent) {
        return MAKE_ERROR(Error::kNotImplemented);
    }

    Error ConfigureMSI(const Device&dev,uint32_t msg_addr,uint32_t msg_data,unsigned int num_vec_exponent){
        uint8_t cap_addr=pci::getConfReg(dev,0x34)&0xFFu;
        uint8_t msi_cap_addr=0,msix_cap_addr=0;
        while(cap_addr){
            auto header=getCapabilityHeader(dev,cap_addr);
            if(header.bits.cap_id==kCapabilityMSI) msi_cap_addr=cap_addr;
            else if(header.bits.cap_id==kCapabilityMSIX) msix_cap_addr=cap_addr;
            cap_addr=header.bits.next_ptr;
        }
        if(msi_cap_addr) return ConfigureMSIRegister(dev,msi_cap_addr,msg_addr,msg_data,num_vec_exponent);
        if(msix_cap_addr)return ConfigureMSIXRegister(dev,msix_cap_addr,msg_addr,msg_data,num_vec_exponent);
        return MAKE_ERROR(Error::kNoPCIMSI);
    }

    Error ConfigureMSIFixedDestination(const Device& dev, uint8_t apic_id,MSITriggerMode trigger_mode, MSIDeliveryMode delivery_mode,uint8_t vector, unsigned int num_vector_exponent) {
        uint32_t msg_addr = 0xfee00000u | (apic_id << 12);
        uint32_t msg_data = (static_cast<uint32_t>(delivery_mode) << 8) | vector;
        if (trigger_mode == MSITriggerMode::kLevel) msg_data |= 0xc000;
        return ConfigureMSI(dev, msg_addr, msg_data, num_vector_exponent);
    }
}