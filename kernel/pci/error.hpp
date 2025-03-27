#ifndef SOS_KERNEL_PCI_ERROR
#define SOS_KERNEL_PCI_ERROR
#include <array>
#include <cstdint>

namespace pci{
struct Error{
    enum class Code{ success, full, empty };
    Error(Code code):v(code){}
    operator bool()const{ return v!=Code::success;}
    const char*Name()const{ return name[static_cast<int32_t>(v)]; }
    private:
    static constexpr std::array<const char*,3>name={"success","full","empty"};
    Code v;
};
}
#endif