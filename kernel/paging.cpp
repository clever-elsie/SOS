#include "paging.hpp"

#include <array>

extern "C" uint16_t getCodeSegment(void);
extern "C" void LoadIDT(uint16_t limit, uint64_t offset);
extern "C" void LoadGDT(uint16_t limit, uint64_t offset);
extern "C" void SetCSSS(uint16_t cs, uint16_t ss);
extern "C" void SetDSAll(uint16_t value);
extern "C" void SetCR3(uint64_t value);

namespace {
  const uint64_t kPageSize4K = 4096;
  const uint64_t kPageSize2M = 512 * kPageSize4K;
  const uint64_t kPageSize1G = 512 * kPageSize2M;

  alignas(kPageSize4K) std::array<uint64_t, 512> pml4_table;
  alignas(kPageSize4K) std::array<uint64_t, 512> pdp_table;
  alignas(kPageSize4K)
    std::array<std::array<uint64_t, 512>, kPageDirectoryCount> page_directory;
}

void SetupIdentityPageTable() {
  pml4_table[0] = reinterpret_cast<uint64_t>(&pdp_table[0]) | 0x003;
  for (int i_pdpt = 0; i_pdpt < page_directory.size(); ++i_pdpt) {
    pdp_table[i_pdpt] = reinterpret_cast<uint64_t>(&page_directory[i_pdpt]) | 0x003;
    for (int i_pd = 0; i_pd < 512; ++i_pd) {
      page_directory[i_pdpt][i_pd] = i_pdpt * kPageSize1G + i_pd * kPageSize2M | 0x083;
    }
  }

  SetCR3(reinterpret_cast<uint64_t>(&pml4_table[0]));
}
