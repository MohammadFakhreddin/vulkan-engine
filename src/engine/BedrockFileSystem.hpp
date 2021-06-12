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

[[nodiscard]]
std::string ExtractDirectoryFromPath(char const * path);

[[nodiscard]]
std::string ExtractExtensionFromPath(char const * path);

enum class Origin {
    Start,
    End,
    Current
};

[[nodiscard]]
bool Seek(FileHandle * file, int offset, Origin origin);

[[nodiscard]]
bool Tell(FileHandle * file, int64_t & outLocation);

}; // MFA::FileSystem