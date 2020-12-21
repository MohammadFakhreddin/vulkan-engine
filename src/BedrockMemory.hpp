// TODO Implement memory system
#if 0
#ifndef MEMORY_HEADER
#define MEMORY_HEADER

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

namespace MFA {

Blob Mem_ActualAlloc(
    size_t size,
    char const * file,
    int line,
    char const * func,
    char const * system
);

void Mem_ActualFree(
    Blob const & mem, 
    char const * file,
    int line,
    char const * func,
    char const * system
);

int Mem_GPAllocator_EnumerateAllBlocks_DEBUG (bool print_blocks);

#pragma region "Generic, and soon-to-be-deprecated allocation and deallocation"
Blob Mem_Alloc (size_t size);

//Blob
//Mem_AllocOnStack (size_t size);

void Mem_Free (Blob const & mem);

//void
//Mem_FreeOnStack (Blob const & mem);

Blob Mem_Duplicate (CBlob const & src);    // Returns *new* memory, with copied contents from src

Blob Mem_Realloc (Blob const & mem, size_t new_size);

template <typename T>
T * Mem_New_NoConstruct () {
    auto blob = Mem_Alloc(sizeof(T));
    return blob.as<T>();
}
template <typename T>
void Mem_Delete_NoDestruct (T * ptr) {
    return Mem_Free({ptr, sizeof(T)});
}
template <typename T>
TBlob<T> Mem_NewArray_NoConstruct (size_t count) {
    auto blob = Mem_Alloc(sizeof(T) * count);
    return {blob.as<void>(), blob.len};
}
template <typename T>
void Mem_DeleteArray_NoDestruct (TBlob<T> blob) {
    Mem_Free({blob.ptr, blob.len * sizeof(T)});
}

template <typename T, typename ... ArgTypes>
T * Mem_New (ArgTypes && ... args) {
    auto ret = Mem_New_NoConstruct<T>();
    if (ret) {
        try {
            new (ret) T (std::forward<ArgTypes>(args)...);
        } catch (...) {
            ret->~T();
            Mem_Delete_NoDestruct(ret);
            ret = nullptr;
        }
    }
    return ret;
}
template <typename T>
void Mem_Delete (T * ptr) {
    if (ptr) {
        ptr->~T();
        Mem_Delete_NoDestruct(ptr);
    }
}
template <typename T>
TBlob<T> Mem_NewArray (size_t count) {
}
template <typename T>
void Mem_DeleteArray (TBlob<T> blob) {
}
#pragma endregion

#pragma region "Memory utilities (compare, etc.)"
bool Mem_Equal (CBlob const & mem1, CBlob const & mem2);

bool  Mem_IsOverlapped (void const * mem1, size_t bytes1, void const * mem2, size_t bytes2);

void * Mem_Copy (void * MFA_RESTRICT dest, void const * MFA_RESTRICT source, size_t size_bytes);

void * Mem_Fill (void * dest, unsigned char fill_value, size_t size_bytes);

void * Mem_Zero (void * dest, size_t size_bytes);

bool Mem_Equal (void const * ptr1, void const * ptr2, size_t size_bytes);
#pragma endregion

#pragma region "Memory utilities (compare, etc.)"
size_t
Str_Length (char const * str);

size_t
Str_Len (char const * str);

int
Str_Compare (char const * s1, char const * s2);

bool
Str_Equal (char const * s1, char const * s2);

// Returns the number of bytes written, including the NUL.
// Also, even if "src" is NULL, will write the NUL to "dst".
size_t
Str_Copy (char * dst, size_t dst_size, char const * src);

template <size_t N>
size_t
Str_Copy (char (&dst) [N], char const * src) {
    return Str_Copy(dst, N, src);
}
#pragma endregion

    namespace Internal {

void * MemCopy_Generic1 (
    void * MFA_RESTRICT dest,
    void const * MFA_RESTRICT source,
    size_t size_bytes
);

void * MemCopy_Generic2 (
    void * MFA_RESTRICT dest,
    void const * MFA_RESTRICT source,
    size_t size_bytes
);

void * MemCopy_Assembly1 (
    void * MFA_RESTRICT dest,
    void const * MFA_RESTRICT source,
    size_t size_bytes
);

void * MemCopy_Assembly2 (
    void * MFA_RESTRICT dest,
    void const * MFA_RESTRICT source,
    size_t size_bytes
);

void * MemCopy_SSE1 (
    void * MFA_RESTRICT dest,
    void const * MFA_RESTRICT source,
    size_t size_bytes
);

void * MemCopy_SSE2 (
    void * MFA_RESTRICT dest, 
    void const * MFA_RESTRICT source, 
    size_t size_bytes
);

void * MemCopy_SSE4 (void * MFA_RESTRICT dest, void const * MFA_RESTRICT source, size_t size_bytes);

void * MemCopy_SSE8 (void * MFA_RESTRICT dest, void const * MFA_RESTRICT source, size_t size_bytes);

void * MemCopy_SSE1_NT (void * MFA_RESTRICT dest, void const * MFA_RESTRICT source, size_t size_bytes);

void * MemCopy_SSE2_NT (void * MFA_RESTRICT dest, void const * MFA_RESTRICT source, size_t size_bytes);

void * MemCopy_SSE4_NT (void * MFA_RESTRICT dest, void const * MFA_RESTRICT source, size_t size_bytes);

void * MemCopy_SSE8_NT (void * MFA_RESTRICT dest, void const * MFA_RESTRICT source, size_t size_bytes);

void * MemCopy_AVX2 (void * MFA_RESTRICT dest, void const * MFA_RESTRICT source, size_t size_bytes);

void * MemCopy_AVX2_NT (void * MFA_RESTRICT dest, void const * MFA_RESTRICT source, size_t size_bytes);

    }   // namespace Internal

inline void * // Good for sizes up to ~8K (in 2020)
Mem_CopySmall (void * MFA_RESTRICT dest, void const * MFA_RESTRICT source, size_t size_bytes) {
    return Internal::MemCopy_SSE2(dest, source, size_bytes);
}

inline void * // Good for sizes up to ~2-3M (in 2020)
Mem_CopyMedium (void * MFA_RESTRICT dest, void const * MFA_RESTRICT source, size_t size_bytes) {
    return Internal::MemCopy_Assembly1(dest, source, size_bytes);
}

inline void * // Doesn't write to cache; specially good for large copies (megabytes.)
Mem_CopyLarge (void * MFA_RESTRICT dest, void const * MFA_RESTRICT source, size_t size_bytes) {
    return Internal::MemCopy_SSE2_NT(dest, source, size_bytes);
}

/**** OS-Dependent Allocation Functions **********/

constexpr size_t
Mem_PageSize () {
    return 64 * 1024;   // NOTE: Do NOT change this! Much code might rely on it...
                        // NOTE: If you rely on this size, static_assert() it!
}

Blob Mem_AllocPages (size_t size_bytes);  // Will be rounded up to page size

bool Mem_FreePages (Blob mem);


/**** Allocators *********************************/

// TODO: Make this thread-safe.
// TODO: Add a preprocessor macro to disable leak/error checking.
class GP_Allocator
{
public:
    enum class State
    {
        INVALID,
        ALLOCATED,
        FREE
    };

private:
    static constexpr char signature[4] = { 'Y','B','L','K' };
    static constexpr char header_sentinel[4] = { 'C','O','D','E' };
    static constexpr char footer_sentinel[16] = { 'C','O','D','E','C','O','D','E','C','O','D','E','C','O','D','E' };

    struct Header
    {
        char signature[4];
        State state;
        uint64_t size;
        Header* next;
        Header* prev;
        char const* file;
        char const* func;
        char const* system;
        int line;
        char sentinel[4];
    };
    static_assert(sizeof(Header) == 64, "");

    struct Footer
    {
        char sentinel[16];
    };
    static_assert(sizeof(Footer) == 16, "");

public:
    GP_Allocator ();
    ~GP_Allocator ();
    Blob alloc (size_t size, char const* file, int line, char const* func, char const* system);
    void dealloc (Blob const& mem, char const* file, int line, char const* func, char const* system);
    
    int print_block_list(bool do_print = true) const;

private:
    static Blob payload_from_header (Header * header);
    static Footer* footer_from_header (Header * header);
    static Header* header_from_payload (void * ptr);

private:
    Header* m_first_header = nullptr;
};



class Allocator {
public:
    enum class Caps {
        None                = 0,

        NamedInstance       = 1,        // Instances of this allocator have their own names
        LimitedSize         = 2,        // This allocator has an upper bound on total size
        Realloc             = 4,        // supports good realloc
        Stats               = 8,        // gathers stats
        Report              = 16,       // supports detailed reporting of (de)allocs
        LeakCheck           = 32,       // supports leak detection
        OwnershipTest       = 64,       // can say whether it owns a particular pointer or range
        ThreadSafe          = 128,      // is thread-safe
        SizeCheck           = 256,      // knows each block size and can check upon freeing
        Aligned             = 512,      // supports (good) aligned allocation
        AlignedOffset       = 1024,     // also supports offsetted aligned allocation
        SupportsUpstream    = 2048,     // supports forwarding requests to an upstream allocator (or gets its memory from it)
        BlockAddrFromPtr    = 4096,     // can find a block's start address from any pointer within that block
        SupportsReset       = 8192,     // can release all the allocations at once (without destructor call perhaps)
        ResetIsFast         = 16384,    // the "reset" operation is fast; probably O(1)
    };
    friend Caps operator | (Caps a, Caps b) {return Caps(int(a) | int(b));}
    friend Caps operator & (Caps a, Caps b) {return Caps(int(a) & int(b));}

public:
    struct DbgInfo {
        char const * sys;
        char const * func;
        char const * file;
        int line;
        int alloc_num;
    };

    struct Stats {
        uint64_t alloc_calls;
        uint64_t realloc_calls;
        uint64_t free_calls;
        uint64_t total_requested_bytes;
        uint64_t total_freed_bytes;
        uint64_t most_total_allocated_bytes;
        uint64_t slack_bytes;
        uint64_t overhead_bytes;
    };

public:
    explicit Allocator (Allocator * upstream) : m_upstream {upstream} {}
    virtual ~Allocator () = default;
    
    Allocator * upstream () {return m_upstream;}

    virtual char const * name_of_class () const = 0;
    virtual Blob alloc (size_t size) = 0;
    virtual void dealloc (Blob const & mem) = 0;
    virtual bool owns_range (CBlob const & range) const = 0;
    virtual bool reset () = 0;

    virtual Blob alloc (size_t size, DbgInfo const & dbg_info);
    virtual void dealloc (Blob const & mem, DbgInfo const & dbg_info);

    virtual Caps capabilities () const;
    virtual bool stats (Stats * out_stats) const;
    virtual char const * name_of_instance () const;
    virtual Blob alloc_and_zero (size_t size);
    virtual Blob alloc_and_zero (size_t size, DbgInfo const & dbg_info);
    virtual Blob alloc_aligned (size_t size, size_t alignment, size_t offset, Blob * out_aligned_blob);
    virtual Blob alloc_aligned (size_t size, size_t alignment, size_t offset, Blob * out_aligned_blob, DbgInfo const & dbg_info);
    virtual Blob realloc (Blob const & mem, size_t new_size);
    virtual Blob realloc (Blob const & mem, size_t new_size, DbgInfo const & dbg_info);
    virtual bool owns_pointer (void const * ptr) const;
    virtual size_t block_size (void const * ptr) const;


    // TODO(yzt): Add some kind of iteration over allocated blocks... Maybe.

private:
    Allocator * m_upstream = nullptr;
};

    namespace Allocators {

class CRT
    : public Allocator
{
public:
    explicit CRT (char const * instance_name);
    virtual ~CRT ();

    virtual char const * name_of_class () const override;
    virtual Blob alloc (size_t size) override;
    virtual void dealloc (Blob const & mem) override;
    virtual bool owns_range (CBlob const & range) const override;
    virtual bool reset () override;

    virtual Caps capabilities () const override;
    virtual bool stats (Stats * out_stats) const override;
    virtual char const * name_of_instance () const override;
    virtual Blob realloc (Blob const & mem, size_t new_size) override;

private:
    char const * m_instance_name = nullptr;
    Stats m_stats = {};
};

class CRTBasedGeneralPurpose
    : public Allocator
{
public:

private:
};

class SlowGeneralPurpose
    : public Allocator
{
private:
    using Offset = uint32_t;
    enum class State : uint8_t {
        Free,
        Allocated,
        Wastage,
        Sentinel,
    };
    struct Header {
        uint32_t cookie_first;
        State state;
        uint8_t padding1 [3];
        Offset size;
        Offset neighbor_size;   // left neighbor, i.e. the block immediately to the left in memory
        Offset next_free;
        Offset prev_free;
        char const * func;
        char const * name;
        char const * file;
        uint32_t line;
        uint8_t padding2 [8];
        uint32_t cookie_second;
    };
    static_assert(sizeof(Header) == 64, "");

    static constexpr uint32_t Cookie1 = 0x1232'1878;
    static constexpr uint32_t Cookie2 = 0x1232'1878;

public:
    static constexpr Offset AlignShift = 6;
    static constexpr Offset Align = 1 << AlignShift;
    static constexpr Offset AlignMask = Align - 1;
    static_assert(Align == sizeof(Header), "");
    static constexpr uint64_t LargestPossibleSize = uint64_t(0xFFFF'FFFFU) << AlignShift;
    static constexpr Offset SmallestUsefulSize = 2 * Align;

public:
    explicit SlowGeneralPurpose (char const * instance_name, Blob const & mem);
    virtual ~SlowGeneralPurpose ();

    virtual char const * name_of_class () const override;
    virtual Blob alloc (size_t size) override;
    virtual void dealloc (Blob const & mem) override;
    virtual bool owns_range (CBlob const & range) const override;
    virtual bool reset () override;

    virtual Caps capabilities () const override;
    virtual bool stats (Stats * out_stats) const override;
    virtual char const * name_of_instance () const override;

    bool internal_sanity_check (bool report_leaks, bool report_all_blocks);

private:
    Header * put_header (uint64_t abs_offset, State state, uint64_t size, uint64_t neighbor_offset);

    static Byte * align_up (Byte * p) {return (Byte *)(((intptr_t(p) + AlignMask) >> AlignShift) << AlignShift);}
    static size_t align_down (size_t s) {return s & AlignMask;}
    static Offset make_size (size_t s) {return Offset((s + AlignMask) >> AlignShift);}
    Header * header_at (Offset offset) {return reinterpret_cast<Header *>(m_aligned_mem.ptr) + offset;}
    void header_init (Header * hdr, State state, Offset size, Offset neighbor_size);
    Offset find_suitable_free_block (Offset size);
    Offset make_offset (void const * ptr) const {
        Offset ret = 0;
        if (
            ptr >= m_aligned_mem.ptr &&
            ptr < m_aligned_mem.ptr + m_aligned_mem.len &&
            0 == ((intptr_t(ptr) - intptr_t(m_aligned_mem.ptr)) & AlignMask)
        ) {
            ret = Offset((intptr_t(ptr) - intptr_t(m_aligned_mem.ptr)) >> AlignShift);
        }
        return ret;
    }
private:
    void * m_mutex = nullptr;
    Blob m_mem = {};
    Blob m_aligned_mem = {};
    Offset m_first_free = 0;
    char const * m_instance_name = nullptr;
    Stats m_stats = {};
};

/*
 * TODO(yzt): This is not thread-safe AT ALL! Probably have to make it so. Or use one of these per-thread?
 *            There is a middle ground though, between full multi-threaded use, and exclusively single-threaded.
 *            Suppose that we did have one of these per-thread. Then each thread would only ever allocate from
 *            its own allocator. But freeing a piece of memory can happen from any thread. So, only make the
 *            access to the "ref_count" in each block thread-safe.
 *            There is one issue though: who will reclaim a block that has reached a ref_cound of 0 then, and how?
 * TODO(yzt): Gather statistics (the commented fields at the end of the class.)
 * TODO(yzt): Add debug facilities:
 *              - debug info in each block (e.g. __FILE__ and __LINE__ and __func__)
 *              - decide on how to pass debug info into the allocate/free functions
 *              - timestamp in each chunk (to gather and analyze lifetime of chunks)
 *              - Detailed leak report upon the user asking and/or destruction of the allocator.
 */
class SmallShortLivedChunked
    : NonCopyableNonMovable
{
    using ChunkSizeType = uint32_t;
    using BlockSizeType = uint16_t;

    struct ChunkHeader {
        static const uint32_t CorrectSignature = 'SSLC';

        SmallShortLivedChunked * owner;
        ChunkHeader * prev; // previous chunk, chronologically
        ChunkHeader * next; // next chunk, chronologically
        ChunkSizeType total_bytes;
        ChunkSizeType used_bytes;
        int32_t ref_count;
        uint32_t signature;          // Should be 'SSLC', i.e. SmallShortLivedChunk
    };
    static_assert(sizeof(ChunkHeader) == 40);

    struct BlockHeader {
        BlockSizeType offset_in_chunk;  // offset of **block header** in chunk
        BlockSizeType block_size;
    };
    static_assert(sizeof(BlockHeader) == 4);

    static constexpr size_t ChunkSize = Mem_PageSize();
    static constexpr size_t LargestPossibleBlock = ChunkSize - sizeof(ChunkHeader) - sizeof(BlockHeader);
    static constexpr size_t LargestPossibleBlockIncOverhead = ChunkSize - sizeof(ChunkHeader);

    static_assert(ChunkSize > sizeof(ChunkHeader) + sizeof(BlockHeader));
    static_assert(LargestPossibleBlock < (1ULL << (8 * sizeof(BlockHeader::block_size))));

public:
    SmallShortLivedChunked () = default;
    ~SmallShortLivedChunked ();

    Blob allocate (size_t size);
    bool free (Blob mem);
    
    bool owns (void * ptr) const;
    size_t block_size (void * ptr) const;

    // In effect, just increments the ref_count of the chunk that contains this block/blob (iff it is valid, that is.)
    Blob acquire_block (Blob existing_block_that_was_allocated_by_this_allocator);

private:
    bool ensure_availability (size_t size_inc_overhead);
    bool acquire_new_chunk ();
    bool release_old_chunk (ChunkHeader * chunk);

    static BlockHeader * block_header_from_block (void * block) {
        return reinterpret_cast<BlockHeader *>(block) - 1;
    }
    static ChunkHeader * chunk_header_from_block (void * block) {
        auto bh = block_header_from_block(block);
        auto ch = reinterpret_cast<ChunkHeader *>(
            reinterpret_cast<Byte *>(bh) - bh->offset_in_chunk
        );
        return ch;
    }

private:
    ChunkHeader * m_last_chunk = nullptr;
    //uint64_t m_bytes_acquired = 0;
    //uint64_t m_bytes_allocated = 0;
    //uint64_t m_bytes_released = 0;
    //uint64_t m_chunks_allocated = 0;
    //uint64_t m_chunks_released = 0;
};


template <size_t ObjectSize_>
class FixedSizeObjectPool {
public:
    using Index = size_t;
    static constexpr Index InvalidIndex = ~(Index)0;
    static constexpr size_t ObjectSize = ObjectSize_;
    static constexpr size_t GrowConst = 128;
public:
    explicit FixedSizeObjectPool (Blob const & mem)
        : m_mem (mem)
        , m_size (0)
        , m_first_free {InvalidIndex}
    {
        MFA_ASSERT(m_mem.ptr && m_mem.len >= ObjectSize);
    }
    ~FixedSizeObjectPool () {
        reset();
    }
    void * alloc () {
        if (InvalidIndex == m_first_free) {
            if (m_size < m_mem.len / ObjectSize) {
                auto new_size = Min(m_size + GrowConst, m_mem.len / ObjectSize);
                for (auto i = m_size; i < new_size - 1; ++i) {
                    *free_slot(i) = i + 1;
                }
                m_first_free = m_size;
                *free_slot(new_size - 1) = InvalidIndex;
            }
        }
        if (InvalidIndex != m_first_free) {
            auto ret_idx = m_first_free;
            m_first_free = *free_slot(m_first_free);
            return m_mem.ptr + ret_idx * ObjectSize;
        } else {
            return nullptr;
        }
    }
    void dealloc (void * ptr) {
        MFA_ASSERT(owns_pointer(ptr));
        Index idx = ((Byte *)ptr - m_mem.ptr) / ObjectSize;
        *free_slot(idx) = m_first_free;
        m_first_free = idx;
    }
    bool owns_pointer (void const * ptr) const {
        bool ret = false;
        if (ptr >= m_mem.ptr && ptr < m_mem.ptr + m_mem.len) {
            if ((((Byte *)ptr - m_mem.ptr) % ObjectSize) == 0)
                ret = true;
        }
        return ret;
    }
    void reset () {
        m_size = 0;
        m_first_free = InvalidIndex;
    }
private:
    Index * free_slot (Index index) {
        return reinterpret_cast<Index *>(m_mem.ptr + index * ObjectSize);
    }
private:
    Blob m_mem;
    size_t m_size;
    size_t m_first_free;

    static_assert(ObjectSize >= sizeof(Index));
};

    }   // namespace Allocators
}   // namespace Yugen

#endif MEMORY_HEADER
#endif