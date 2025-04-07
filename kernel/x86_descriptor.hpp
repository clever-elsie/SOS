#ifndef SOS_KERNEL_X86_DESCRIPTOR_TYPE
#define SOS_KERNEL_X86_DESCRIPTOR_TYPE
enum class DescriptorType {
  // system segment & gate descriptor types
  kUpper8Bytes   = 0,
  kLDT           = 2,
  kTSSAvailable  = 9,
  kTSSBusy       = 11,
  kCallGate      = 12,
  kInterruptGate = 14,
  kTrapGate      = 15,

  // code & data segment types
  kReadWrite     = 2,
  kExecuteRead   = 10,
};
#endif