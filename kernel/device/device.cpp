#include "device.hpp"

void setup_device(const int32_t display_count,const FrameBufConfig*fbc){
    setup_console(display_count,fbc);
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
        usb::HIDMouseDriver::default_observer = MouseObserver;
        for(uint8_t i=0;i<=xhc.MaxPorts();++i){
            usb::xhci::Port port=xhc.PortAt(i);
            if(port.IsConnected()){
                if(auto err=usb::xhci::ConfigurePort(xhc,port))
                    printk("Error filed to configure port: %s\n",err.Name());
            }
        }
        while(1){
            if(auto err=usb::xhci::ProcessEvent(xhc))
                printk("Error while ProcessEvent: %s\n",err.Name());
        }
    }else printk("no usb3.0 device was detected.\n");
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