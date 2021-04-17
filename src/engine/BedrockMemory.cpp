#include "BedrockMemory.hpp"

#include "BedrockCommon.hpp"

#include <cstdlib>

namespace MFA::Memory {

Blob Alloc (size_t const size) {
    return {static_cast<Byte *>(::malloc(size)), size};
}

void Free (Blob const & mem) {
    ::free(mem.ptr);
}

void PtrFree (void * ptr) {
    ::free(ptr);
}

}
