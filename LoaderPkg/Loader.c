#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/PrintLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/DiskIo2.h>
#include <Protocol/BlockIo.h>
#include <Guid/FileInfo.h>
#include "frameBufConfig.hpp"
#include "elf64.h"

typedef struct{
    UINTN buf_size;
    VOID* buf;
    UINTN map_size;
    UINTN map_key;
    UINTN descriptor_size;
    UINT32 descriptor_version;
} MemoryMap_info;

void try(EFI_STATUS,const char*,INTN);
EFI_STATUS GetMemoryMap(MemoryMap_info*);
EFI_STATUS OpenRootDir(EFI_HANDLE,EFI_FILE_PROTOCOL**);
EFI_STATUS OpenGraphicsOutputProtocol(EFI_HANDLE,EFI_GRAPHICS_OUTPUT_PROTOCOL**,UINTN*);
void SetFrameBufConfig(EFI_GRAPHICS_OUTPUT_PROTOCOL**,struct FrameBufConfig*,UINTN);
void CalcLoadAddressRange(Elf64_Ehdr*,UINT64*,UINT64*);
void CopyLoadSegment(Elf64_Ehdr*);
const CHAR16*GetMemoryTypeStr(EFI_MEMORY_TYPE);
const CHAR16*GetPixelFormatUnicode(EFI_GRAPHICS_PIXEL_FORMAT);



EFI_STATUS EFIAPI UefiMain(EFI_HANDLE image_handle,EFI_SYSTEM_TABLE* system_table){
    CHAR8 memmap_buf[4096*4];
    MemoryMap_info mmap_info={sizeof(memmap_buf),memmap_buf,0,0,0,0};
    try(GetMemoryMap(&mmap_info),__FILE__,__LINE__);
 
    // get and print display information
    UINTN gop_size=0;
    EFI_GRAPHICS_OUTPUT_PROTOCOL*gop[MAX_DISPLAY_COUNT];
    struct FrameBufConfig fbc[MAX_DISPLAY_COUNT];
    try(OpenGraphicsOutputProtocol(image_handle,gop,&gop_size),__FILE__,__LINE__);
    SetFrameBufConfig(gop,fbc,gop_size);

    // Open root dir to open kernel.elf
    // Open kernel.elf to load at base_addr
    EFI_FILE_PROTOCOL*root_dir,*kernel_file;
    try(OpenRootDir(image_handle,&root_dir),__FILE__,__LINE__);
    try(root_dir->Open(root_dir,&kernel_file,L"\\sys\\kernel\\kernel.elf",EFI_FILE_MODE_READ,0),__FILE__,__LINE__);

    // get file size of kernel.elf to load
    UINTN file_info_size=sizeof(EFI_FILE_INFO)+sizeof(CHAR16)*16;
    UINT8 file_info_buf[file_info_size];
    try(kernel_file->GetInfo(kernel_file,&gEfiFileInfoGuid,&file_info_size,file_info_buf),__FILE__,__LINE__);
    EFI_FILE_INFO*file_info=(EFI_FILE_INFO*)file_info_buf;
    UINTN kernel_file_size=file_info->FileSize;
    // write kernel file to buffer
    VOID* kbuf;
    try(gBS->AllocatePool(EfiLoaderData,kernel_file_size,&kbuf),__FILE__,__LINE__);
    try(kernel_file->Read(kernel_file,&kernel_file_size,kbuf),__FILE__,__LINE__);
    Elf64_Ehdr*k_ehdr=(Elf64_Ehdr*)kbuf;
    UINT64 k_1st_addr,k_last_addr; // [1st, last)
    CalcLoadAddressRange(k_ehdr,&k_1st_addr,&k_last_addr);
    // set kernel on memory
    const UINTN page_shift=12, page_size=1u<<page_shift;
    const UINTN ceil_page_size=(k_last_addr-k_1st_addr+0xfff)/page_size;
    try(gBS->AllocatePages(AllocateAddress,EfiLoaderData,ceil_page_size,&k_1st_addr),__FILE__,__LINE__);
    CopyLoadSegment(k_ehdr);
    Print(L"[Boot Loader] Kernel: 0x%0lx - 0x%0lx\n",k_1st_addr,k_last_addr);
    kernel_file->Close(kernel_file);
    try(gBS->FreePool(kbuf),__FILE__,__LINE__);
 
    // call entry point function which was loaded at kernel_base_addr(0x10'0000) previous.
    UINTN entry_addr=*(UINT64*)(k_1st_addr+24);
    typedef void EntryPointType(const UINT64,const struct FrameBufConfig*);
    EntryPointType* entry_point=(EntryPointType*)entry_addr;
    Print(L"[Boot Loader] Entry Point Address: 0x%lx\n",entry_point);

    // exit boot service
    EFI_STATUS status=gBS->ExitBootServices(image_handle,mmap_info.map_key);
    if(EFI_ERROR(status)){
        try(GetMemoryMap(&mmap_info),__FILE__,__LINE__);
        try(gBS->ExitBootServices(image_handle,mmap_info.map_key),__FILE__,__LINE__);
    }

    entry_point((UINT64)gop_size,&fbc[0]);
 
    return EFI_SUCCESS;
}

void try(EFI_STATUS status,const char*file,INTN line){
    if(status==EFI_SUCCESS)return;
    CHAR16 f16[1024];
    AsciiStrToUnicodeStrS(file,f16,1024);
    Print(L"[Boot Loader] : Failed at %s : %ld :// %r\n",f16,line,status);
    while(1);
}

EFI_STATUS GetMemoryMap(MemoryMap_info*map_info){
    if(map_info->buf==NULL) return EFI_BUFFER_TOO_SMALL;
    map_info->map_size=map_info->buf_size;
    return gBS->GetMemoryMap(
        &map_info->map_size,
        (EFI_MEMORY_DESCRIPTOR*)map_info->buf,
        &map_info->map_key,
        &map_info->descriptor_size,
        &map_info->descriptor_version);
}

EFI_STATUS OpenRootDir(EFI_HANDLE image_handle,EFI_FILE_PROTOCOL**root){
    EFI_LOADED_IMAGE_PROTOCOL*loaded_img;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL*fs;
    try(gBS->OpenProtocol(image_handle,&gEfiLoadedImageProtocolGuid,
        (VOID**)&loaded_img,image_handle,NULL,EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL),
        __FILE__,__LINE__);
    try(gBS->OpenProtocol(loaded_img->DeviceHandle,&gEfiSimpleFileSystemProtocolGuid,
        (VOID**)&fs,image_handle,NULL,EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL),
        __FILE__,__LINE__);
    try(fs->OpenVolume(fs,root),__FILE__,__LINE__);
    return EFI_SUCCESS;
}

EFI_STATUS OpenGraphicsOutputProtocol(EFI_HANDLE image_handle,EFI_GRAPHICS_OUTPUT_PROTOCOL**gop,UINTN*gop_size){
    UINTN num_gop_handles=0;
    EFI_HANDLE*gop_handles=NULL;
    try(gBS->LocateHandleBuffer(ByProtocol,&gEfiGraphicsOutputProtocolGuid,NULL,&num_gop_handles,&gop_handles),__FILE__,__LINE__);
    *gop_size=// max(num_gop_handles,MAX_DISPLAY_COUNT);
        num_gop_handles>MAX_DISPLAY_COUNT?
            MAX_DISPLAY_COUNT:num_gop_handles;
    for(UINTN i=0;i<*gop_size;++i){
        try(gBS->OpenProtocol(gop_handles[i],
            &gEfiGraphicsOutputProtocolGuid,
            (VOID**)(gop+i),
            image_handle,
            NULL,
            EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL),
            __FILE__,__LINE__);
    }
    // if exists unused pointer, set NULL
    for(UINTN i=*gop_size;i<MAX_DISPLAY_COUNT;++i)
        gop[i]=NULL;
    try(gBS->FreePool(gop_handles),__FILE__,__LINE__);
    return EFI_SUCCESS;
}

void SetFrameBufConfig(EFI_GRAPHICS_OUTPUT_PROTOCOL**gop,struct FrameBufConfig*fbc,UINTN gop_size){
    Print(L"The number of Display is %u.\n",gop_size);
    for(UINTN i=0;i<gop_size;++i){
    }
    for(UINTN i=0;i<MAX_DISPLAY_COUNT;++i){
        if(i<gop_size){
            fbc[i].frame_buf=(UINT8*)gop[i]->Mode->FrameBufferBase;
            EFI_GRAPHICS_OUTPUT_MODE_INFORMATION*info=gop[i]->Mode->Info;
            fbc[i].linesize=info->PixelsPerScanLine;
            fbc[i].hres=info->HorizontalResolution;
            fbc[i].vres=info->VerticalResolution;
            Print(L"Display %u, addr:0x%lx Resolution: %ux%u, fmt: %s, %u pixels/line\n",fbc[i].frame_buf,i+1,fbc[i].hres,fbc[i].vres,GetPixelFormatUnicode(info->PixelFormat),fbc[i].linesize);
            switch((UINTN)(info->PixelFormat)){
                case PixelRedGreenBlueReserved8BitPerColor:
                    fbc[i].fmt=kRGB_8bitPerColor; break;
                case PixelBlueGreenRedReserved8BitPerColor:
                    fbc[i].fmt=kBGR_8bitPerColor; break;
                case PixelBitMask: fbc[i].fmt=kBitmask; break;
                case PixelBltOnly: fbc[i].fmt=kBltOnly; break;
            }
        }else{ // if display is less than limit
            fbc[i].frame_buf=NULL;
            fbc[i].linesize=fbc[i].hres=fbc[i].vres=0;
            fbc[i].fmt=kRGB_8bitPerColor;
        }
    }
}

void CalcLoadAddressRange(Elf64_Ehdr*ehdr,UINT64*first,UINT64*last){
    Elf64_Phdr*phdr=(Elf64_Phdr*)((UINT64)ehdr+ehdr->e_phoff);
    *first=MAX_UINT64;
    *last=0;
    for(Elf64_Half i=0;i<ehdr->e_phnum;++i){
        if(phdr[i].p_type!=PT_LOAD) continue;
        *first=*first<phdr[i].p_vaddr?*first:phdr[i].p_vaddr; // min
        *last=*last>phdr[i].p_vaddr+phdr[i].p_memsz?*last:phdr[i].p_vaddr+phdr[i].p_memsz; // max
    }
}

void CopyLoadSegment(Elf64_Ehdr*ehdr){
    Elf64_Phdr*phdr=(Elf64_Phdr*)((UINT64)ehdr+ehdr->e_phoff);
    for(Elf64_Half i=0;i<ehdr->e_phnum;++i){
        if(phdr[i].p_type!=PT_LOAD)continue;
        UINT64 segm_in_file=(UINT64)ehdr+phdr[i].p_offset;
        CopyMem((VOID*)phdr[i].p_vaddr,(VOID*)segm_in_file,phdr[i].p_filesz);
        UINT64 rem_bytes=phdr[i].p_memsz-phdr[i].p_filesz;
        SetMem((VOID*)(phdr[i].p_vaddr+phdr[i].p_filesz),rem_bytes,0);
    }
}

const CHAR16* GetMemoryTypeStr(EFI_MEMORY_TYPE type) {
    switch(type){
        case EfiReservedMemoryType: return L"EfiReservedMemoryType";
        case EfiLoaderCode: return L"EfiLoaderCode";
        case EfiLoaderData: return L"EfiLoaderData";
        case EfiBootServicesCode: return L"EfiBootServicesCode";
        case EfiBootServicesData: return L"EfiBootServicesData";
        case EfiRuntimeServicesCode: return L"EfiRuntimeServicesCode";
        case EfiRuntimeServicesData: return L"EfiRuntimeServicesData";
        case EfiConventionalMemory: return L"EfiConventionalMemory";
        case EfiUnusableMemory: return L"EfiUnusableMemory";
        case EfiACPIReclaimMemory: return L"EfiACPIReclaimMemory";
        case EfiACPIMemoryNVS: return L"EfiACPIMemoryNVS";
        case EfiMemoryMappedIO: return L"EfiMemoryMappedIO";
        case EfiMemoryMappedIOPortSpace: return L"EfiMemoryMappedIOPortSpace";
        case EfiPalCode: return L"EfiPalCode";
        case EfiPersistentMemory: return L"EfiPersistentMemory";
        case EfiMaxMemoryType: return L"EfiMaxMemoryType";
        default: return L"InvalidMemoryType";
    }
}

const CHAR16*GetPixelFormatUnicode(EFI_GRAPHICS_PIXEL_FORMAT fmt){
    switch (fmt) {
        case PixelRedGreenBlueReserved8BitPerColor:
          return L"PixelRedGreenBlueReserved8BitPerColor";
        case PixelBlueGreenRedReserved8BitPerColor:
          return L"PixelBlueGreenRedReserved8BitPerColor";
        case PixelBitMask:
          return L"PixelBitMask";
        case PixelBltOnly:
          return L"PixelBltOnly";
        case PixelFormatMax:
          return L"PixelFormatMax";
        default:
          return L"InvalidPixelFormat";
    }
}