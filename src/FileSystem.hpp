#ifndef FILE_SYSTEM_HEADER
#define FILE_SYSTEM_HEADER

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
File CreateFile(char const * path);

bool CloseFile(File * file);

[[nodiscard]]
size_t FileSize(File * file);

};

#endif
