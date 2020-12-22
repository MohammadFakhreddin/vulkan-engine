#include "FileSystem.hpp"

#include "BedrockCommon.hpp"

#include <filesystem>
#include <fstream>

#include "BedrockAssert.hpp"

namespace MFA::FileSystem {

class File {
public:

explicit File(char const * path, Usage const usage) {
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
    m_file = ::fopen(path, mode);
    if (!MFA_PTR_VALID(m_file) && Usage::Append == usage) {
        m_file = ::fopen(path, "w+b");
    }
    
    if (MFA_PTR_VALID(m_file) && Usage::Append == usage) {
        auto const seek_result = seek_to_end();  MFA_ASSERT(seek_result); MFA_CONSUME_VAR(seek_result);    
    }
}

~File() {close();}

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

uint64_t File::read (Blob const & memory) const {
    uint64_t ret = 0;
    if (is_ok() && memory.len > 0) {
        ret = ::fread(memory.ptr, 1, memory.len, m_file);
    }
    return ret;
}

// TODO: Write

[[nodiscard]]
uint64_t File::total_size () const {
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

[[nodiscard]]
File CreateFile(char const * path, Usage const usage) {
    return File {path, usage};
}

bool CloseFile(File * file) {
    bool ret = false;
    if(MFA_PTR_VALID(file)) {
        file->close();
        ret = true;
    }
    return ret;
}

[[nodiscard]]
size_t FileSize(File * file) {
    size_t ret = 0;
    if(MFA_PTR_VALID(file)) {
        ret = file->total_size();
    }
    return ret;
}

uint64_t Read(File * file, Blob const & memory) {
    uint64_t ret = 0;
    if(MFA_PTR_VALID(file)) {
        ret = file->read(memory);
    }
    return ret;
}

}
