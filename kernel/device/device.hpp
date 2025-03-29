#ifndef SOS_KERNEL_DEVICE
#define SOS_KERNEL_DEVICE
#include <cstdint>
#include <cstddef>

#include "graphics/graphics.hpp"
#include "pci/pci.hpp"

#include "usb/memory.hpp"
#include "usb/device.hpp"
#include "usb/classdriver/mouse.hpp"
#include "usb/xhci/xhci.hpp"
#include "usb/xhci/trb.hpp"

inline pci::Device*xhc_dev=nullptr;

void setup_device(const int32_t display_count,const FrameBufConfig*fbc);

void MouseObserver(int8_t displacement_x,int8_t displacement_y);
void SwitchEhci2Xhci(const pci::Device&xhc_dev);

#endif