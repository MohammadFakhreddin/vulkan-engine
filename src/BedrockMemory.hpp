#ifndef MEMORY_NAMESPACE
#define MEMORY_NAMESPACE

// TODO Implement memory system
#if 0 // It needs subsystem which currently is unavailable

// TODO Some functions here might be unused, It may need some cleanup

#include "BedrockCommon.hpp"

#include <new>                  // for placement "new"
#include <utility>

#define MFA_PLACEMENT_NEW(p_, T_, ...)        new (p_) T_(__VA_ARGS__)
#define MFA_PLACEMENT_NEW_ARRAY(p_, T_, N_)   new (p_) T_ [N_]

#define MFA_ALLOC(allocator_, sys_, size_)    allocator_->alloc(size_, __FILE__, __LINE__, __FUNCTION__, sys_)
#define MFA_FREE(alloctor_, blob_)            allocator_->dealloc(blob_,  __FILE__, __LINE__, __FUNCTION__, sys_  )

#define MFA_ALLOC_GP(sys_, size_)             ::MFA::Mem_ActualAlloc(size_, __FILE__, __LINE__, __FUNCTION__, sys_)
#define MFA_FREE_GP(sys_, blob_)              ::MFA::Mem_ActualFree(blob_, __FILE__, __LINE__, __FUNCTION__, sys_)

#define MFA_STACK_ALLOC(type_, size_)         reinterpret_cast<type_*>(::alloca(size_))
#define MFA_STACK_FREE(ptr_)                  /**/

#define MFA_GPALLOC_ENUMERATE_ALL_ALLOCATED_BLOCKS(do_print_) ::MFA::Mem_GPAllocator_EnumerateAllBlocks_DEBUG(do_print_)
#endif
#include <type_traits>

#include "BedrockCommon.hpp"

namespace MFA::Memory {

Blob Alloc (size_t size);

void Free (Blob const & mem);

template <typename T>
T * New_NoConstruct () {
    auto blob = Alloc(sizeof(T));
    return blob.as<T>();
}
template <typename T>
void Delete_NoDestruct (T * ptr) {
    return Free({ptr, sizeof(T)});
}
template <typename T>
TBlob<T> NewArray_NoConstruct (size_t count) {
    auto blob = Alloc(sizeof(T) * count);
    return {blob.as<void>(), blob.len};
}
template <typename T>
void DeleteArray_NoDestruct (TBlob<T> blob) {
    Free({blob.ptr, blob.len * sizeof(T)});
}

template <typename T, typename ... ArgTypes>
T * New (ArgTypes && ... args) {
    auto ret = New_NoConstruct<T>();
    if (ret) {
        try {
            new (ret) T (std::forward<ArgTypes>(args)...);
        } catch (...) {
            ret->~T();
            Delete_NoDestruct(ret);
            ret = nullptr;
        }
    }
    return ret;
}
template <typename T>
void Delete (T * ptr) {
    if (ptr) {
        ptr->~T();
        Delete_NoDestruct(ptr);
    }
}

}

#endif