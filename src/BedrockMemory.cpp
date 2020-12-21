// TODO Implement memory system
#if 0
#include <interface/bedrock_memory.hpp>
#include <interface/bedrock_assert.hpp>
#include <interface/bedrock_subsystem.hpp>
#include <intrin.h>
#include <cstdlib>
#include <cstring>
#include <cstdio>

namespace Yugen {


YUGEN_ENGINE_API Blob
Mem_Alloc (size_t size) {
    return {static_cast<Byte *>(::malloc(size)), size};
}

//YUGEN_ENGINE_API Blob
//Mem_AllocOnStack (size_t size) {
//    return {static_cast<Byte *>(_malloca(size)), size};
//}

YUGEN_ENGINE_API void
Mem_Free (Blob const & mem) {
    ::free(mem.ptr);
}

//YUGEN_ENGINE_API void
//Mem_FreeOnStack (Blob const & mem) {
//
//}

YUGEN_ENGINE_API Blob
Mem_Duplicate (CBlob const & src) {
    Blob ret = {};
    if (YUGEN_PTR_VALID(src.ptr) && src.len > 0) {
        ret = Mem_Alloc(src.len);
        if (YUGEN_PTR_VALID(ret.ptr) && ret.len >= src.len) {
            ::memcpy(ret.ptr, src.ptr, src.len);
        } else {
            Mem_Free(ret);
            ret = {};
        }
    }
    return ret;
}

YUGEN_ENGINE_API Blob
Mem_Realloc (Blob const & mem, size_t new_size) {
    Blob ret = {};
    ret.ptr = static_cast<Byte *>(::realloc(mem.ptr, new_size));
    if (ret.ptr)
        ret.len = new_size;
    return ret;
}

using MemCopyFn = void * (void *, void const *, size_t);
static MemCopyFn * g_memcpy_small_ptr = Internal::MemCopy_Generic1;
static MemCopyFn * g_memcpy_medium_ptr = Internal::MemCopy_SSE8;
static MemCopyFn * g_memcpy_large_ptr = Internal::MemCopy_SSE2_NT;

YUGEN_ENGINE_API bool
Mem_Equal (CBlob const & mem1, CBlob const & mem2) {
    //bool ret = false;
    //if (mem1.len == mem2.len) {
    //    ret = (0 == ::memcmp(mem1.ptr, mem2.ptr, mem1.len));
    //}
    //return ret;
    return 
        (mem1.len == mem2.len) &&
        (0 == ::memcmp(mem1.ptr, mem2.ptr, mem1.len));
}

YUGEN_ENGINE_API bool 
Mem_IsOverlapped (void const * mem1, size_t bytes1, void const * mem2, size_t bytes2) {
    char const * p1 = static_cast<char const *>(mem1);
    char const * p2 = static_cast<char const *>(mem2);
    return (p1 <= p2 && p1 + bytes1 > p2) || (p2 <= p1 && p2 + bytes2 > p1);
}

YUGEN_ENGINE_API void *
Mem_Copy (void * YUGEN_RESTRICT dest, void const * YUGEN_RESTRICT source, size_t size_bytes) {
    YUGEN_ASSERT(!Mem_IsOverlapped(source, size_bytes, dest, size_bytes));
    if (size_bytes <= 8192 + 64)
        return g_memcpy_small_ptr(dest, source, size_bytes);
    else if (size_bytes <= 3 * 1024 * 1024)
        return g_memcpy_medium_ptr(dest, source, size_bytes);
    else
        return g_memcpy_large_ptr(dest, source, size_bytes);
}

#pragma intrinsic(__stosb)
YUGEN_ENGINE_API void *
Mem_Fill (void * dest, unsigned char fill_byte, size_t size_bytes) {
#if 1
    auto p1 = reinterpret_cast<char *>(dest);
    for (size_t i = 0; i < size_bytes; ++i)
        p1[i] = fill_byte;
#elif 1
    __stosb(static_cast<unsigned char *>(dest), fill_byte, size_bytes);
#elif 1
    uint64_t const fill_word = 
        uint64_t(fill_byte) <<  0 | uint64_t(fill_byte) <<  8 |
        uint64_t(fill_byte) << 16 | uint64_t(fill_byte) << 24 |
        uint64_t(fill_byte) << 32 | uint64_t(fill_byte) << 40 |
        uint64_t(fill_byte) << 48 | uint64_t(fill_byte) << 56;
    __stosq(static_cast<unsigned long long *>(dest), fill_word, size_bytes / 8);
    __stosb(static_cast<unsigned char *>(dest) + size_bytes / 8, fill_byte, size_bytes % 8);
#endif
    return dest;
}

YUGEN_ENGINE_API void *
Mem_Zero (void * dest, size_t size_bytes) {
    return Mem_Fill(dest, 0, size_bytes);
}

YUGEN_ENGINE_API bool
Mem_Equal (void const * ptr1, void const * ptr2, size_t size_bytes) {
    auto p1 = reinterpret_cast<char const *>(ptr1);
    auto p2 = reinterpret_cast<char const *>(ptr2);
    for (size_t i = 0; i < size_bytes; ++i)
        if (p1[i] != p2[i])
            return false;
    return true;
}

YUGEN_ENGINE_API size_t
Str_Len (char const * str) {
    size_t ret = 0;
    if (str) {
        char const * p = str;
        while (*p)
            p++;
        ret = p - str;
    }
    return ret;
}

YUGEN_ENGINE_API size_t
Str_Length (char const * str) {
    size_t ret = 0;
    if (YUGEN_PTR_VALID(str)) {
        while (*str++)
            ret++;
    }
    return ret;
}

YUGEN_ENGINE_API int
Str_Compare (char const * s1, char const * s2) {
    if (s1 == s2)
        return 0;
    else if (!s1)
        return -1;  // TODO: Is this a good thing?
    else if (!s2)
        return 1;  // TODO: Is this a good thing?
    else {
        while (*s1 && *s2 && *s1 == *s2) {
            s1++;
            s2++;
        }
        return *s1 - *s2;
    }
}

YUGEN_ENGINE_API bool
Str_Equal (char const * s1, char const * s2) {
    return 0 == Str_Compare(s1, s2);
}

YUGEN_ENGINE_API size_t
Str_Copy (char * dst, size_t dst_size, char const * src) {
    size_t ret = 0;
    if (YUGEN_PTR_VALID(dst) && dst_size > 0) {
        if (YUGEN_PTR_VALID(src)) {
            while (ret < dst_size - 1 && src[ret]) {
                dst[ret] = src[ret];
                ret++;
            }
        }
        dst[ret++] = '\0';
    }
    return ret;
}

    namespace Internal {

YUGEN_ENGINE_API void *
MemCopy_Generic1 (void * YUGEN_RESTRICT dest, void const * YUGEN_RESTRICT source, size_t size_bytes) {
    auto p1 = reinterpret_cast<char *>(dest);
    auto p2 = reinterpret_cast<char const *>(source);
    for (size_t i = 0; i < size_bytes; ++i)
        p1[i] = p2[i];
    return dest;
}
YUGEN_ENGINE_API void *
MemCopy_Generic2 (void * YUGEN_RESTRICT dest, void const * YUGEN_RESTRICT source, size_t size_bytes) {
    using W = unsigned long long;
    auto p1 = reinterpret_cast<W *>(dest);
    auto p2 = reinterpret_cast<W const *>(source);
    auto const words = size_bytes / sizeof(W);
    auto const bytes = size_bytes % sizeof(W);
    for (size_t i = 0; i < words; ++i)
        *p1++ = *p2++;
    auto q1 = reinterpret_cast<char *>(p1);
    auto q2 = reinterpret_cast<char const *>(p2);
    for (size_t i = 0; i < bytes; ++i)
        *q1++ = *q2++;
    return dest;
}
#pragma intrinsic(__movsb)
YUGEN_ENGINE_API void *
MemCopy_Assembly1 (void * YUGEN_RESTRICT dest, void const * YUGEN_RESTRICT source, size_t size_bytes) {
    __movsb((unsigned char *)dest, (unsigned char const *)source, size_bytes);
    return dest;
}
#pragma intrinsic(__movsb)
#pragma intrinsic(__movsq)
YUGEN_ENGINE_API void *
MemCopy_Assembly2 (void * YUGEN_RESTRICT dest, void const * YUGEN_RESTRICT source, size_t size_bytes) {
    auto const words = size_bytes / 8;
    auto const bytes = size_bytes % 8;
    __movsq(
        (unsigned long long *)dest,
        (unsigned long long const *)source,
        words
    );
    __movsb(
        (unsigned char *)dest + size_bytes - bytes,
        (unsigned char const *)source + size_bytes - bytes,
        bytes
    );
    return dest;
}
YUGEN_ENGINE_API void *
MemCopy_SSE1 (void * YUGEN_RESTRICT dest, void const * YUGEN_RESTRICT source, size_t size_bytes) {
    constexpr size_t Group = 1;
    constexpr size_t SIMD = sizeof(__m128) / sizeof(float);
    auto const groups = size_bytes / (Group * sizeof(__m128));
    auto const bytes = size_bytes - (groups * Group * sizeof(__m128));
    auto * p1 = reinterpret_cast<float *>(dest);
    auto const * p2 = reinterpret_cast<float const *>(source);
    for (size_t i = 0; i < groups; ++i) {
        __m128 a = _mm_load_ps(p2 + 0 * SIMD);
        _mm_store_ps(p1 + 0 * SIMD, a);
        p2 += SIMD * Group;
        p1 += SIMD * Group;
    }
    for (size_t i = 0; i < bytes; ++i)
        *((char *)dest + size_bytes - bytes + i) = *((char *)source + size_bytes - bytes + i);
    return dest;
}
YUGEN_ENGINE_API void *
MemCopy_SSE2 (void * YUGEN_RESTRICT dest, void const * YUGEN_RESTRICT source, size_t size_bytes) {
    constexpr size_t Group = 2;
    constexpr size_t SIMD = sizeof(__m128) / sizeof(float);
    auto const groups = size_bytes / (Group * sizeof(__m128));
    auto const bytes = size_bytes - (groups * Group * sizeof(__m128));
    auto * p1 = reinterpret_cast<float *>(dest);
    auto const * p2 = reinterpret_cast<float const *>(source);
    for (size_t i = 0; i < groups; ++i) {
        __m128 a = _mm_load_ps(p2 + 0 * SIMD);
        __m128 b = _mm_load_ps(p2 + 1 * SIMD);
        _mm_store_ps(p1 + 0 * SIMD, a);
        _mm_store_ps(p1 + 1 * SIMD, b);
        p2 += SIMD * Group;
        p1 += SIMD * Group;
    }
    for (size_t i = 0; i < bytes; ++i)
        *((char *)dest + size_bytes - bytes + i) = *((char *)source + size_bytes - bytes + i);
    return dest;
}
YUGEN_ENGINE_API void *
MemCopy_SSE4 (void * YUGEN_RESTRICT dest, void const * YUGEN_RESTRICT source, size_t size_bytes) {
    constexpr size_t Group = 4;
    constexpr size_t SIMD = sizeof(__m128) / sizeof(float);
    auto const groups = size_bytes / (Group * sizeof(__m128));
    auto const bytes = size_bytes - (groups * Group * sizeof(__m128));
    auto * p1 = reinterpret_cast<float *>(dest);
    auto const * p2 = reinterpret_cast<float const *>(source);
    for (size_t i = 0; i < groups; ++i) {
        __m128 a = _mm_load_ps(p2 + 0 * SIMD);
        __m128 b = _mm_load_ps(p2 + 1 * SIMD);
        __m128 c = _mm_load_ps(p2 + 2 * SIMD);
        __m128 d = _mm_load_ps(p2 + 3 * SIMD);
        _mm_store_ps(p1 + 0 * SIMD, a);
        _mm_store_ps(p1 + 1 * SIMD, b);
        _mm_store_ps(p1 + 2 * SIMD, c);
        _mm_store_ps(p1 + 3 * SIMD, d);
        p2 += SIMD * Group;
        p1 += SIMD * Group;
    }
    for (size_t i = 0; i < bytes; ++i)
        *((char *)dest + size_bytes - bytes + i) = *((char *)source + size_bytes - bytes + i);
    return dest;
}
YUGEN_ENGINE_API void *
MemCopy_SSE8 (void * YUGEN_RESTRICT dest, void const * YUGEN_RESTRICT source, size_t size_bytes) {
    constexpr size_t Group = 8;
    constexpr size_t SIMD = sizeof(__m128) / sizeof(float);
    auto const groups = size_bytes / (Group * sizeof(__m128));
    auto const bytes = size_bytes - (groups * Group * sizeof(__m128));
    auto * p1 = reinterpret_cast<float *>(dest);
    auto const * p2 = reinterpret_cast<float const *>(source);
    for (size_t i = 0; i < groups; ++i) {
        __m128 a = _mm_load_ps(p2 + 0 * SIMD);
        __m128 b = _mm_load_ps(p2 + 1 * SIMD);
        __m128 c = _mm_load_ps(p2 + 2 * SIMD);
        __m128 d = _mm_load_ps(p2 + 3 * SIMD);
        __m128 e = _mm_load_ps(p2 + 4 * SIMD);
        __m128 f = _mm_load_ps(p2 + 5 * SIMD);
        __m128 g = _mm_load_ps(p2 + 6 * SIMD);
        __m128 h = _mm_load_ps(p2 + 7 * SIMD);
        _mm_store_ps(p1 + 0 * SIMD, a);
        _mm_store_ps(p1 + 1 * SIMD, b);
        _mm_store_ps(p1 + 2 * SIMD, c);
        _mm_store_ps(p1 + 3 * SIMD, d);
        _mm_store_ps(p1 + 4 * SIMD, e);
        _mm_store_ps(p1 + 5 * SIMD, f);
        _mm_store_ps(p1 + 6 * SIMD, g);
        _mm_store_ps(p1 + 7 * SIMD, h);
        p2 += SIMD * Group;
        p1 += SIMD * Group;
    }
    for (size_t i = 0; i < bytes; ++i)
        *((char *)dest + size_bytes - bytes + i) = *((char *)source + size_bytes - bytes + i);
    return dest;
}
YUGEN_ENGINE_API void *
MemCopy_SSE1_NT (void * YUGEN_RESTRICT dest, void const * YUGEN_RESTRICT source, size_t size_bytes) {
    constexpr size_t Group = 1;
    constexpr size_t SIMD = sizeof(__m128) / sizeof(float);
    auto const groups = size_bytes / (Group * sizeof(__m128));
    auto const bytes = size_bytes - (groups * Group * sizeof(__m128));
    auto * p1 = reinterpret_cast<float *>(dest);
    auto const * p2 = reinterpret_cast<float const *>(source);
    for (size_t i = 0; i < groups; ++i) {
        __m128 a = _mm_load_ps(p2 + 0 * SIMD);
        _mm_stream_ps(p1 + 0 * SIMD, a);
        p2 += SIMD * Group;
        p1 += SIMD * Group;
    }
    for (size_t i = 0; i < bytes; ++i)
        *((char *)dest + size_bytes - bytes + i) = *((char *)source + size_bytes - bytes + i);
    return dest;
}
YUGEN_ENGINE_API void *
MemCopy_SSE2_NT (void * YUGEN_RESTRICT dest, void const * YUGEN_RESTRICT source, size_t size_bytes) {
    constexpr size_t Group = 2;
    constexpr size_t SIMD = sizeof(__m128) / sizeof(float);
    auto const groups = size_bytes / (Group * sizeof(__m128));
    auto const bytes = size_bytes - (groups * Group * sizeof(__m128));
    auto * p1 = reinterpret_cast<float *>(dest);
    auto const * p2 = reinterpret_cast<float const *>(source);
    for (size_t i = 0; i < groups; ++i) {
        __m128 a = _mm_load_ps(p2 + 0 * SIMD);
        __m128 b = _mm_load_ps(p2 + 1 * SIMD);
        _mm_stream_ps(p1 + 0 * SIMD, a);
        _mm_stream_ps(p1 + 1 * SIMD, b);
        p2 += SIMD * Group;
        p1 += SIMD * Group;
    }
    for (size_t i = 0; i < bytes; ++i)
        *((char *)dest + size_bytes - bytes + i) = *((char *)source + size_bytes - bytes + i);
    return dest;
}
YUGEN_ENGINE_API void *
MemCopy_SSE4_NT (void * YUGEN_RESTRICT dest, void const * YUGEN_RESTRICT source, size_t size_bytes) {
    constexpr size_t Group = 4;
    constexpr size_t SIMD = sizeof(__m128) / sizeof(float);
    auto const groups = size_bytes / (Group * sizeof(__m128));
    auto const bytes = size_bytes - (groups * Group * sizeof(__m128));
    auto * p1 = reinterpret_cast<float *>(dest);
    auto const * p2 = reinterpret_cast<float const *>(source);
    for (size_t i = 0; i < groups; ++i) {
        __m128 a = _mm_load_ps(p2 + 0 * SIMD);
        __m128 b = _mm_load_ps(p2 + 1 * SIMD);
        __m128 c = _mm_load_ps(p2 + 2 * SIMD);
        __m128 d = _mm_load_ps(p2 + 3 * SIMD);
        _mm_stream_ps(p1 + 0 * SIMD, a);
        _mm_stream_ps(p1 + 1 * SIMD, b);
        _mm_stream_ps(p1 + 2 * SIMD, c);
        _mm_stream_ps(p1 + 3 * SIMD, d);
        p2 += SIMD * Group;
        p1 += SIMD * Group;
    }
    for (size_t i = 0; i < bytes; ++i)
        *((char *)dest + size_bytes - bytes + i) = *((char *)source + size_bytes - bytes + i);
    return dest;
}
YUGEN_ENGINE_API void *
MemCopy_SSE8_NT (void * YUGEN_RESTRICT dest, void const * YUGEN_RESTRICT source, size_t size_bytes) {
    constexpr size_t Group = 8;
    constexpr size_t SIMD = sizeof(__m128) / sizeof(float);
    auto const groups = size_bytes / (Group * sizeof(__m128));
    auto const bytes = size_bytes - (groups * Group * sizeof(__m128));
    auto * p1 = reinterpret_cast<float *>(dest);
    auto const * p2 = reinterpret_cast<float const *>(source);
    for (size_t i = 0; i < groups; ++i) {
        __m128 a = _mm_load_ps(p2 + 0 * SIMD);
        __m128 b = _mm_load_ps(p2 + 1 * SIMD);
        __m128 c = _mm_load_ps(p2 + 2 * SIMD);
        __m128 d = _mm_load_ps(p2 + 3 * SIMD);
        __m128 e = _mm_load_ps(p2 + 4 * SIMD);
        __m128 f = _mm_load_ps(p2 + 5 * SIMD);
        __m128 g = _mm_load_ps(p2 + 6 * SIMD);
        __m128 h = _mm_load_ps(p2 + 7 * SIMD);
        _mm_stream_ps(p1 + 0 * SIMD, a);
        _mm_stream_ps(p1 + 1 * SIMD, b);
        _mm_stream_ps(p1 + 2 * SIMD, c);
        _mm_stream_ps(p1 + 3 * SIMD, d);
        _mm_stream_ps(p1 + 4 * SIMD, e);
        _mm_stream_ps(p1 + 5 * SIMD, f);
        _mm_stream_ps(p1 + 6 * SIMD, g);
        _mm_stream_ps(p1 + 7 * SIMD, h);
        p2 += SIMD * Group;
        p1 += SIMD * Group;
    }
    for (size_t i = 0; i < bytes; ++i)
        *((char *)dest + size_bytes - bytes + i) = *((char *)source + size_bytes - bytes + i);
    return dest;
}
YUGEN_ENGINE_API void *
MemCopy_AVX2 (void * YUGEN_RESTRICT dest, void const * YUGEN_RESTRICT source, size_t size_bytes) {
    constexpr size_t Group = 2;
    constexpr size_t SIMD = sizeof(__m256) / sizeof(float);
    auto const groups = size_bytes / (Group * sizeof(__m256));
    auto const bytes = size_bytes - (groups * Group * sizeof(__m256));
    auto * p1 = reinterpret_cast<float *>(dest);
    auto const * p2 = reinterpret_cast<float const *>(source);
    for (size_t i = 0; i < groups; ++i) {
        __m256 a = _mm256_load_ps(p2 + 0 * SIMD);
        __m256 b = _mm256_load_ps(p2 + 1 * SIMD);
        _mm256_store_ps(p1 + 0 * SIMD, a);
        _mm256_store_ps(p1 + 1 * SIMD, b);
        p2 += SIMD * Group;
        p1 += SIMD * Group;
    }
    for (size_t i = 0; i < bytes; ++i)
        *((char *)dest + size_bytes - bytes + i) = *((char *)source + size_bytes - bytes + i);
    return dest;
}
YUGEN_ENGINE_API void *
MemCopy_AVX2_NT (void * YUGEN_RESTRICT dest, void const * YUGEN_RESTRICT source, size_t size_bytes) {
    constexpr size_t Group = 2;
    constexpr size_t SIMD = sizeof(__m256) / sizeof(float);
    auto const groups = size_bytes / (Group * sizeof(__m256));
    auto const bytes = size_bytes - (groups * Group * sizeof(__m256));
    auto * p1 = reinterpret_cast<float *>(dest);
    auto const * p2 = reinterpret_cast<float const *>(source);
    for (size_t i = 0; i < groups; ++i) {
        __m256 a = _mm256_load_ps(p2 + 0 * SIMD);
        __m256 b = _mm256_load_ps(p2 + 1 * SIMD);
        _mm256_stream_ps(p1 + 0 * SIMD, a);
        _mm256_stream_ps(p1 + 1 * SIMD, b);
        p2 += SIMD * Group;
        p1 += SIMD * Group;
    }
    for (size_t i = 0; i < bytes; ++i)
        *((char *)dest + size_bytes - bytes + i) = *((char *)source + size_bytes - bytes + i);
    return dest;
}

    }   // namespace Internal
}   // namespace Yugen


#if defined(YUGEN_OS_WINDOWS)

#include <interface/external_include_win32.hpp> // for VirtualAlloc(), etc.

namespace Yugen {

YUGEN_ENGINE_API Blob
Mem_AllocPages (size_t size_bytes) {
    size_bytes = (size_bytes + Mem_PageSize() - 1) / Mem_PageSize() * Mem_PageSize();
    void * ptr = ::VirtualAlloc(nullptr, size_bytes, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    return {ptr, ptr ? size_bytes : 0};
}
YUGEN_ENGINE_API bool
Mem_FreePages (Blob mem) {
    return FALSE != ::VirtualFree(mem.ptr, 0, MEM_RELEASE);
}

}   // namespace Yugen

#else

#error "Implement virtual memory stuff for other platforms."

#endif


namespace Yugen {

Blob Allocator::alloc (size_t size, DbgInfo const & /*dbg_info*/) {
    return alloc(size);
}
void Allocator::dealloc (Blob const & mem, DbgInfo const & /*dbg_info*/) {
    return dealloc(mem);
}
bool Allocator::stats (Stats * /*out_stats*/) const {
    return false;
}
Allocator::Caps Allocator::capabilities () const {
    return Caps::None;
}
char const * Allocator::name_of_instance () const {
    return name_of_class();
}
Blob Allocator::alloc_and_zero (size_t size) {
    auto ret = alloc(size);
    if (YUGEN_PTR_VALID(ret.ptr))
        ::memset(ret.ptr, 0, ret.len);
    return ret;
}
Blob Allocator::alloc_and_zero (size_t size, DbgInfo const & dbg_info) {
    auto ret = alloc(size, dbg_info);
    if (YUGEN_PTR_VALID(ret.ptr))
        ::memset(ret.ptr, 0, ret.len);
    return ret;
}
Blob Allocator::alloc_aligned (size_t size, size_t alignment, size_t offset, Blob * out_aligned_blob) {
    auto actual_size = size + alignment - 1 + offset;
    auto ret = alloc(actual_size);
    if (out_aligned_blob) {
        out_aligned_blob->ptr = (Byte *)(size_t(ret.ptr + alignment - 1) / alignment * alignment + offset);
        out_aligned_blob->len = size;
    }
    return ret;
}
Blob Allocator::alloc_aligned (size_t size, size_t alignment, size_t offset, Blob * out_aligned_blob, DbgInfo const & dbg_info) {
    auto actual_size = size + alignment - 1 + offset;
    auto ret = alloc(actual_size, dbg_info);
    if (out_aligned_blob) {
        out_aligned_blob->ptr = (Byte *)(size_t(ret.ptr + alignment - 1) / alignment * alignment + offset);
        out_aligned_blob->len = size;
    }
    return ret;
}
Blob Allocator::realloc (Blob const & mem, size_t new_size) {
    Blob new_mem = {};
    if (new_size != 0) {
        new_mem = alloc(new_size);
    }
    if (new_mem.ptr) {
        ::memcpy(new_mem.ptr, mem.ptr, Min(mem.len, new_mem.len));
    }
    if (0 == new_size || new_mem.ptr) {
        dealloc(mem);
    }
    return new_mem;
}
Blob Allocator::realloc (Blob const & mem, size_t new_size, DbgInfo const & dbg_info) {
    Blob new_mem = {};
    if (new_size != 0) {
        new_mem = alloc(new_size, dbg_info);
    }
    if (new_mem.ptr) {
        ::memcpy(new_mem.ptr, mem.ptr, Min(mem.len, new_mem.len));
    }
    if (0 == new_size || new_mem.ptr) {
        dealloc(mem, dbg_info);
    }
    return new_mem;
}
bool Allocator::owns_pointer (void const * ptr) const {
    return owns_range(CBlob{ptr, 0});
}
size_t Allocator::block_size (void const * ptr) const {
    return 0;
}

    namespace Allocators {

SmallShortLivedChunked::~SmallShortLivedChunked () {
    YUGEN_ASSERT_STRONG(
        nullptr == m_last_chunk || (0 == m_last_chunk->ref_count && nullptr == m_last_chunk->prev),
        "Expected all blocks inside all the chunks to have had been freed. Alas, it is not so..."
    );
    auto p = m_last_chunk;
    while (p) {
        auto q = p->prev;
        Mem_FreePages({p, p->total_bytes});
        p = q;
    }
}
Blob SmallShortLivedChunked::allocate (size_t size) {
    Blob ret = {};
    ChunkSizeType const size_inc_overhead = ChunkSizeType(size + sizeof(BlockHeader));
    if (ensure_availability(size_inc_overhead)) {
        YUGEN_ASSERT(nullptr != m_last_chunk && (m_last_chunk->used_bytes + size_inc_overhead <= m_last_chunk->total_bytes));
            
        auto p = (BlockHeader *)((Byte *)m_last_chunk + m_last_chunk->used_bytes);

        YUGEN_ASSERT(m_last_chunk->used_bytes == (BlockSizeType)m_last_chunk->used_bytes);
        p->offset_in_chunk = (BlockSizeType)m_last_chunk->used_bytes;
        p->block_size = BlockSizeType(size);

        ret = {p + 1, size};
        m_last_chunk->used_bytes += size_inc_overhead;
        m_last_chunk->ref_count += 1;
    }
    return ret;
}
bool SmallShortLivedChunked::free (Blob mem) {
    bool ret = false;
    auto bh = block_header_from_block(mem.ptr);
    if (
        YUGEN_ASSERT_REPAIR(bh) &&
        YUGEN_ASSERT_REPAIR(bh->block_size == mem.len)
    ) {
        auto ch = chunk_header_from_block(mem.ptr);
        if (
            YUGEN_ASSERT_REPAIR(ch) &&
            YUGEN_ASSERT_REPAIR(ch->signature == ChunkHeader::CorrectSignature) &&
            YUGEN_ASSERT_REPAIR(ch->owner == this) &&
            YUGEN_ASSERT_REPAIR(ch->ref_count > 0)
        ) {
            --ch->ref_count;

            if (ch != m_last_chunk && ch->ref_count == 0) {
                [[maybe_unused]] bool b = release_old_chunk(ch);
                YUGEN_ASSERT(b);
            }

            ret = true;
        }
    }
    return ret;
}
bool SmallShortLivedChunked::owns (void * ptr) const {
    auto ch = chunk_header_from_block(ptr);
    return ChunkHeader::CorrectSignature == ch->signature && this == ch->owner;
}
size_t SmallShortLivedChunked::block_size (void * ptr) const {
    return block_header_from_block(ptr)->block_size;
}
Blob SmallShortLivedChunked::acquire_block (Blob existing_block) {
    Blob ret = {};
    auto bh = block_header_from_block(existing_block.ptr);
    auto ch = chunk_header_from_block(existing_block.ptr);
    if (existing_block.len == bh->block_size && ChunkHeader::CorrectSignature == ch->signature && this == ch->owner) {
        ch->ref_count += 1;
        ret = existing_block;
    }
    return ret;
}
bool SmallShortLivedChunked::ensure_availability (size_t size_inc_overhead) {
    bool ret = false;
    if (size_inc_overhead > 0 && size_inc_overhead <= LargestPossibleBlockIncOverhead) {
        bool have_room = 
            (
                nullptr != m_last_chunk &&
                m_last_chunk->used_bytes + size_inc_overhead <= m_last_chunk->total_bytes
            )
            ||
            acquire_new_chunk();

        ret = have_room;
    }
    return ret;
}
bool SmallShortLivedChunked::acquire_new_chunk () {
    bool ret = false;
    if (nullptr != m_last_chunk && m_last_chunk->ref_count <= 0) {
        YUGEN_ASSERT_STRONG('SSLC' == m_last_chunk->signature, "Chunk (at address 0x%016X) has its signature corrupted!", m_last_chunk);
        YUGEN_ASSERT_STRONG(m_last_chunk->ref_count == 0, "Dafuq?! Chunk (at address 0x%016X) reference count is %d...", m_last_chunk, m_last_chunk->ref_count);
        m_last_chunk->used_bytes = ChunkSizeType(sizeof(ChunkHeader));
        ret = true;
    } else {
        auto blob = Mem_AllocPages(ChunkSize);
        YUGEN_ASSERT_STRONG((uint64_t(blob.ptr) % ChunkSize) == 0, "Expected page-allocated pointer to be aligned on page size (which is %u), instead got 0x%016X.", ChunkSize, blob.ptr);
        if (nullptr != blob.ptr) {
            auto chunk = blob.as<ChunkHeader>();
            chunk->owner = this;
            chunk->prev = m_last_chunk;
            chunk->next = nullptr;
            chunk->total_bytes = ChunkSizeType(ChunkSize);
            chunk->used_bytes = ChunkSizeType(sizeof(ChunkHeader));
            chunk->ref_count = 0;
            chunk->signature = ChunkHeader::CorrectSignature;
            if (nullptr != m_last_chunk)
                m_last_chunk->next = chunk;
            m_last_chunk = chunk;
            ret = true;
        }
    }
    return ret;
}
bool SmallShortLivedChunked::release_old_chunk (ChunkHeader * chunk) {
    bool ret = false;
    if (chunk) {
        YUGEN_ASSERT_STRONG(chunk->ref_count == 0, "Freeing a chunk with live blocks inside!");
        YUGEN_ASSERT_STRONG(chunk != m_last_chunk, "Shouldn't release current, in-use chunk!");
        if (chunk->prev)
            chunk->prev->next = chunk->next;
        if (chunk->next)
            chunk->next->prev = chunk->prev;
        [[maybe_unused]] bool b = Mem_FreePages({chunk, chunk->total_bytes});
        YUGEN_ASSERT(b);
        ret = true;
    }
    return ret;
}

CRT::CRT (char const * instance_name)
    : Allocator {nullptr}
    , m_instance_name {instance_name}
{
    m_stats.overhead_bytes = sizeof(*this);
}
CRT::~CRT () {
    YUGEN_ASSERT_STRONG(m_stats.total_freed_bytes == m_stats.total_requested_bytes);
}
char const * CRT::name_of_class () const {
    return "CRT malloc()ator";
}
Blob CRT::alloc (size_t size) {
    Blob ret = {};
    ret.ptr = static_cast<Byte *>(::malloc(size));
    if (YUGEN_PTR_VALID(ret.ptr)) {
        ret.len = size;

        m_stats.alloc_calls += 1;
        m_stats.total_requested_bytes += size;
        if (m_stats.most_total_allocated_bytes < m_stats.total_requested_bytes - m_stats.total_freed_bytes)
            m_stats.most_total_allocated_bytes = m_stats.total_requested_bytes - m_stats.total_freed_bytes;
    }
    return ret;
}
void CRT::dealloc (Blob const & mem) {
    YUGEN_ASSERT(nullptr == mem.ptr || mem.len > 0);
    if (YUGEN_PTR_VALID(mem.ptr)) {
        ::free(mem.ptr);

        m_stats.free_calls += 1;
        m_stats.total_freed_bytes += mem.len;
    }
}
bool CRT::owns_range (CBlob const & range) const {
    return false;
}
bool CRT::reset () {
    return false;
}
Allocator::Caps CRT::capabilities () const {
    return Caps::None
        | Caps::NamedInstance
        | Caps::Realloc
        | Caps::Stats
      //| Caps::ThreadSafe
        ;
}
bool CRT::stats (Stats * out_stats) const {
    *out_stats = m_stats;
    return true;
}
char const * CRT::name_of_instance () const {
    return m_instance_name;
}
Blob CRT::realloc (Blob const & mem, size_t new_size) {
    auto p = ::realloc(mem.ptr, new_size);

    m_stats.realloc_calls += 1;
    m_stats.total_freed_bytes += mem.len;
    m_stats.total_requested_bytes += new_size;
    if (m_stats.most_total_allocated_bytes < m_stats.total_requested_bytes - m_stats.total_freed_bytes)
        m_stats.most_total_allocated_bytes = m_stats.total_requested_bytes - m_stats.total_freed_bytes;

    return {p, new_size};
}
//----------------------------------------------------------------------
SlowGeneralPurpose::SlowGeneralPurpose (char const * instance_name, Blob const & mem)
    : Allocator {nullptr}
    , m_mem {mem}
    , m_instance_name {instance_name}
{
    YUGEN_ASSERT_STRONG(YUGEN_PTR_VALID(m_mem.ptr) && m_mem.len >= 3 * sizeof(Header));
    m_aligned_mem.ptr = align_up(m_mem.ptr);
    m_aligned_mem.len = align_down(m_mem.len - (m_aligned_mem.ptr - m_mem.ptr));
    YUGEN_ASSERT_STRONG(YUGEN_PTR_VALID(m_aligned_mem.ptr) && m_aligned_mem.len >= 3 * sizeof(Header));
    YUGEN_ASSERT_STRONG(0 == (intptr_t(m_aligned_mem.ptr) & AlignMask));
    YUGEN_ASSERT_STRONG(0 == (m_aligned_mem.len & AlignMask));

    auto free_size = make_size(m_aligned_mem.len - 3 * sizeof(Header));
    
    header_init(header_at(0), State::Sentinel, 0, 0);
    header_init(header_at(1), State::Free, free_size, 0);
    header_init(header_at(free_size + 2), State::Sentinel, 0, free_size);

    m_first_free = 1;

    // TODO(yzt): Update stats
    // TODO(yzt): Do the mutex!
}
SlowGeneralPurpose::~SlowGeneralPurpose () {
    //...
}
char const * SlowGeneralPurpose::name_of_class () const {
    return "Slow General-Purpose";
}
Blob SlowGeneralPurpose::alloc (size_t size) {
    Blob ret = {};
    if (size > 0 && size <= LargestPossibleSize) {
        auto s = make_size(size);
        auto o = find_suitable_free_block(s);
        if (0 != o) {
            auto h = header_at(o);
            YUGEN_ASSERT(State::Free == h->state && h->size >= s);
            if (h->size <= s + 1)
                s = h->size;
            if (h->size > s) {
                auto g = header_at(o + s);
                header_init(g, State::Free, h->size - s - 1, s);
                g->next_free = h->next_free;
                g->prev_free = h->prev_free;
                if (o == m_first_free)
                    m_first_free = o + s;
            }
            h->state = State::Allocated;
            h->size = s;
            h->next_free = 0;
            h->prev_free = 0;
            // TODO(yzt): Set debug info as well, if available.
            ret = {(Byte *)(h + 1), size_t(s) << AlignShift};
        }
    }
    return ret;
}
void SlowGeneralPurpose::dealloc (Blob const & mem) {
    if (owns_range(mem)) {
        auto o = make_offset(mem.ptr);
        if (0 != o) {
            auto h = header_at(o);
            YUGEN_ASSERT(Cookie1 == h->cookie_first);
            YUGEN_ASSERT(Cookie2 == h->cookie_second);
            YUGEN_ASSERT(State::Allocated == h->state);
            YUGEN_ASSERT(make_size(mem.len) == h->size);
            
            // TODO(yzt): Coallesce adjacent free blocks.

            h->state = State::Free;
            if (0 != m_first_free) {
                h->next_free = m_first_free;
                YUGEN_ASSERT(0 == header_at(m_first_free)->prev_free);
                header_at(m_first_free)->prev_free = o;
            }
            m_first_free = o;
        }
    }
}
bool SlowGeneralPurpose::owns_range (CBlob const & range) const {
    return (m_aligned_mem.ptr <= range.ptr) && (range.ptr + range.len <= m_aligned_mem.ptr + m_aligned_mem.len);
}
bool SlowGeneralPurpose::reset () {
    return false;
}
Allocator::Caps SlowGeneralPurpose::capabilities () const {
    return Caps::LimitedSize | Caps::NamedInstance | Caps::OwnershipTest | Caps::SizeCheck | Caps::Stats | Caps::ThreadSafe;
}
bool SlowGeneralPurpose::stats (Stats * out_stats) const {
    *out_stats = m_stats;
    return true;
}
char const * SlowGeneralPurpose::name_of_instance () const {
    return m_instance_name;
}
bool SlowGeneralPurpose::internal_sanity_check (bool report_leaks, bool report_all_blocks) {
    //...
    return false;
}

SlowGeneralPurpose::Header * SlowGeneralPurpose::put_header (uint64_t offset, State state, uint64_t size, uint64_t neighbor_offset) {
    YUGEN_ASSERT(offset >= 0 && offset + sizeof(Header) <= m_mem.len); 
    YUGEN_ASSERT((size % 16) == 0);
    YUGEN_ASSERT(size / 16 <= 0xFFFF'FFFF);
    //YUGEN_ASSERT(...);
    auto hdr = reinterpret_cast<Header *>(m_mem.ptr + offset);
    hdr->state = state;
    hdr->size = uint32_t(size / 16);
    hdr->neighbor_size = uint32_t((offset - neighbor_offset) / 16);
    return hdr;
}
void SlowGeneralPurpose::header_init (Header * hdr, State state, Offset size, Offset neighbor_size) {
    hdr->cookie_first = Cookie1;
    hdr->state = state;
    hdr->size = size;
    hdr->neighbor_size = neighbor_size;
    hdr->next_free = 0;
    hdr->prev_free = 0;
    hdr->func = nullptr;
    hdr->name = nullptr;
    hdr->file = nullptr;
    hdr->line = 0;
    hdr->cookie_second = Cookie2;
}
SlowGeneralPurpose::Offset SlowGeneralPurpose::find_suitable_free_block (Offset size) {
    auto f = m_first_free;
    while (f) {
        auto p = header_at(f);
        YUGEN_ASSERT(State::Free == p->state);
        if (p->size >= size)
            break;
        f = p->next_free;
    }
    return f;
}

    } // namespace Allocators

static bool check_characters(char const* crc, char const* valid_crc, int size) {
    bool ret = true;
    for (int i = 0; i < size; ++i) {
        if (crc[i] != valid_crc[i]) {
            ret = false;
            break;
        }
    }
    return ret;
}

static void load_characters(char* crc, char const* valid_crc, int size) {
    for (int i = 0; i < size; ++i) {
        crc[i] = valid_crc[i];
    }
}


static void
log (char const* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    ::vfprintf(stderr, fmt, args);
    va_end(args);
}

Blob GP_Allocator::payload_from_header(Header* header)
{
    return { (Byte*)(header + 1),header->size };
}

GP_Allocator::Footer* GP_Allocator::footer_from_header(Header* header)
{
    return (Footer*)((Byte*)header + sizeof(Header) + header->size);
}

GP_Allocator::Header* GP_Allocator::header_from_payload(void* ptr)
{
    return (Header*)((Byte*)ptr - sizeof(Header));
}

GP_Allocator::GP_Allocator () {
}

GP_Allocator::~GP_Allocator() {
    while (m_first_header)
    {
        auto p_header = m_first_header;
        m_first_header = m_first_header->next;
        ::memset(p_header, 0, sizeof(Header) + p_header->size + sizeof(Footer));
        p_header->state = State::FREE;
        ::free(p_header);
    }
}

Blob GP_Allocator::alloc(size_t size, char const* file, int line, char const* func, char const* system)
{
    Blob ret = {};

    auto p_header = static_cast<Header*>(::malloc(sizeof(Header) + size + sizeof(Footer)));
    if (p_header != nullptr)
    {
        load_characters(p_header->signature, signature, 4);
        p_header->state = State::ALLOCATED;
        p_header->size = size;
        p_header->next = nullptr;
        p_header->prev = nullptr;
        p_header->file = file;
        p_header->func = func;
        p_header->system = system;
        p_header->line = line;
        load_characters(p_header->sentinel, header_sentinel, 4);

        auto p_footer = footer_from_header(p_header);
        load_characters(p_footer->sentinel, footer_sentinel, 16);

        if (m_first_header != nullptr) {
            m_first_header->prev = p_header;
        }
        p_header->next = m_first_header;
        m_first_header = p_header;

        ret = payload_from_header(p_header);
    }
    return ret;
}

// TODO: Return some kind of error code (mostly for unit-testing.)
void GP_Allocator::dealloc(Blob const& mem, char const* file, int line, char const* func, char const* system)
{
    if (mem.ptr) {
        auto p_header = header_from_payload(mem.ptr);
        if (p_header->state == State::ALLOCATED) {
            if (check_characters(p_header->signature, signature, 4))
            {
                if (mem.len != p_header->size)
                    log("[MEMORY ALLOCATOR] [WARNING] Wrong size for the allocated block\n FILE: %s\n LINE: %d\nFUNC: %s\nSYS: %s\n"
                       , file, line, func, system
                    );
                if (!check_characters(p_header->sentinel, header_sentinel, 4))
                    log(
                    "[MEMORY ALLOCATOR] [WARNING] Overwritten header\n FILE: %s\n LINE: %d\nFUNC: %s\nSYS: %s\n"
                       , file, line, func, system
                    );
                auto p_footer = footer_from_header(p_header);
                if (!check_characters(p_footer->sentinel, footer_sentinel, 16))
                    log(
                    "[MEMORY ALLOCATOR] [WARNING] Overwritten footer\n FILE: %s\n LINE: %d\nFUNC: %s\nSYS: %s\n"
                       , file, line, func, system
                    );

                if (p_header->prev)
                    p_header->prev->next = p_header->next;

                if (p_header->next)
                    p_header->next->prev = p_header->prev;

                if (m_first_header == p_header)
                    m_first_header = p_header->next;

                memset(p_header, 0, sizeof(Header) + mem.len + sizeof(Footer));
                p_header->state = State::FREE;
                ::free(p_header);
            }
        }
        else
            log("[MEMORY ALLOCATOR] [ERROR] Double free\n");
    }
}

int GP_Allocator::print_block_list (bool do_print) const
{
    int ret = 0;
    auto p_header = m_first_header;
    if (do_print && p_header) {
        log("[MEMORY ALLOCATOR] [Block List]\n");
    }
    while (p_header)
    {
        if (do_print) {
            log("{ptr: %p} {Size: %d} {System: %s} {File: %s} {Function: %s} {state: %s} \n"
                , payload_from_header(p_header).ptr
                , p_header->size
                , p_header->system
                , p_header->file
                , p_header->func
                , ((p_header->state == State::ALLOCATED) ? "Allocated" : (p_header->state == State::FREE ? "Free" : "Invalid"))
            );
        }
        p_header = p_header->next;
        ret += 1;
    }
    return ret;
}

class AllocSubsys :public Subsystem::Interface
{
public:
    AllocSubsys() { }
    virtual ~AllocSubsys() { m_allocator.print_block_list(); }

    virtual bool init() override { return true; }
    virtual bool shutdown() override { return true; }

public:
    Blob alloc(size_t size, char const* file, int line, char const* func, char const* system)
    {
        return  m_allocator.alloc(size, file, line, func, system);
    }
    void dealloc(Blob const& mem, char const* file, int line, char const* func, char const* system)
    {
        //m_allocator.print_block_list();
        return m_allocator.dealloc(mem, file, line, func, system);
    }
    int enumerate_blocks (bool print) {
        return m_allocator.print_block_list(print);
    }
private:
    GP_Allocator m_allocator;
};

static AllocSubsys* g_sysptr_alloc = new AllocSubsys;
YUGEN_SUBSYSTEM_REGISTER_SIMPLE(AllocSubsys, "Allocation", 0.05f, (void**)&g_sysptr_alloc);

YUGEN_ENGINE_API Blob
Mem_ActualAlloc(size_t size, char const* file, int line, char const* func, char const* system)
{
    return g_sysptr_alloc->alloc(size, file, line, func, system);

}

YUGEN_ENGINE_API void
Mem_ActualFree(Blob const& mem, char const* file, int line, char const* func, char const* system)
{
    return g_sysptr_alloc->dealloc(mem, file, line, func, system);
}

YUGEN_ENGINE_API int
Mem_GPAllocator_EnumerateAllBlocks_DEBUG (bool print_blocks) {
    return g_sysptr_alloc->enumerate_blocks(print_blocks);
}


} // namespace Yugen
#endif