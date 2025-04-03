#include <cstdint>
#include <cstddef>
#include <climits>
#include <type_traits>
#include <utility>
#include <cstdio>

#include <vector>
#include <numeric>

#include "graphics/graphics.hpp"
#include "pci/pci.hpp"

#include "usb/memory.hpp"
#include "usb/device.hpp"
#include "usb/classdriver/mouse.hpp"
#include "usb/xhci/xhci.hpp"
#include "usb/xhci/trb.hpp"

#include "interrupt.hpp"
#include "queue.hpp"

extern "C" uint16_t getCodeSegment(void);
extern "C" void LoadIDT(uint16_t limit, uint64_t offset);

inline pci::Device*xhc_dev=nullptr;
usb::xhci::Controller*xhc;

void MouseObserver(int8_t displacement_x,int8_t displacement_y);
void SwitchEhci2Xhci(const pci::Device&xhc_dev);

struct Message{
    enum Type{
        kInterruptXHCI,
    }type;
};

ArrayQueue<Message>*main_queue;

__attribute__((interrupt))
void intrruptHandlerXHCI(InterruptFrame*frame){
    main_queue->push(Message{Message::kInterruptXHCI});
    NotifyEndOfInterrupt();
}


extern "C" void KernelMain(const uint64_t display_count,const FrameBufConfig*fbc){
    setup_console(display_count,fbc);

    std::array<Message,4096>main_queue_buf;
    ArrayQueue<Message>main_queue(main_queue_buf);
    ::main_queue=&main_queue;

    mouse_cursor=new(g_mouse_cursor_buf) MouseCursor{pxWr[0],{300,200}};
    pci::ScanAllBus();
    // enumerate all device
    for(int i=0;i<pci::devs_cnt;++i){
        auto cc=pci::getClassCode(pci::devs[i]);
        auto vd=pci::getVendorID(pci::devs[i]);
        int32_t hd=pci::getHeaderType(pci::devs[i]);
        printk("%d.%d.%d: vend %4d, class: base{%02x} sub{%02x} itf{%02x}, head %02d\n",
            pci::devs[i].bus,pci::devs[i].device,pci::devs[i].func,
            vd,(int32_t)cc.base,(int32_t)cc.sub,(int32_t)cc.interface,hd
        );
    }
    for(int i=0;i<pci::devs_cnt;++i) // find_xHC_device
        if(pci::devs[i].classCode.Match(0x0c,0x03,0x30)){
            // USB3.0==xHC device
            xhc_dev=&pci::devs[i];
            break;;
        }
    if(xhc_dev){
        printk("xHC has been found :%d.%d.%d\n",xhc_dev->bus,xhc_dev->device,xhc_dev->func);
        const uint16_t cs=getCodeSegment();
        SetIDTEntry(idt[InterruptVector::kXHCI],MakeIDTAttr(DescriptorType::kInterruptGate,0),reinterpret_cast<uint64_t>(intrruptHandlerXHCI),cs);
        LoadIDT(sizeof(idt)-1,reinterpret_cast<uintptr_t>(&idt[0]));
        const uint8_t bsp_local_apic_id=*reinterpret_cast<const uint32_t*>(0xfee00020)>>24;
        pci::ConfigureMSIFixedDestination(*xhc_dev,bsp_local_apic_id,pci::MSITriggerMode::kLevel,pci::MSIDeliveryMode::kFixed,InterruptVector::kXHCI,0);

        const withError<uint64_t>xhc_bar=pci::getBAR(*xhc_dev,0);
        if(xhc_bar.error){
            printk("Error %s\n",xhc_bar.error.Name());
            return;
        }
        const uint64_t mouse_mmio_addr=xhc_bar.value&~(uint64_t)0xf;
        Log(kDebug,"addr %lx\n",mouse_mmio_addr);
        usb::xhci::Controller xhc{mouse_mmio_addr};
        if(0x8086==pci::getVendorID(*xhc_dev))
            SwitchEhci2Xhci(*xhc_dev);
        auto err=xhc.Initialize();
        Log(kDebug,"xhc.Initialize: %s\n",err.Name());
        Log(kInfo,"Starting xHC\n");
        xhc.Run();
        ::xhc=&xhc;
        __asm__("sti");
        usb::HIDMouseDriver::default_observer = MouseObserver;
        for(uint8_t i=0;i<=xhc.MaxPorts();++i){
            usb::xhci::Port port=xhc.PortAt(i);
            if(port.IsConnected()){
                if(auto err=usb::xhci::ConfigurePort(xhc,port))
                    printk("Error filed to configure port: %s\n",err.Name());
            }
        }
        while(1){
            __asm__("cli");
            if(main_queue.size()==0){
                __asm__("sti\n\thlt");
                continue;
            }
            Message msg=main_queue.front();
            main_queue.pop();
            __asm__("sti");
            switch(msg.type){
            case Message::kInterruptXHCI:
                while(xhc.PrimaryEventRing()->HasFront())
                    if(auto err=usb::xhci::ProcessEvent(xhc))
                        Log(kError,"Error while ProcessEvent: %s at %s:%d\n",err.Name(),err.File(),err.Line());
                break;
            default:
                Log(kError,"Unknown Message::Type: %d\n",msg.type);
                break;
            }
        }
    }else printk("no usb3.0 device was detected.\n");
    while(1)__asm__("hlt");
}

void MouseObserver(int8_t displacement_x,int8_t displacement_y){
    mouse_cursor->MoveRelative({displacement_x,displacement_y});
}

void SwitchEhci2Xhci(const pci::Device&xhc_dev){
    bool intel_ehc_exist=false;
    for(int32_t i=0;i<pci::devs_cnt;++i)
        if(pci::devs[i].classCode.Match(0x0c,0x03,0x20)&&0x8086==pci::getVendorID(pci::devs[i])){
            intel_ehc_exist=true;
            break;
        }
    if(!intel_ehc_exist)return;
    uint32_t superSpeed_ports=pci::getConfReg(xhc_dev,0xdc);
    pci::setConfReg(xhc_dev,0xd8,superSpeed_ports);
    uint32_t ehci2xhci_ports=pci::getConfReg(xhc_dev,0xd4);
    pci::setConfReg(xhc_dev,0xd0,ehci2xhci_ports);
    printk("SwitchEhci2Xhci: SS =%02x, xHCI = %02x\n",superSpeed_ports,ehci2xhci_ports);
}