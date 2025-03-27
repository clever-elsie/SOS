#ifndef SOS_KERNEL_PCI
#define SOS_KERNEL_PCI
#include <stdint.h>
namespace pci{
inline constexpr uint16_t gPciConfigAddress;
inline constexpr uint16_t gPciConfigData;
void IoOut32(uint16_t addr,uint32_t data);
uint32_t IoIn32(uint16_t addr);
inline uint32_t genAddr(uint8_t bus,uint8_t device,uint8_t func,uint8_t reg_addr);

inline void setAddr(uint32_t addr);
inline void setData(uint32_t val);
inline uint32_t getData();
inline uint16_t getVendorID(uint8_t bus,uint8_t device,uint8_t func);

// 未実装
inline uint16_t getDeviceID(uint8_t bus,uint8_t device,uint8_t func);
inline uint16_t getHeaderType(uint8_t bus,uint8_t device,uint8_t func);
/**
 * [31:24] base class
 * [23:16] sub class
 * [15: 8] interface
 * [ 7: 0] revision
 */
inline uint32_t getClassCode(uint8_t bus,uint8_t device,uint8_t func);
/**
 * [23:16] sub ordinate bus number
 * [15: 8] secondary bus number
 * [ 7: 0] revision number
 */
uint32_t getBusNumbers(uint8_t bus,uint8_t device,uint8_t func);
bool is_single_func_device(uint8_t header_type);

struct Device{ uint8_t bus,device,func,header_type; };

} // namespace pci
#endif