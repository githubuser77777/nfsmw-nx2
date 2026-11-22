#include <rex/platform.h>

#if defined(__SWITCH2__)

#include <cstdlib>
#include <cstring>
#include <map>
#include <mutex>
#include <malloc.h>

#include <rex/logging.h>
#include <rex/memory/utils.h>

namespace rex::memory {
namespace {

constexpr size_t kPageSize = 0x1000;
constexpr size_t kAlignment = 0x10000;
constexpr FileMappingHandle kArenaHandle = 1;
std::mutex g_mutex;
uint8_t* g_arena = nullptr;
size_t g_arena_size = 0;
std::map<uintptr_t, size_t> g_blocks;

bool Contains(uintptr_t start, size_t size, uintptr_t address, size_t length) {
  return address >= start && address - start <= size && length <= size - (address - start);
}

bool InAllocation(uintptr_t address, size_t length) {
  if (g_arena && Contains(reinterpret_cast<uintptr_t>(g_arena), g_arena_size, address, length)) {
    return true;
  }
  auto it = g_blocks.upper_bound(address);
  if (it == g_blocks.begin()) return false;
  --it;
  return Contains(it->first, it->second, address, length);
}

}  // namespace

size_t page_size() { return kPageSize; }
size_t allocation_granularity() { return kAlignment; }
bool IsWritableExecutableMemorySupported() { return false; }

void* GetFileMappingBase(FileMappingHandle handle) {
  std::lock_guard<std::mutex> lock(g_mutex);
  return handle == kArenaHandle ? g_arena : nullptr;
}

FileMappingHandle CreateFileMappingHandle(const std::filesystem::path&, size_t length,
                                          PageAccess, bool) {
  std::lock_guard<std::mutex> lock(g_mutex);
  if (g_arena || !length || length > SIZE_MAX - (kAlignment - 1)) {
    REXLOG_ERROR("Switch 2 guest arena: invalid size or arena already exists");
    return kFileMappingHandleInvalid;
  }
  const size_t size = (length + kAlignment - 1) & ~(kAlignment - 1);
  g_arena = static_cast<uint8_t*>(memalign(kAlignment, size));
  if (!g_arena) {
    REXLOG_ERROR("Switch 2 guest arena: could not allocate {} bytes", size);
    return kFileMappingHandleInvalid;
  }
  g_arena_size = size;
  std::memset(g_arena, 0, size);
  REXLOG_WARN("Switch 2 experimental guest memory: host page protection and write watches are not available");
  return kArenaHandle;
}

void CloseFileMappingHandle(FileMappingHandle handle, const std::filesystem::path&) {
  std::lock_guard<std::mutex> lock(g_mutex);
  if (handle != kArenaHandle || !g_arena) {
    REXLOG_ERROR("Switch 2 guest arena: invalid handle on close");
    return;
  }
  std::free(g_arena);
  g_arena = nullptr;
  g_arena_size = 0;
}

void* AllocFixed(void* base, size_t length, AllocationType allocation_type, PageAccess) {
  if (!length) {
    REXLOG_ERROR("Switch 2 AllocFixed: empty allocation");
    return nullptr;
  }
  std::lock_guard<std::mutex> lock(g_mutex);
  if (base) {
    if (InAllocation(reinterpret_cast<uintptr_t>(base), length)) return base;
    REXLOG_ERROR("Switch 2 AllocFixed: address outside the arena or allocated blocks");
    return nullptr;
  }
  if (length > SIZE_MAX - (kAlignment - 1)) {
    REXLOG_ERROR("Switch 2 AllocFixed: allocation size overflow");
    return nullptr;
  }
  const size_t size = (length + kAlignment - 1) & ~(kAlignment - 1);
  void* block = memalign(kAlignment, size);
  if (!block) {
    REXLOG_ERROR("Switch 2 AllocFixed: could not allocate {} bytes", size);
    return nullptr;
  }
  if (allocation_type != AllocationType::kReserve) std::memset(block, 0, size);
  g_blocks.emplace(reinterpret_cast<uintptr_t>(block), size);
  return block;
}

bool DeallocFixed(void* base, size_t length, DeallocationType type) {
  std::lock_guard<std::mutex> lock(g_mutex);
  const uintptr_t address = reinterpret_cast<uintptr_t>(base);
  if (g_arena && Contains(reinterpret_cast<uintptr_t>(g_arena), g_arena_size, address, length)) {
    return true;
  }
  auto it = g_blocks.find(address);
  if (it == g_blocks.end()) {
    REXLOG_ERROR("Switch 2 DeallocFixed: unknown allocation");
    return false;
  }
  if (type == DeallocationType::kRelease) {
    std::free(base);
    g_blocks.erase(it);
  }
  return true;
}

bool Protect(void* base, size_t length, PageAccess access, PageAccess* old_access) {
  std::lock_guard<std::mutex> lock(g_mutex);
  if (!InAllocation(reinterpret_cast<uintptr_t>(base), length)) {
    REXLOG_ERROR("Switch 2 Protect: address outside an allocation");
    return false;
  }
  if (old_access) *old_access = PageAccess::kReadWrite;
  if (access != PageAccess::kReadWrite) {
    REXLOG_ERROR("Switch 2 Protect: restrictive host page permissions are unsupported");
    return false;
  }
  return true;
}

bool QueryProtect(void* base, size_t& length, PageAccess& access) {
  std::lock_guard<std::mutex> lock(g_mutex);
  if (!InAllocation(reinterpret_cast<uintptr_t>(base), 1)) {
    length = 0;
    access = PageAccess::kNoAccess;
    return false;
  }
  access = PageAccess::kReadWrite;
  return true;
}

void* MapFileView(FileMappingHandle, void*, size_t, PageAccess, size_t) {
  REXLOG_ERROR("Switch 2: file views are not supported by the folded guest arena");
  return nullptr;
}
bool UnmapFileView(FileMappingHandle, void*, size_t) {
  REXLOG_ERROR("Switch 2: file views are not supported by the folded guest arena");
  return false;
}

}  // namespace rex::memory
#endif
