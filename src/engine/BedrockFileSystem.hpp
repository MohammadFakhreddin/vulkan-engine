#pragma once

#include <string>

#include "BedrockCommon.hpp"

namespace MFA::FileSystem {

enum class Usage {
    Read,
    Write,
    Append,
};

class FileHandle;

[[nodiscard]]
bool Exists(char const * path);

[[nodiscard]]
FileHandle * OpenFile(char const * path, Usage usage);

bool CloseFile(FileHandle * file);

[[nodiscard]]
size_t FileSize(FileHandle * file);

uint64_t Read(FileHandle * file, Blob const & memory);

[[nodiscard]]
bool IsUsable(FileHandle * file);

[[nodiscard]]
FILE * GetCHandle(FileHandle * file);

std::string ExtractDirectoryFromPath(char const * path);

}; // MFA::FileSystem