#ifndef IMPORTER_NAMESPACE
#define IMPORTER_NAMESPACE

#include "FoundationAsset.hpp"

namespace MFA::Importer {
//TODO:// After implementing Resource system return handle instead

//------------------------------Assets-------------------------------------

[[nodiscard]]
Asset::TextureAsset ImportUncompressedImage(char const * path);

// TODO
[[nodiscard]]
Asset::TextureAsset ImportDDSFile(char const * path);

[[nodiscard]]
Asset::MeshAsset ImportObj(char const * path);

[[nodiscard]]
Asset::MeshAsset ImportGLTF(char const * path);

// Temporary function for freeing imported assets
bool FreeAsset(Asset::GenericAsset * asset);

//------------------------------RawFile------------------------------------

struct RawFile {
    Blob data;
    [[nodiscard]]
    bool valid() const {
        return MFA_PTR_VALID(data.ptr);
    }
};

[[nodiscard]]
RawFile ReadRawFile(char const * path);

bool FreeRawFile (RawFile * raw_file);

}


#endif