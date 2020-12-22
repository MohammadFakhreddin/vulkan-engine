#ifndef FILE_SYSTEM_HEADER
#define FILE_SYSTEM_HEADER

#include "BedrockCommon.hpp"

namespace MFA::FileSystem {

enum class Usage {
    Read,
    Write,
    Append,
};

class File;

[[nodiscard]]
bool Exists(char const * path);

[[nodiscard]]
File * OpenFile(char const * path, Usage usage);

bool CloseFile(File * file);

[[nodiscard]]
size_t FileSize(File * file);

uint64_t Read(File * file, Blob const & memory);

};

#endif
