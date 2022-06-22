#include "BedrockFileSystem.hpp"

#include "BedrockCommon.hpp"
#include "BedrockAssert.hpp"

#include <filesystem>
#include <fstream>

namespace MFA::FileSystem {

#ifdef __ANDROID__
    static android_app * mApp = nullptr;
#endif
    // TODO: Maybe we should use modern c++ instead

    FileHandle::FileHandle(std::string const & path, Usage const usage) {
        close();
        auto const * const mode = [=]{
            switch (usage)
            {
                case Usage::Read: return "rb";
                case Usage::Write: return "wb";
                case Usage::Append: return "r+b";
                default: return "";
            }
        }();
        #ifndef __PLATFORM_WIN__
        int errorCode = 0;
        mFile = fopen(path.c_str(), mode);
        if (mFile == nullptr) {
            errorCode = 1;
        }
        #else
        auto errorCode = ::fopen_s(&mFile, path.c_str(), mode);
        #endif
        if(errorCode == 0) {
            if (mFile != nullptr && Usage::Append == usage) {
                auto const seekResult = seekToEnd();
                MFA_ASSERT(seekResult);
            }
        }
        if (mFile == nullptr && Usage::Append == usage) {
            #ifndef __PLATFORM_WIN__
            errorCode = 0;
            auto mFile = fopen(path.c_str(), mode);
            if (mFile != nullptr) {
                errorCode = 1;
            }
            #else
            errorCode = ::fopen_s(&mFile, path.c_str(), "w+b");
            #endif
        }
        if(errorCode != 0) {
            mFile = nullptr;
        }
    }

    FileHandle::~FileHandle() {
        close();
    }

    bool FileHandle::isOk() const {
        bool ret = false;
        if(mFile != nullptr) {
            ret = true;
        }
        return ret;
    }

    bool FileHandle::close() {
        bool ret = false;
        if (isOk()) {
            // TODO Should we check close result
            ::fclose(static_cast<FILE*>(mFile));
            mFile = nullptr;
            ret = true;
        }
        return ret;    
    }

    bool FileHandle::seek(const int offset, const Origin origin) const {
        bool ret = false;
        if (isOk()) {
            int _origin {};
            switch(origin) {
                case Origin::Start:
                _origin = 0;
                break;
                case Origin::End:
                _origin = SEEK_END;
                break;
                case Origin::Current:
                _origin = SEEK_CUR;
                break;
                default:
                MFA_CRASH("Invalid origin");
            }
            int const seek_result = ::fseek(mFile, offset, _origin);
            ret = (0 == seek_result);
        }
        return ret;
    }

    bool FileHandle::seekToEnd() const {
        return seek(0, Origin::End);
    }

    bool FileHandle::tell(int64_t & outLocation) const {
        bool success = false;
        if (isOk()) {
            outLocation = ::ftell(mFile);
            success = true;
        }
        return success;
    }

    size_t FileHandle::read (Blob const & memory) const {
        size_t ret = 0;
        if (isOk() && memory.len > 0) {
            ret = ::fread(memory.ptr, 1, memory.len, mFile);
        }
        return ret;
    }

    size_t FileHandle::write (CBlob const & memory) const {
        size_t ret = 0;
        if (isOk() && memory.len > 0)
        {
            ret = ::fwrite(memory.ptr, 1, memory.len, mFile);
        }
        return ret;
    }

    uint64_t FileHandle::size () const {
        uint64_t ret = 0;
        if (isOk()) {
            fpos_t pos, end_pos;
            ::fgetpos(mFile, &pos);
            ::fseek(mFile, 0, SEEK_END);
            ::fgetpos(mFile, &end_pos);
            ::fsetpos(mFile, &pos);
        #ifdef __PLATFORM_LINUX__
            ret = static_cast<uint64_t>(end_pos.__pos);
        #else
            ret = static_cast<uint64_t>(end_pos);
        #endif
        }
        return ret;
    }

    FILE * FileHandle::getHandle() const {
        return mFile;
    }

    bool Exists(std::string const & path) {
        return std::filesystem::exists(path);
    }

    std::shared_ptr<FileHandle> OpenFile(std::string const & path, Usage const usage) {
        auto file = std::make_shared<FileHandle>(path, usage);
        if (file->isOk()) {
            return file;
        }
        return nullptr;
    }

    bool FileIsUsable(FileHandle * file) {
        return file != nullptr && file->isOk();
    }

#ifdef __ANDROID__
    // TODO: Change to pointer
    void SetAndroidApp(android_app * androidApp) {
        MFA_ASSERT(androidApp != nullptr);
        mApp = androidApp;
    }

    AndroidAssetHandle::AndroidAssetHandle(std::string const & path) {
        close();
        mFile = AAssetManager_open(
            mApp->activity->assetManager,
            path,
            AASSET_MODE_BUFFER
        );
    }

    AndroidAssetHandle::~AndroidAssetHandle() {close();}

    [[nodiscard]]
    bool AndroidAssetHandle::isOk() const {
        bool ret = false;
        if(mFile != nullptr) {
            ret = true;
        }
        return ret;
    }

    bool AndroidAssetHandle::close() {
        bool ret = false;
        if (isOk()) {
            AAsset_close(mFile);
            mFile = nullptr;
            ret = true;
        }
        return ret;
    }

    [[nodiscard]]
    bool AndroidAssetHandle::seek(const int offset, const Origin origin) const {
        bool success = false;
        if (isOk()) {
            int _origin {};
            switch(origin) {
                case Origin::Start:
                    _origin = 0;
                    break;
                case Origin::End:
                    _origin = SEEK_END;
                    break;
                case Origin::Current:
                    _origin = SEEK_CUR;
                    break;
                default:
                    MFA_CRASH("Invalid origin");
            }
            auto const seekResult = AAsset_seek(mFile, offset, _origin);
            success = (seekResult >= 0);
        }
        return success;
    }

    [[nodiscard]]
    bool AndroidAssetHandle::seekToEnd() const {
        return seek(0, Origin::End);
    }

    [[nodiscard]]
    uint64_t AndroidAssetHandle::read (Blob const & memory) const {
        uint64_t ret = 0;
        if (isOk() && memory.len > 0) {
            ret = AAsset_read(mFile, memory.ptr, memory.len);
        }
        return ret;
    }

    [[nodiscard]]
    uint64_t AndroidAssetHandle::totalSize () const {
        uint64_t totalSize = 0;
        if (isOk()) {
            totalSize = AAsset_getLength64(mFile);
        }
        return totalSize;
    }

    bool AndroidAssetHandle::tell(int64_t & outLocation) const {
        bool success = false;
        if (isOk()) {
            outLocation = totalSize() - AAsset_getRemainingLength64(mFile);
            success = true;
        }
        return success;
    }

    AndroidAssetHandle * Android_OpenAsset(std::string const & path) {
        // TODO Use allocator system
        auto * handle = new AndroidAssetHandle {path};
        return handle;
    }

    bool Android_CloseAsset(AndroidAssetHandle * file) {
        bool ret = false;
        if(file != nullptr) {
            file->close();
            delete file;
            ret = true;
        }
        return ret;
    }

    size_t Android_AssetSize(AndroidAssetHandle * file) {
        size_t result = 0;
        if (file != nullptr) {
            result = file->totalSize();
        }
        return result;
    }

    uint64_t Android_ReadAsset(AndroidAssetHandle * file, Blob const & memory) {
        uint64_t readCount = 0;
        if (file != nullptr) {
            readCount = file->read(memory);
        }
        return readCount;
    }

    bool Android_AssetIsUsable(AndroidAssetHandle * file) {
        return file != nullptr && file->isOk();
    }

    bool Android_Seek(AndroidAssetHandle * file, int offset, Origin origin) {
        return file != nullptr && file->seek(offset, origin);
    }

    bool Android_Tell(AndroidAssetHandle * file, int64_t &outLocation) {
        if (file != nullptr) {
            return file->tell(outLocation);
        }
        return false;
    }

#endif

}
