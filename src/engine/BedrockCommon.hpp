#pragma once

// This class was originally part of Yugen engine by Y.Zhian

#include "BedrockPlatforms.hpp"

#include <cstdint>
// TODO I wish we could add no-discard
#define MFA_PTR_VALID(p_) ((p_) != nullptr)
#define MFA_BLOB_VALID(blob) (MFA_PTR_VALID(blob.ptr) && blob.len > 0)
#define MFA_CONSUME_VAR(v_) ((void)(v_))

#define MFA_UNIQUE_NAME(base_) MFA_CONCAT(base_, __COUNTER__)

#define MFA_CONCAT__IMPL(x_, y_) x_ ## y_
#define MFA_CONCAT(x_, y_) MFA_CONCAT__IMPL(x_, y_)

#define MFA_STRINGIFY__IMPL(x_)   #x_
#define MFA_STRINGIFY(x_)         MFA_STRINGIFY__IMPL(x_)

#define MFA_DEFER auto MFA_UNIQUE_NAME(deferrinator_) = DeferHelper{} + [&]()

#ifndef __PLATFORM_WIN__
#include <stdio.h>
#include <stddef.h>
#endif

// Do not use this, Use MFA_DEFFER instead
template <typename F>
struct Deferrer {
    Deferrer (F && f) : m_f (std::move(f)) {}
    ~Deferrer () noexcept {if (!m_canceled) m_f();}
    void cancel () noexcept {m_canceled = true;}
private:
    bool m_canceled = false;
    F m_f;
};
template <typename F>
Deferrer<F> MakeDeferrer (F && f) {return Deferrer<F>(std::move(f));}
struct DeferHelper {
    template <typename F>
    friend Deferrer<F> operator + (DeferHelper const &, F && f) {return MakeDeferrer(std::move(f));}
};

namespace MFA {

/* Ubiquitous Types */
using I8 = std::int8_t;
using I16 = std::int16_t;
using I32 = std::int32_t;
using I64 = std::int64_t;

using U8 = std::uint8_t;
using U16 = std::uint16_t;
using U32 = std::uint32_t;
using U64 = std::uint64_t;

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