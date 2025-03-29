#ifndef SOS_KERNEL_ERROR
#define SOS_KERNEL_ERROR
#include <array>
#include <cstdint>

struct Error{
    enum Code{
        kSuccess,
        kFull,
        kEmpty,
        kNoEnoughMemory,
        kIndexOutOfRange,
        kHostControllerNotHalted,
        kInvalidSlotID,
        kPortNotConnected,
        kInvalidEndpointNumber,
        kTransferRingNotSet,
        kAlreadyAllocated,
        kNotImplemented,
        kInvalidDescriptor,
        kBufferTooSmall,
        kUnknownDevice,
        kNoCorrespondingSetupStage,
        kTransferFailed,
        kInvalidPhase,
        kUnknownXHCISpeedID,
        kNoWaiter,
        kLastOfCode,
    };
    Error(Code v_,const char*file_,int32_t line_):v(v_),file(file_),line(line_){}
    Code Cause()const{ return v; }
    operator bool()const{return v!=kSuccess;}
    const char*Name()const{ return code_names_[static_cast<int32_t>(v)]; }
    const char*File()const{ return file;}
    int32_t Line()const{return line;}
    private:
    static constexpr std::array code_names_{
      "kSuccess",
      "kFull",
      "kEmpty",
      "kNoEnoughMemory",
      "kIndexOutOfRange",
      "kHostControllerNotHalted",
      "kInvalidSlotID",
      "kPortNotConnected",
      "kInvalidEndpointNumber",
      "kTransferRingNotSet",
      "kAlreadyAllocated",
      "kNotImplemented",
      "kInvalidDescriptor",
      "kBufferTooSmall",
      "kUnknownDevice",
      "kNoCorrespondingSetupStage",
      "kTransferFailed",
      "kInvalidPhase",
      "kUnknownXHCISpeedID",
      "kNoWaiter",
    };
    Code v;
    const char*file;
    int32_t line;
};

template<class T>
struct withError{
    T value;
    Error error;
};
#define MAKE_ERROR(code) Error((code),__FILE__,__LINE__)
#endif