#include "memory_manager.hpp"

BitmapMemoryManager::BitmapMemoryManager()
  : alloc_map_{}, range_begin_{FrameID{0}}, range_end_{FrameID{kFrameCount}} {
}

withError<FrameID> BitmapMemoryManager::Allocate(size_t num_frames) {
  for(size_t first=range_begin_.ID();1;++first){
    size_t fI=first/kBitsPerMapLine;
    size_t fi=first%kBitsPerMapLine;
    if(alloc_map_[fI]&MapLineType(1)<<fi){
      ++first;
      continue;
    }
    size_t head=0;
    bool valid=true;
    for(;head+fi<kBitsPerMapLine;++head) // 最初のframe区画
      if(first+head>=range_end_.ID()) return {kNullFrame,MAKE_ERROR(Error::kNoEnoughMemory)};
      else if(alloc_map_[fI]&MapLineType(1)<<(head+fi)){
        first+=head+1;
        valid=false;
        break;
      }else if(head+1==num_frames){
        MarkAllocated(FrameID{first},num_frames);
        return {FrameID{first},MAKE_ERROR(Error::kSuccess)};
      }
    if(!valid)continue;
    size_t rem=num_frames-head,i=fI+1;
    while(rem>=64){  // 定数倍高速化のためのull省略
      if((i+1)*kBitsPerMapLine>=range_end_.ID()) return {kNullFrame,MAKE_ERROR(Error::kNoEnoughMemory)};
      if(alloc_map_[i]){
        valid=false; // 必要量が64bit以上の区間を持っているのでここのMSBの次から初めてOK
        first=(i*kBitsPerMapLine)+(kBitsPerMapLine-__builtin_clzl(alloc_map_[i])+1);
        break;
      }else rem-=64, ++i;
    }
    if(!valid)continue;
    size_t cur=i*kBitsPerMapLine;
    for(int s=0;rem;++s,--rem) // 残り
      if(cur+s>=range_end_.ID()) return {kNullFrame,MAKE_ERROR(Error::kNoEnoughMemory)};
      else if(alloc_map_[i]&MapLineType(1)<<s){
        valid=false;
        first=i*kBitsPerMapLine+s+1;
        break;
      }
    if(!valid)continue;
    MarkAllocated(FrameID{first},num_frames);
    return{FrameID{first},MAKE_ERROR(Error::kSuccess)};
  }
  /*
  size_t start_frame_id = range_begin_.ID();
  while (true) {
    size_t i = 0;
    for (; i < num_frames; ++i) {
      if (start_frame_id + i >= range_end_.ID()) {
        return {kNullFrame, MAKE_ERROR(Error::kNoEnoughMemory)};
      }
      if (GetBit(FrameID{start_frame_id + i})) {
        // "start_frame_id + i" にあるフレームは割り当て済み
        break;
      }
    }
    if (i == num_frames) {
      // num_frames 分の空きが見つかった
      MarkAllocated(FrameID{start_frame_id}, num_frames);
      return {
        FrameID{start_frame_id},
        MAKE_ERROR(Error::kSuccess),
      };
    }
    // 次のフレームから再検索
    start_frame_id += i + 1;
  }
  */
}

Error BitmapMemoryManager::Free(FrameID start_frame, size_t num_frames) {
  for (size_t i = 0; i < num_frames; ++i) {
    SetBit(FrameID{start_frame.ID() + i}, false);
  }
  return MAKE_ERROR(Error::kSuccess);
}

void BitmapMemoryManager::MarkAllocated(FrameID start_frame, size_t num_frames) {
  for (size_t i = 0; i < num_frames; ++i) {
    SetBit(FrameID{start_frame.ID() + i}, true);
  }
}

void BitmapMemoryManager::SetMemoryRange(FrameID range_begin, FrameID range_end) {
  range_begin_ = range_begin;
  range_end_ = range_end;
}

bool BitmapMemoryManager::GetBit(FrameID frame) const {
  auto line_index = frame.ID() / kBitsPerMapLine;
  auto bit_index = frame.ID() % kBitsPerMapLine;

  return (alloc_map_[line_index] & (static_cast<MapLineType>(1) << bit_index)) != 0;
}

void BitmapMemoryManager::SetBit(FrameID frame, bool allocated) {
  auto line_index = frame.ID() / kBitsPerMapLine;
  auto bit_index = frame.ID() % kBitsPerMapLine;

  if (allocated) {
    alloc_map_[line_index] |= (static_cast<MapLineType>(1) << bit_index);
  } else {
    alloc_map_[line_index] &= ~(static_cast<MapLineType>(1) << bit_index);
  }
}
