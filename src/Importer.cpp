#include "Importer.hpp"


#include "BedrockMemory.hpp"
#include "FileSystem.hpp"

namespace MFA::Importer {

Asset::TextureAsset ImportUncompressedImage(char const * path) {
    
}

Asset::TextureAsset ImportDDSFile(char const * path) {
    // TODO
}

#if 0
Asset::MeshAsset ImportObj(char const * path) {
    using Byte = char;
    LoadObjResult ret {};
    if(std::filesystem::exists(path)){

        std::ifstream file {path};

        bool is_counter_clockwise = false;
        {//Check if normal vectors are reverse
            std::string first_line;
            std::getline(file, first_line);
            if(first_line.find("ccw") != std::string::npos){
            is_counter_clockwise = true;
            }
        }

        tinyobj::attrib_t attributes;
        std::vector<tinyobj::shape_t> shapes;
        std::string error;
        auto load_obj_result = tinyobj::LoadObj(&attributes, &shapes, nullptr, &error, &file);
        if(load_obj_result) {
            if(shapes.empty()) {
                std::cerr << "Object has no shape" << std::endl;
            } else if (0 != attributes.vertices.size() % 3) {
                std::cerr << "Vertices must be dividable by 3" << std::endl;
            } else if (0 != attributes.texcoords.size() % 2) {
                std::cerr << "Attributes must be dividable by 3" << std::endl;
            } else if (0 != shapes[0].mesh.indices.size() % 3) {
                std::cerr << "Indices must be dividable by 3" << std::endl;
            } else if (attributes.vertices.size() / 3 != attributes.texcoords.size() / 2) {
                std::cerr << "Vertices and texture coordinates must have same size" << std::endl;
            } else {
                std::vector<LoadObjResult::Position> positions {attributes.vertices.size() / 2};
                for(
                    uintmax_t vertex_index = 0; 
                    vertex_index < attributes.vertices.size() / 3; 
                    vertex_index ++
                ) {
                    positions[vertex_index].pos[0] = attributes.vertices[vertex_index * 3 + 0];
                    positions[vertex_index].pos[1] = attributes.vertices[vertex_index * 3 + 1];
                    positions[vertex_index].pos[2] = attributes.vertices[vertex_index * 3 + 2];
                }
                std::vector<LoadObjResult::TextureCoordinates> coords {attributes.texcoords.size() / 2};
                for(
                    uintmax_t tex_index = 0; 
                    tex_index < attributes.texcoords.size() / 2; 
                    tex_index ++
                ) {
                    coords[tex_index].uv[0] = attributes.texcoords[tex_index * 2 + 0];
                    coords[tex_index].uv[1] = attributes.texcoords[tex_index * 2 + 1];
                }
                ret.positions.resize(positions.size());
                ret.texture_coordinates.resize(coords.size());
                ret.indices.resize(shapes[0].mesh.indices.size());
                for(
                    uintmax_t indices_index = 0;
                    indices_index < shapes[0].mesh.indices.size();
                    indices_index += 1
                ) {
                    auto const vertex_index = shapes[0].mesh.indices[indices_index].vertex_index;
                    auto const uv_index = shapes[0].mesh.indices[indices_index].texcoord_index;
                    ret.indices[indices_index] = shapes[0].mesh.indices[indices_index].vertex_index;
                    ::memcpy(ret.positions[vertex_index].pos, positions[vertex_index].pos, sizeof(positions[vertex_index].pos));
                    ::memcpy(ret.texture_coordinates[vertex_index].uv, coords[uv_index].uv, sizeof(coords[uv_index].uv));
                    ret.texture_coordinates[vertex_index].uv[1] = 1.0f - ret.texture_coordinates[vertex_index].uv[1];
                }
                ret.success = true;
            } 
        } else if (!error.empty() && error.substr(0, 4) != "WARN") {
            std::cerr << "LoadObj returned error:" << error << " File:" << path << std::endl;
        } else {
            std::cerr << "LoadObj failed" << std::endl;
        }
    }
    return ret;
}
#endif

Asset::MeshAsset ImportGLTF(char const * path) {
    // TODO
    return Asset::MeshAsset {};
}

// Temporary function for freeing imported assets // Resource system will be used instead
bool FreeAsset(Asset::GenericAsset * asset) {
    bool ret = false;
    if(MFA_PTR_VALID(asset) && asset->valid()) {
        // TODO This is RCMGMT task
        Memory::Free(asset->asset());
        asset->set_asset({});
        ret = true;
    }
    return ret;
}

RawFile ReadRawFile(char const * path) {
    RawFile ret {};
    if(MFA_PTR_VALID(path) && FileSystem::Exists(path)) {
        auto * file = FileSystem::OpenFile(path, FileSystem::Usage::Read);
        MFA_DEFER {FileSystem::CloseFile(file);};
        auto const file_size = FileSystem::FileSize(file);
        // TODO Allocate using a memory pool system
        auto const memory_blob = Memory::Alloc(file_size);
        auto const read_bytes = FileSystem::Read(file, ret.data);
        // Means that reading is successful
        if(read_bytes == file_size) {
            ret.data = memory_blob;
        } else {
            Memory::Free(memory_blob);
        }
    }
    return ret;
}

bool FreeRawFile (RawFile * raw_file) {
    bool ret = false;
    if(MFA_PTR_VALID(raw_file) && raw_file->valid()) {
        Memory::Free(raw_file->data);
        raw_file->data = {};
        ret = true; 
    }
    return ret;
}

}