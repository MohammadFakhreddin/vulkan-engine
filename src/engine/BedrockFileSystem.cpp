#include "BedrockFileSystem.hpp"

#include "BedrockCommon.hpp"
#include "BedrockAssert.hpp"

#include <filesystem>
#include <fstream>

namespace MFA::FileSystem {

#ifdef __ANDROID__
    static android_app * mApp = nullptr;
#endif

class FileHandle {
public:

explicit FileHandle(char const * path, Usage const usage) {
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
    mFile = fopen(path, mode);
    if (mFile == nullptr) {
        errorCode = 1;
    }
    #else
    auto errorCode = ::fopen_s(&mFile, path, mode);
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
        auto mFile = fopen(path, mode);
        if (mFile != nullptr) {
            errorCode = 1;
        }
        #else
        errorCode = ::fopen_s(&mFile, path, "w+b");
        #endif
    }
    if(errorCode != 0) {
        mFile = nullptr;
    }
}

~FileHandle() {close();}

[[nodiscard]]
bool isOk() const {
    bool ret = false;
    if(mFile != nullptr) {
        ret = true;
    }
    return ret;
}

bool close() {
    bool ret = false;
    if (isOk()) {
        // TODO Should we check close result
        ::fclose(static_cast<FILE*>(mFile));
        mFile = nullptr;
        ret = true;
    }
    return ret;    
}

[[nodiscard]]
bool seek(const int offset, const Origin origin) const {
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

[[nodiscard]]
bool seekToEnd() const {
    return seek(0, Origin::End);
}

bool tell(int64_t & outLocation) const {
    bool success = false;
    if (isOk()) {
        outLocation = ::ftell(mFile);
        success = true;
    }
    return success;
}

[[nodiscard]]
uint64_t read (Blob const & memory) const {
    uint64_t ret = 0;
    if (isOk() && memory.len > 0) {
        ret = ::fread(memory.ptr, 1, memory.len, mFile);
    }
    return ret;
}

// TODO: Write

[[nodiscard]]
uint64_t totalSize () const {
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

// TODO Delete move/copy constructor
public:
    FILE * mFile = nullptr;
};

bool Exists(char const * path) {
    return std::filesystem::exists(path);
}

FileHandle * OpenFile(char const * path, Usage const usage) {
    // TODO Use allocator system
    auto * file = new FileHandle {path, usage};
    return file;
}

bool CloseFile(FileHandle * file) {
    bool ret = false;
    if(file != nullptr) {
        file->close();
        delete file;
        ret = true;
    }
    return ret;
}

size_t FileSize(FileHandle * file) {
    size_t ret = 0;
    if(file != nullptr) {
        ret = file->totalSize();
    }
    return ret;
}

uint64_t Read(FileHandle * file, Blob const & memory) {
    uint64_t ret = 0;
    if(file != nullptr) {
        ret = file->read(memory);
    }
    return ret;
}

bool FileIsUsable(FileHandle * file) {
    return file != nullptr && file->isOk();
}

FILE * GetCHandle(FileHandle * file) {
    FILE * ret {};
    if(file != nullptr) {
        ret = file->mFile;
    }
    return ret;
}

std::string ExtractDirectoryFromPath(char const * path)  {
    MFA_ASSERT(path != nullptr);
    return std::filesystem::path(path).parent_path().string();
}

std::string ExtractExtensionFromPath(char const * path) {
    MFA_ASSERT(path != nullptr);
    return std::filesystem::path(path).extension().string();
}

bool Seek(FileHandle * file, const int offset, const Origin origin) {
    MFA_ASSERT(file != nullptr);
    return file->seek(offset, origin);
}

bool Tell(FileHandle * file, int64_t & outLocation) {
    MFA_ASSERT(file != nullptr);
    return file->tell(outLocation);
}

#ifdef __ANDROID__
void SetAndroidApp(android_app * androidApp) {
    MFA_ASSERT(androidApp != nullptr);
    mApp = androidApp;
}

AndroidAssetHandle::AndroidAssetHandle(char const * path) {
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

AndroidAssetHandle * Android_OpenAsset(char const * path) {
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
