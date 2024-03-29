#include "BedrockMemory.hpp"

#include "BedrockCommon.hpp"

#include <cstdlib>

namespace MFA::Memory {

Blob Alloc (size_t const size) {
    return {static_cast<uint8_t *>(::malloc(size)), size};
}

void Free (Blob const & mem) {
    if (mem.ptr != nullptr) {
        ::free(mem.ptr);
    }
}

void PtrFree (void * ptr) {
    if (ptr != nullptr) {
        ::free(ptr);
    }
}

}
