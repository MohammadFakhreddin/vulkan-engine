#pragma once

// This class was originally part of Yugen engine by Y.Zhian

#include "BedrockPlatforms.hpp"

#include <cstdint>
#include <memory>
#include <cstring>

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

#ifdef __PLATFORM_LINUX__
#include <stdint.h>
#endif

// Do not use this, Use MFA_DEFFER instead
template <typename F>
struct Deferrer
{
    Deferrer(F && f) : m_f(std::move(f)) {}
    ~Deferrer() noexcept { if (!m_canceled) m_f(); }
    void cancel() noexcept { m_canceled = true; }
private:
    bool m_canceled = false;
    F m_f;
};
template <typename F>
Deferrer<F> MakeDeferrer(F && f) { return Deferrer<F>(std::move(f)); }
struct DeferHelper
{
    template <typename F>
    friend Deferrer<F> operator + (DeferHelper const &, F && f) { return MakeDeferrer(std::move(f)); }
};

namespace MFA
{

    using F32 = float;
    using F64 = double;

    /* Blob, TBlob, CBlob and TCBlob (C means that the contents are const) */
    template <typename T>
    struct TBlob
    {
        T * ptr = nullptr; // For safety checking when placed inside uninitialized struct
        size_t len = 0;

        TBlob() = default;
        TBlob(T * p, size_t l) : ptr{ p }, len{ l } {}
        TBlob(void * p, size_t bytes) : ptr{ (T *)p }, len{ bytes / sizeof(T) } {}

        template <typename U>
        U * as() { return reinterpret_cast<U *>(ptr); }
        template <typename U>
        U * as() const { return reinterpret_cast<U *>(ptr); }
        template <typename U>
        U const * as_const() const { return reinterpret_cast<U const *>(ptr); }
        bool isEmpty()
        {
            return ptr == nullptr && len = 0;
        }
    };

    using Blob = TBlob<uint8_t>;

    template <typename T>
    struct TCBlob
    {
        T const * ptr;
        size_t len;

        TCBlob() = default;
        TCBlob(TBlob<T> const & blob) : ptr{ blob.ptr }, len{ blob.len } {}
        TCBlob(T const * p, size_t l) : ptr{ p }, len{ l } {}
        TCBlob(void const * p, size_t bytes) : ptr{ (T const *)p }, len{ bytes / sizeof(T) } {}

        template <typename U>
        U const * as() const { return reinterpret_cast<U const *>(ptr); }
    };

    using CBlob = TCBlob<uint8_t>;

    template <typename T>
    CBlob CBlobAliasOf(T const & v)
    {
        return { &v, sizeof(v) };
    }
    template <typename T, size_t N>
    CBlob CBlobAliasOf(T const (&v)[N])
    {
        return { &v[0], sizeof(v) };
    }

    template <typename T, size_t N>
    constexpr size_t ArrayCount(T(&)[N]) { return N; }

    template <typename T, uint32_t N>
    constexpr uint32_t ArrayCount32(T(&)[N]) { return N; }

    template <typename T, int N>
    constexpr int ArrayCountInt(T(&)[N]) { return N; }

    template<uint32_t Count, typename B, typename A>
    constexpr void Copy(B * dst, A const * src)
    {
        static_assert(sizeof(B) == sizeof(A));
        memcpy(dst, src, Count * sizeof(B));
    }

    template<uint32_t Count, typename  T>
    constexpr void Copy(T * dst, std::initializer_list<T> items)
    {
        memcpy(dst, items.begin(), Count * sizeof(T));
    }

    template<typename T, typename B>
    constexpr void Copy(T * dst, B const * src, uint32_t const count)
    {
        static_assert(sizeof(T) == sizeof(B));
        memcpy(dst, src, count * sizeof(T));
    }

    template<typename A, typename B>
    constexpr void Copy(A & dst, B const & src)
    {
        if constexpr (sizeof(A) > sizeof(B))
        {
            memcpy(&dst, &src, sizeof(B));
        } else
        {
            memcpy(&dst, &src, sizeof(A));
        }
    }

    template<uint32_t Count, typename A, typename B>
    constexpr void Copy(A & dst, B const * src)
    {
        static_assert(sizeof(A) >= sizeof(B) * Count);
        memcpy(&dst, src, sizeof(B) * Count);
    }

    template<uint32_t Count, typename A, typename B>
    constexpr void Copy(A * dst, B const & src)
    {
        static_assert(sizeof(A) * Count <= sizeof(B));
        memcpy(dst, &src, sizeof(A) * Count);
    }

    template<typename A, typename B>
    constexpr A Copy(B const & src)
    {
        A dst {};
        Copy(dst, src);
        return dst;
    }

    template<uint32_t Count, typename A, typename B>
    constexpr A Copy(B const * src)
    {
        A dst {};
        Copy<Count, A, B>(dst, src);
        return dst;
    }

    template<uint32_t Count, typename T>
    constexpr bool IsEqual(T const * memory1, T const * memory2)
    {
        return memcmp(memory1, memory2, Count * sizeof(T)) == 0;
    }

    template<uint32_t Count, typename A, typename B>
    constexpr bool IsEqual(A const & memory1, B const * memory2)
    {
        static_assert(sizeof(A) >= sizeof(B) * Count);
        return memcmp(&memory1, memory2, sizeof(B) * Count) == 0;
    }

    template<uint32_t Count, typename A, typename B>
    constexpr bool IsEqual(A const * memory1, B const & memory2)
    {
        static_assert(sizeof(A) * Count <= sizeof(B));
        return memcmp(memory1, &memory2, sizeof(A) * Count) == 0;
    }

    template<typename A, typename B>
    constexpr bool IsEqual(A const & memory1, B const & memory2)
    {
        if constexpr (sizeof(A) > sizeof(B))
        {
            return memcmp(&memory1, &memory2, sizeof(B)) == 0;
        }
        return memcmp(&memory1, &memory2, sizeof(A)) == 0;
    }

    template<typename T>
    constexpr void SetZero(T const * memory, uint32_t length) 
    {
        memset(memory, 0, sizeof(T) * length);
    }

}
