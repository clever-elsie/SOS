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
#include "memory_map.hpp"
#include "segment.hpp"
#include "paging.hpp"
#include "memory_manager.hpp"


// fn of asm is declared below
extern "C" uint16_t getCodeSegment(void);
extern "C" void LoadIDT(uint16_t limit, uint64_t offset);
extern "C" void LoadGDT(uint16_t limit, uint64_t offset);
extern "C" void SetCSSS(uint16_t cs, uint16_t ss);
extern "C" void SetDSAll(uint16_t value);
extern "C" void SetCR3(uint64_t value);
// end of declaration of asm functions

inline pci::Device*xhc_dev=nullptr;
usb::xhci::Controller*xhc;

char g_memory_manager_buf[sizeof(BitmapMemoryManager)];
BitmapMemoryManager*memory_manager;

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

alignas(16) uint8_t kernel_main_stack[1024*1024];

extern "C" void KernelMainNewStack(const uint64_t display_count_cp,const FrameBufConfig*fbc_ref,const MemoryMap&memory_map_ref){
    display_count=display_count_cp;
    FrameBufConfig fbc[MAX_DISPLAY_COUNT];
    for(int i=0;i<display_count;++i)
        fbc[i]=fbc_ref[i];
    setup_console(fbc);
    MemoryMap memory_map{memory_map_ref};
    const std::array available_memory_types{
      MemoryType::kEfiBootServicesCode,
      MemoryType::kEfiBootServicesData,
      MemoryType::kEfiConventionalMemory,
    };

    SetupSegments();
    const uint16_t kernel_cs=1<<3, kernel_ss=2<<3;
    SetDSAll(0);
    SetCSSS(kernel_cs,kernel_ss);
    SetupIdentityPageTable();

    std::array<Message,4096>main_queue_buf;
    ArrayQueue<Message>main_queue(main_queue_buf);
    ::main_queue=&main_queue;
    
    memory_manager=new(g_memory_manager_buf) BitmapMemoryManager;
    const auto memory_map_base=reinterpret_cast<uintptr_t>(memory_map.buffer);
    uintptr_t available_end=0;
    for(uintptr_t itr=memory_map_base;itr<memory_map_base+memory_map.map_size;itr+=memory_map.descriptor_size){
        const auto desc=reinterpret_cast<const MemoryDescriptor*>(itr);
        if(available_end<desc->physical_start)
            memory_manager->MarkAllocated(FrameID{available_end/kBytesPerFrame},(desc->physical_start-available_end)/kBytesPerFrame);
        const auto physical_end=desc->physical_start+desc->number_of_pages*kUEFIPageSize;
        if(IsAvailable(static_cast<MemoryType>(desc->type)))
            available_end=physical_end;
        else memory_manager->MarkAllocated(FrameID{desc->physical_start/kBytesPerFrame},desc->number_of_pages*kUEFIPageSize/kBytesPerFrame);
    }
    memory_manager->SetMemoryRange(FrameID{1},FrameID{available_end/kBytesPerFrame});

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