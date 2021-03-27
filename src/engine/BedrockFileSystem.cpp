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
    // TODO Use FopenS
    auto errorCode = ::fopen_s(&mFile, path, mode);
    if(errorCode == 0) {
        if (mFile != nullptr && Usage::Append == usage) {
            auto const seekResult = seekToEnd();
            MFA_ASSERT(seekResult);
        }
    }
    if (mFile == nullptr && Usage::Append == usage) {
        errorCode = ::fopen_s(&mFile, path, "w+b");
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
bool seekToEnd() const {
    bool ret = false;
    if (isOk()) {
        int const seek_result = ::fseek(mFile, 0, SEEK_END);
        ret = (0 == seek_result);
    }
    return ret;
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

}
