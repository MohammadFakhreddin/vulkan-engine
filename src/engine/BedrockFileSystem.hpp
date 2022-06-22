#pragma once

#include <string>

#include "BedrockCommon.hpp"

#ifdef __ANDROID__
#include <android_native_app_glue.h>
#endif

namespace MFA::FileSystem
{

    enum class Usage
    {
        Read,
        Write,
        Append,
    };

    enum class Origin
    {
        Start,
        End,
        Current
    };

    class FileHandle {
    public:

        explicit FileHandle(std::string const & path, Usage const usage);

        ~FileHandle();

        FileHandle(FileHandle const &) noexcept = delete;
        FileHandle(FileHandle &&) noexcept = delete;
        FileHandle & operator= (FileHandle const & rhs) noexcept = delete;
        FileHandle & operator= (FileHandle && rhs) noexcept = delete;

        [[nodiscard]]
        bool isOk() const;

        bool close();

        [[nodiscard]]
        bool seek(const int offset, const Origin origin) const;

        [[nodiscard]]
        bool seekToEnd() const;

        bool tell(int64_t & outLocation) const;

        // Return read count from file
        size_t read (Blob const & memory) const;

        // Returns the write count into file
        size_t write(CBlob const & memory) const;

        [[nodiscard]]
        uint64_t size () const;

        [[nodiscard]]
        FILE * getHandle() const;

    private:

        FILE * mFile = nullptr;
        
    };

    [[nodiscard]]
    bool Exists(std::string const & path);

    // TODO Use shared_ptr instead
    [[nodiscard]]
    std::shared_ptr<FileHandle> OpenFile(std::string const & path, Usage usage);
    
    [[nodiscard]]
    bool FileIsUsable(FileHandle * file);
    
#ifdef __ANDROID__
    class AndroidAssetHandle
    {
    public:

        explicit AndroidAssetHandle(std::string const & path);

        ~AndroidAssetHandle();

        [[nodiscard]]
        bool isOk() const;

        bool close();

        [[nodiscard]]
        bool seek(const int offset, const Origin origin) const;

        [[nodiscard]]
        bool seekToEnd() const;

        bool tell(int64_t & outLocation) const;

        [[nodiscard]]
        uint64_t read(Blob const & memory) const;

        [[nodiscard]]
        uint64_t totalSize() const;

    public:
        AAsset * mFile = nullptr;
    };

    void SetAndroidApp(android_app * androidApp);

    [[nodiscard]]
    AndroidAssetHandle * Android_OpenAsset(std::string const & path);

    bool Android_CloseAsset(AndroidAssetHandle * file);

    [[nodiscard]]
    size_t Android_AssetSize(AndroidAssetHandle * file);

    uint64_t Android_ReadAsset(AndroidAssetHandle * file, Blob const & memory);

    [[nodiscard]]
    bool Android_AssetIsUsable(AndroidAssetHandle * file);

    [[nodiscard]]
    bool Android_Seek(AndroidAssetHandle * file, int offset, Origin origin);

    [[nodiscard]]
    bool Android_Tell(AndroidAssetHandle * file, int64_t & outLocation);

#endif


}; // MFA::FileSystem

namespace MFA
{
    namespace FS = FileSystem;
}
