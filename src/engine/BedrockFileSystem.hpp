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

    class FileHandle;

    [[nodiscard]]
    bool Exists(std::string const & path);

    // TODO Use shared_ptr instead
    [[nodiscard]]
    FileHandle * OpenFile(std::string const & path, Usage usage);

    bool CloseFile(FileHandle * file);

    [[nodiscard]]
    size_t FileSize(FileHandle * file);

    uint64_t Read(FileHandle * file, Blob const & memory);

    [[nodiscard]]
    bool FileIsUsable(FileHandle * file);

    [[nodiscard]]
    FILE * GetCHandle(FileHandle * file);

    enum class Origin
    {
        Start,
        End,
        Current
    };

    [[nodiscard]]
    bool Seek(FileHandle * file, int offset, Origin origin);

    [[nodiscard]]
    bool Tell(FileHandle * file, int64_t & outLocation);

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
