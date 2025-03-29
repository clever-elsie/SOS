#ifndef SOS_KERNEL_PLACEMENT_NEW
#define SOS_KERNEL_PLACEMENT_NEW
#include <cstddef>

template<class T> void* operator new(size_t size,T*buf){return buf;}
void operator delete(void*buf)noexcept;

#endif