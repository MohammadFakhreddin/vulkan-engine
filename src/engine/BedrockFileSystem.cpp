#include "BedrockFileSystem.hpp"

#include "BedrockCommon.hpp"
#include "BedrockAssert.hpp"

#include <filesystem>
#include <fstream>

namespace MFA::FileSystem {

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
    #ifdef __PLATFORM_MAC__
    int errorCode = 0;
    auto mFile = fopen(path, mode);
    if (mFile != nullptr) {
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
        #ifdef __PLATFORM_MAC__
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
        ret = static_cast<uint64_t>(end_pos);
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

bool IsUsable(FileHandle * file) {
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

bool Seek(FileHandle * file, const int offset, const Origin origin) {
    MFA_ASSERT(file != nullptr);
    return file->seek(offset, origin);
}

bool Tell(FileHandle * file, int64_t & outLocation) {
    MFA_ASSERT(file != nullptr);
    return file->tell(outLocation);
}

}
