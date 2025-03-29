OPT=-target x86_64-elf -mno-red-zone -ffreestanding -fno-exceptions -fno-rtti -nostdlibinc -std=c++17 -Wall -g -O2 
LIB=-I ~/sos/LoaderPkg -I ~/osbook/devenv/x86_64-elf/include/c++/v1 -I ~/osbook/devenv/x86_64-elf/include -I ~/sos/kernel/device -I ~/sos/kernel
DEF=-D__ELF__ -D_LDBL_EQ_DBL -D_GNU_SOURCE -D_POSIX_TIMERS

KTAR=kernel/main.o
KCPP=kernel/kernel.cpp
KEFL=kernel/kernel.elf

HLP=kernel/include/newlib_helper.o kernel/include/placement_new.o
GPX=kernel/device/graphics
PCI=kernel/device/pci

USBD=kernel/device/usb
XHCI=$(USBD)/xhci
CLSD=$(USBD)/classdriver

OBJSG=$(GPX)/console.o $(GPX)/graphics.o $(GPX)/mouse.o $(GPX)/pxWriter.o


OBJSUSB=$(USBD)/device.o $(USBD)/memory.o $(XHCI)/device.o $(XHCI)/devmgr.o $(XHCI)/port.o $(XHCI)/registers.o $(XHCI)/ring.o $(XHCI)/trb.o $(XHCI)/xhci.o $(CLSD)/base.o $(CLSD)/hid.o $(CLSD)/keyboard.o $(CLSD)/mouse.o

OBJD=$(OBJSG) $(OBJSUSB) $(PCI)/pci.o kernel/logger.o kernel/device/device.o

OBJS=$(HLP) $(OBJD)

CPPFLAG=$(OPT) $(LIB) $(DEF)
LDFLAG =--entry KernelMain -z norelro --image-base 0x100000 --static
LDLIB=-L/home/elsie/osbook/devenv/x86_64-elf/lib -lc -lc++

.PHONY: all
all:
	clear; make cc && make run
.PHONY: run
run:
	sudo cp ~/edk2/Build/LoaderX64/DEBUG_CLANG38/X64/Loader.efi build/mnt/EFI/BOOT/BOOTX64.EFI

	sudo cp ~/sos/kernel/kernel.elf build/mnt/sys/kernel/kernel.elf

	qemu-system-x86_64 \
-s -m 1G \
-drive if=pflash,format=raw,readonly,file=/home/elsie/osbook/devenv/OVMF_CODE.fd \
-drive if=pflash,format=raw,file=/home/elsie/osbook/devenv/OVMF_VARS.fd \
-drive if=ide,index=0,media=disk,format=raw,file=build/disk.img \
-device nec-usb-xhci,id=xhci \
-device usb-mouse -device usb-kbd \
-monitor stdio

.PHONY: cc
cc: $(KTAR)
	ld.lld $(LDFLAG) -o $(KEFL) $(KTAR) $(OBJS) $(LDLIB)
$(KTAR): $(KCPP) $(OBJS)
	clang++ $(CPPFLAG) -c $(KCPP) -o $(KTAR)

%.o: %.cpp Makefile
	clang++ $(CPPFLAG) -c $< -o $*.o

%.o: %.c Makefile
	clang++ $(CPPFLAG) -c $< -o $*.o
