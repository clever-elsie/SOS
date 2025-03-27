
OPT=-target x86_64-elf -mno-red-zone -ffreestanding -fno-exceptions -fno-rtti -nostdlibinc -std=c++17 -Wall -g -O2 
LIB=-I ~/sos/LoaderPkg -I ~/osbook/devenv/x86_64-elf/include/c++/v1 -I ~/osbook/devenv/x86_64-elf/include 
DEF=-D__ELF__ -D_LDBL_EQ_DBL -D_GNU_SOURCE -D_POSIX_TIMERS

KTAR=kernel/main.o
KCPP=kernel/kernel.cpp
KEFL=kernel/kernel.elf
GPX=kernel/graphics
PCI=kernel/pci
OBJS=kernel/include/newlib_helper.o $(GPX)/console.o $(PCI)/pci.o $(PCI)/error.o

CPPFLAG=$(OPT) $(LIB) $(DEF)
LDFLAG =--entry KernelMain -z norelro --image-base 0x100000 --static
LDLIB=-L/home/elsie/osbook/devenv/x86_64-elf/lib -lc

.PHONY: run
run:
	sudo cp ~/edk2/Build/LoaderX64/DEBUG_CLANG38/X64/Loader.efi build/mnt/EFI/BOOT/BOOTX64.EFI
	sudo cp ~/sos/kernel/kernel.elf build/mnt/sys/kernel/kernel.elf
	qemu-system-x86_64 -m 1G -drive if=pflash,format=raw,readonly,file=/home/elsie/osbook/devenv/OVMF_CODE.fd -drive if=pflash,format=raw,file=/home/elsie/osbook/devenv/OVMF_VARS.fd -drive if=ide,index=0,media=disk,format=raw,file=build/disk.img -device nec-usb-xhci,id=xhci -device usb-mouse -device usb-kbd -monitor stdio 

.PHONY: cc
cc: $(KTAR)
	ld.lld $(LDFLAG) -o $(KEFL) $(KTAR) $(OBJS) $(LDLIB)
$(KTAR): $(KCPP) $(OBJS)
	clang++ $(CPPFLAG) -c $(KCPP) -o $(KTAR)

%.o: %.cpp Makefile
	clang++ $(CPPFLAG) -c $< -o $*.o

%.o: %.c Makefile
	clang++ $(CPPFLAG) -c $< -o $*.o