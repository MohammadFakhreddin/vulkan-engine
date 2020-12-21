#ifndef BASE_INTERFACE
#define BASE_INTERFACE

#include <vulkan/vulkan.h>

#define MFA_PTR_VALID(p_) ((p_) != nullptr)

#define MFA_CONSUME_VAR(v_)       ((void)(v_))

namespace MFA {

/* Ubiquitous Types */
using I8 = int8_t;
using I16 = int16_t;
using I32 = int32_t;
using I64 = int64_t;

using U8 = uint8_t;
using U16 = uint16_t;
using U32 = uint32_t;
using U64 = uint64_t;

using Byte = U8;

using F32 = float;
using F64 = double;

/* Blob, TBlob, CBlob and TCBlob (C means that the contents are const) */
template <typename T>
struct TBlob {
    T * ptr = nullptr; // For safety checking when placed inside uninitialized struct
    size_t len = 0;

    TBlob () = default;
    TBlob (T * p, size_t l) : ptr {p}, len {l} {}
    TBlob (void * p, size_t bytes) : ptr {(T *)p}, len {bytes / sizeof(T)} {}

    template <typename U>
    U * as () {return reinterpret_cast<U *>(ptr);}
    template <typename U>
    U * as () const {return reinterpret_cast<U *>(ptr);}
    template <typename U>
    U const * as_const () const {return reinterpret_cast<U const *>(ptr);}
};

using Blob = TBlob<Byte>;

template <typename T>
struct TCBlob {
    T const * ptr;
    size_t len;

    TCBlob () = default;
    TCBlob (TBlob<T> const & blob) : ptr {blob.ptr}, len {blob.len} {}
    TCBlob (T const * p, size_t l) : ptr {p}, len {l} {}
    TCBlob (void const * p, size_t bytes) : ptr {(T const *)p}, len {bytes / sizeof(T)} {}

    template <typename U>
    U const * as () const {return reinterpret_cast<U const *>(ptr);}
};

using CBlob = TCBlob<Byte>;

template <typename T>
CBlob CBlobAliasOf (T const & v) {
    return {&v, sizeof(v)};
}
template <typename T, size_t N>
CBlob CBlobAliasOf (T const (&v) [N]) {
    return {&v[0], sizeof(v)};
}

template <typename T, size_t N>
constexpr size_t ArrayCount (T (&) [N]) {return N;}

template <typename T, uint32_t N>
constexpr uint32_t ArrayCount32(T(&)[N]) { return N; }

template <typename T, int N>
constexpr int ArrayCountInt(T(&)[N]) { return N; }

}

#endif