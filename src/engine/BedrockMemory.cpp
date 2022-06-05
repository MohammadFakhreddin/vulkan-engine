#include "BedrockMemory.hpp"

#include <cstdlib>

namespace MFA::Memory {

    std::shared_ptr<SmartBlob> Alloc (size_t const size) {
        return std::make_shared<SmartBlob>(Blob {
            static_cast<uint8_t *>(::malloc(size)),
            size
        });
    }

    static void Free (Blob const & mem) {
        if (mem.ptr != nullptr) {
            ::free(mem.ptr);
        }
    }

}

MFA::SmartBlob::SmartBlob(Blob memory_)
    : memory(memory_)
{}

MFA::SmartBlob::~SmartBlob()
{
    Memory::Free(memory);
}
