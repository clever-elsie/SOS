#include <sys/types.h>
#include <errno.h>
#include <new>

std::new_handler std::get_new_handler() noexcept {
  return nullptr;
}

extern "C" int posix_memalign(void**, size_t, size_t) {
  return ENOMEM;
}

extern "C" void _exit(void) {
  while (1) __asm__("hlt");
}

extern "C" int getpid(void) {
  return 1;
}

extern "C" int kill(int pid, int sig) {
  errno = EINVAL;
  return -1;
}


extern "C" caddr_t sbrk(int incl){return NULL;}

extern "C" void __cxa_pure_virtual() {
  while (1) __asm__("hlt");
}