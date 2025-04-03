#ifndef SOS_KERNEL_PCI
#define SOS_KERNEL_PCI
#include <stdint.h>
#include <array>
#include "error.hpp"
namespace pci{

inline constexpr uint16_t gPciConfigAddress=0x0cf8;
inline constexpr uint16_t gPciConfigData=0x0cfc;

class ClassCode{
    public:
    uint8_t base,sub,interface;
    friend ClassCode getClassCode(uint8_t,uint8_t,uint8_t);
    bool Match(uint8_t b){return b==base;}
    bool Match(uint8_t b,uint8_t s){return b==base&&s==sub;}
    bool Match(uint8_t b,uint8_t s,uint8_t i){
        return b==base&&s==sub&&i==interface;
    }
};
struct Device{
    uint8_t bus,device,func,header_type;
    ClassCode classCode;
};

void IoOut32(uint16_t addr,uint32_t data);
uint32_t IoIn32(uint16_t addr);
uint32_t genAddr(uint8_t bus,uint8_t device,uint8_t func,uint8_t reg_addr);

void setAddr(uint32_t addr);
void setData(uint32_t val);
uint32_t getData();
uint16_t getVendorID(uint8_t bus,uint8_t device,uint8_t func);
uint16_t getVendorID(const Device&dev);

uint16_t getDeviceID(uint8_t bus,uint8_t device,uint8_t func);
uint16_t getDeviceID(const Device&dev);
uint8_t getHeaderType(uint8_t bus,uint8_t device,uint8_t func);
uint8_t getHeaderType(const Device&dev);
/**
 * [31:24] base class
 * [23:16] sub class
 * [15: 8] interface
 * [ 7: 0] revision
 */
ClassCode getClassCode(uint8_t bus,uint8_t device,uint8_t func);
ClassCode getClassCode(const Device&dev);
/**
 * [23:16] sub ordinate bus number
 * [15: 8] secondary bus number
 * [ 7: 0] revision number
 */
uint32_t getBusNumbers(uint8_t bus,uint8_t device,uint8_t func);
uint32_t getBusNumbers(const Device&dev);
bool is_single_func_device(uint8_t header_type);
//
// pci device
inline std::array<Device,128>devs;
inline int32_t devs_cnt;

Error ScanAllBus();

uint32_t getConfReg(const Device&dev,uint8_t reg_addr);
void setConfReg(const Device&dev,uint8_t reg_addr,uint32_t val);
withError<uint64_t> getBAR(const Device&v,uint32_t bar_index);
uint8_t CalcBarAddress(uint32_t bar_index);


union CapabilityHeader {
uint32_t data;
struct {
  uint32_t cap_id : 8;
  uint32_t next_ptr : 8;
  uint32_t cap : 16;
} __attribute__((packed)) bits;
} __attribute__((packed));

const uint8_t kCapabilityMSI = 0x05;
const uint8_t kCapabilityMSIX = 0x11;

CapabilityHeader getCapabilityHeader(const Device& dev, uint8_t addr);

struct MSICapability {
union {
  uint32_t data;
  struct {
    uint32_t cap_id : 8;
    uint32_t next_ptr : 8;
    uint32_t msi_enable : 1;
    uint32_t multi_msg_capable : 3;
    uint32_t multi_msg_enable : 3;
    uint32_t addr_64_capable : 1;
    uint32_t per_vector_mask_capable : 1;
    uint32_t : 7;
  } __attribute__((packed)) bits;
} __attribute__((packed)) header ;

uint32_t msg_addr;
uint32_t msg_upper_addr;
uint32_t msg_data;
uint32_t mask_bits;
uint32_t pending_bits;
} __attribute__((packed));

Error ConfigureMSI(const Device& dev, uint32_t msg_addr, uint32_t msg_data, unsigned int num_vector_exponent);

enum class MSITriggerMode { kEdge = 0, kLevel = 1 };

enum class MSIDeliveryMode { kFixed = 0b000, kLowestPriority = 0b001, kSMI = 0b010, kNMI = 0b100, kINIT = 0b101, kExtINT = 0b111, };

Error ConfigureMSIFixedDestination(const Device& dev, uint8_t apic_id,MSITriggerMode trigger_mode, MSIDeliveryMode delivery_mode,uint8_t vector, unsigned int num_vector_exponent);

}
#endif