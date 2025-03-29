#include <cstdint>
#include <cstddef>
#include <climits>
#include <type_traits>
#include <utility>
#include <cstdio>

#include <vector>
#include <numeric>
#include "device.hpp"

extern "C" void KernelMain(const uint64_t display_count,const FrameBufConfig*fbc){
    setup_device(display_count,fbc);
    while(1)__asm__("hlt");
}

