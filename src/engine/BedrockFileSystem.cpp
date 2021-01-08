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
    auto error_code = ::fopen_s(&m_file, path, mode);
    if(error_code == 0) {
        if (MFA_PTR_VALID(m_file) && Usage::Append == usage) {
            auto const seek_result = seek_to_end();  MFA_ASSERT(seek_result); MFA_CONSUME_VAR(seek_result);    
        }
    }
    if (!MFA_PTR_VALID(m_file) && Usage::Append == usage) {
        error_code = ::fopen_s(&m_file, path, "w+b");
    }
    if(error_code != 0) {
        m_file = nullptr;
    }
}

~FileHandle() {close();}

[[nodiscard]]
bool is_ok() const {
    bool ret = false;
    if(true == MFA_PTR_VALID(m_file)) {
        ret = true;
    }
    return ret;
}

bool close() {
    bool ret = false;
    if (is_ok()) {
        // TODO Should we check close result
        ::fclose(static_cast<FILE*>(m_file));
        m_file = nullptr;
        ret = true;
    }
    return ret;    
}

bool seek_to_end() const {
    bool ret = false;
    if (is_ok()) {
        int const seek_result = ::fseek(m_file, 0, SEEK_END);
        ret = (0 == seek_result);
    }
    return ret;
}
[[nodiscard]]
uint64_t read (Blob const & memory) const {
    uint64_t ret = 0;
    if (is_ok() && memory.len > 0) {
        ret = ::fread(memory.ptr, 1, memory.len, m_file);
    }
    return ret;
}

// TODO: Write

[[nodiscard]]
uint64_t total_size () const {
    uint64_t ret = 0;
    if (is_ok()) {
        fpos_t pos, end_pos;
        ::fgetpos(m_file, &pos);
        ::fseek(m_file, 0, SEEK_END);
        ::fgetpos(m_file, &end_pos);
        ::fsetpos(m_file, &pos);
        ret = static_cast<uint64_t>(end_pos);
    }
    return ret;
}

// TODO Delete move/copy constructor
public:
    FILE * m_file = nullptr;
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
    if(MFA_PTR_VALID(file)) {
        file->close();
        delete file;
        ret = true;
    }
    return ret;
}

size_t FileSize(FileHandle * file) {
    size_t ret = 0;
    if(MFA_PTR_VALID(file)) {
        ret = file->total_size();
    }
    return ret;
}

uint64_t Read(FileHandle * file, Blob const & memory) {
    uint64_t ret = 0;
    if(MFA_PTR_VALID(file)) {
        ret = file->read(memory);
    }
    return ret;
}

bool IsUsable(FileHandle * file) {
    return MFA_PTR_VALID(file) && file->is_ok();
}

}
