#pragma once

#include "BedrockCommon.hpp"

namespace MFA
{
    struct SmartBlob
    {

        explicit SmartBlob(Blob memory_);
        ~SmartBlob();

        SmartBlob(SmartBlob const &) noexcept = delete;
        SmartBlob(SmartBlob &&) noexcept = delete;
        SmartBlob & operator= (SmartBlob const & rhs) noexcept = delete;
        SmartBlob & operator= (SmartBlob && rhs) noexcept = delete;

        Blob const memory {};

    };
}

namespace MFA::Memory
{

    std::shared_ptr<SmartBlob> Alloc(size_t size);

    //void Free(SmartBlob const & mem);

    //void PtrFree(void * ptr);

    //template <typename T>
    //T * New_NoConstruct()
    //{
    //    auto blob = Alloc(sizeof(T));
    //    return blob.as<T>();
    //}

    //template <typename T>
    //void Delete_NoDestruct(T * ptr)
    //{
    //    return Free({ ptr, sizeof(T) });
    //}

    //template <typename T>
    //TBlob<T> NewArray_NoConstruct(size_t count)
    //{
    //    auto blob = Alloc(sizeof(T) * count);
    //    return { blob.as<void>(), blob.len };
    //}

    //template <typename T>
    //void DeleteArray_NoDestruct(TBlob<T> blob)
    //{
    //    Free({ blob.ptr, blob.len * sizeof(T) });
    //}

    //template <typename T, typename ... ArgTypes>
    //T * New(ArgTypes && ... args)
    //{
    //    auto ret = New_NoConstruct<T>();
    //    if (ret)
    //    {
    //        try
    //        {
    //            new (ret) T(std::forward<ArgTypes>(args)...);
    //        }
    //        catch (...)
    //        {
    //            ret->~T();
    //            Delete_NoDestruct(ret);
    //            ret = nullptr;
    //        }
    //    }
    //    return ret;
    //}

    //template <typename T>
    //void Delete(T * ptr)
    //{
    //    if (ptr)
    //    {
    //        ptr->~T();
    //        Delete_NoDestruct(ptr);
    //    }
    //}

} // MFA::Memory
