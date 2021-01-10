#include "Importer.hpp"

#include "../engine/BedrockMemory.hpp"
#include "../engine/BedrockFileSystem.hpp"
#include "../tools/ImageUtils.hpp"
#include "libs/stb_image/stb_image_resize.h"
#include "libs/tiny_obj_loader/tiny_obj_loader.h"

namespace MFA::Importer {

Asset::TextureAsset ImportUncompressedImage(
    char const * path,
    ImportUncompressedImageOptions const & options
) {
    Asset::TextureAsset texture {};
    Utils::UncompressedTexture::Data image_data {};
    auto const use_srgb = options.prefer_srgb;
    auto const load_image_result = Utils::UncompressedTexture::Load(
        image_data, 
        path, 
        use_srgb
    );
    if(load_image_result == Utils::UncompressedTexture::LoadResult::Success) {
        MFA_ASSERT(image_data.valid());
        auto const image_width = image_data.width;
        auto const image_height = image_data.height;
        auto const pixels = image_data.pixels;
        auto const components = image_data.components;
        auto const format = image_data.format;
        auto const depth = 1; // TODO We need to support depth
        auto const slices = 1;
        Asset::TextureHeader::Dimensions const original_image_dims {
            static_cast<U32>(image_width),
            static_cast<U32>(image_height),
            depth
        };
        U8 const mip_count = options.generate_mipmaps
            ? Asset::TextureHeader::ComputeMipCount(original_image_dims)
            : 1;
        //if (static_cast<unsigned>(options.usage_flags) &
        //    static_cast<unsigned>(YUGA::TextureUsageFlags::Normal | 
        //        YUGA::TextureUsageFlags::Metalness |
        //        YUGA::TextureUsageFlags::Roughness)) {
        //    use_srgb = false;
        //}
        auto const texture_descriptor_size = Asset::TextureHeader::Size(mip_count);
        // TODO Maybe we should add a check to make sure asset has uncompressed type
        auto const texture_data_size = Asset::TextureHeader::CalculateUncompressedTextureRequiredDataSize(
            format,
            slices,
            original_image_dims,
            mip_count
        );
        auto const texture_asset_size = texture_descriptor_size + texture_data_size;
        auto resource_blob = Memory::Alloc(texture_asset_size);
        MFA_DEFER {
            if(MFA_PTR_VALID(resource_blob.ptr)) {// Pointer being valid means that process has failed
                Memory::Free(resource_blob);
            }
        };
        // TODO We need to a function for generating metadata that uses functionality implemented inside fileWriter
        auto * texture_descriptor = resource_blob.as<Asset::TextureHeader>();
        texture_descriptor->mip_count = mip_count;
        texture_descriptor->format = format;
        texture_descriptor->slices = slices;
        //texture_descriptor->usage_flags = options.usage_flags;        // TODO
        {
            size_t current_mip_map_location = 0;//texture_descriptor_size;
            bool resize_result {}; MFA_CONSUME_VAR(resize_result);
            for (U8 mip_level = 0; mip_level < mip_count - 1; mip_level++) {
                Byte * current_mipmap_ptr = resource_blob.ptr + current_mip_map_location;
                auto const current_mip_dims = Asset::TextureHeader::MipDimensions(
                    mip_level,
                    mip_count,
                    original_image_dims
                );
                auto const current_mip_size_bytes = Asset::TextureHeader::MipSizeBytes(
                    format,
                    slices,
                    current_mip_dims
                );
                // TODO What should we do for 3d assets ?
                resize_result = use_srgb ? stbir_resize_uint8_srgb(
                    pixels.ptr,
                    image_width,
                    image_height,
                    0,
                    current_mipmap_ptr,
                    current_mip_dims.width,
                    current_mip_dims.height,
                    0,
                    components,
                    components > 3 ? 3 : STBIR_ALPHA_CHANNEL_NONE,
                    0
                ) : stbir_resize_uint8(
                    pixels.ptr,
                    image_width,
                    image_height,
                    0,
                    current_mipmap_ptr,
                    current_mip_dims.width,
                    current_mip_dims.height, 0,
                    components
                );
                MFA_ASSERT(true == resize_result);
                current_mip_map_location += current_mip_size_bytes;
                texture_descriptor->mipmap_infos[mip_level] = {
                    static_cast<U32>(current_mip_map_location),
                    static_cast<U32>(current_mip_size_bytes),
                    current_mip_dims
                };
            }
            texture_descriptor->mipmap_infos[mip_count - 1] = {
                static_cast<U32>(current_mip_map_location),
                static_cast<U32>(pixels.len),
                original_image_dims
            };
            ::memcpy(resource_blob.ptr + current_mip_map_location + texture_descriptor_size, pixels.ptr, pixels.len);
        }

        texture = Asset::TextureAsset(resource_blob);
        MFA_ASSERT(texture.valid());
        if(true == texture.valid()) {
            resource_blob = {};
        }
    }
    // TODO: Handle errors
    return texture;
}

Asset::TextureAsset ImportDDSFile(char const * path) {
    // TODO
    MFA_NOT_IMPLEMENTED_YET("Mohammad Fakhreddin");
}

Asset::ShaderAsset ImportShaderFromHLSL(char const * path) {
    MFA_NOT_IMPLEMENTED_YET("Mohammad Fakhreddin");
}

Asset::ShaderAsset ImportShaderFromSPV(
    char const * path,
    Asset::ShaderStage const stage,
    char const * entry_point
) {
    if(MFA_PTR_VALID(path)) {
        auto * file = FileSystem::OpenFile(path,  FileSystem::Usage::Read);
        MFA_DEFER{FileSystem::CloseFile(file);};
        if(FileSystem::IsUsable(file)) {
            auto const file_size = FileSystem::FileSize(file);
            MFA_ASSERT(file_size > 0);
            auto const asset_memory = Memory::Alloc(file_size + Asset::ShaderHeader::Size());
            MFA_ASSERT(MFA_PTR_VALID(asset_memory.ptr) && asset_memory.len == file_size + Asset::ShaderHeader::Size());
            auto * shader_header = asset_memory.as<Asset::ShaderHeader>();
            shader_header->stage = stage;
            ::strncpy_s(shader_header->entry_point, entry_point, Asset::ShaderHeader::EntryPointLength);
            Blob const data_memory = Blob {asset_memory.ptr + Asset::ShaderHeader::Size(), file_size};
            auto const read_bytes = FileSystem::Read(file, data_memory);
            if(read_bytes == data_memory.len) {
                return Asset::ShaderAsset {asset_memory};
            }
            Memory::Free(asset_memory);
        }
    }
    return Asset::ShaderAsset {{}};
}

Asset::MeshAsset ImportObj(char const * path) {
    Asset::MeshAsset mesh_asset {};
    if(FileSystem::Exists(path)){
        auto * file = FileSystem::OpenFile(path, FileSystem::Usage::Read);
        MFA_DEFER {FileSystem::CloseFile(file);};
        if(FileSystem::IsUsable(file)) {
            bool is_counter_clockwise = false;
            {//Check if normal vectors are reverse
                auto first_line_blob = Memory::Alloc(200);
                MFA_DEFER {Memory::Free(first_line_blob);};
                FileSystem::Read(file, first_line_blob);
                std::string const first_line = std::string(first_line_blob.as<char>());
                if(first_line.find("ccw") != std::string::npos){
                    is_counter_clockwise = true;
                }
            }

            tinyobj::attrib_t attributes;
            std::vector<tinyobj::shape_t> shapes;
            std::string error;
            auto const load_obj_result = tinyobj::LoadObj(
                &attributes,
                &shapes,
                nullptr,
                &error, 
                path
            );
            if(load_obj_result) {
                if(shapes.empty()) {
                    MFA_CRASH( "Object has no shape");
                }
                if (0 != attributes.vertices.size() % 3) {
                    MFA_CRASH("Vertices must be dividable by 3");
                }
                if (0 != attributes.texcoords.size() % 2) {
                    MFA_CRASH("Attributes must be dividable by 3");
                }
                if (0 != shapes[0].mesh.indices.size() % 3) {
                    MFA_CRASH("Indices must be dividable by 3");
                }
                if (attributes.vertices.size() / 3 != attributes.texcoords.size() / 2) {
                    MFA_CRASH("Vertices and texture coordinates must have same size");
                }
                struct Position {
                    float value[3];
                };
                std::vector<Position> positions {attributes.vertices.size() / 2};
                for(
                    uintmax_t vertex_index = 0; 
                    vertex_index < attributes.vertices.size() / 3; 
                    vertex_index ++
                ) {
                    positions[vertex_index].value[0] = attributes.vertices[vertex_index * 3 + 0];
                    positions[vertex_index].value[1] = attributes.vertices[vertex_index * 3 + 1];
                    positions[vertex_index].value[2] = attributes.vertices[vertex_index * 3 + 2];
                }
                struct TextureCoordinates {
                    float value[2];
                };
                std::vector<TextureCoordinates> coords {attributes.texcoords.size() / 2};
                for(
                    uintmax_t tex_index = 0; 
                    tex_index < attributes.texcoords.size() / 2; 
                    tex_index ++
                ) {
                    coords[tex_index].value[0] = attributes.texcoords[tex_index * 2 + 0];
                    coords[tex_index].value[1] = attributes.texcoords[tex_index * 2 + 1];
                }
                auto const vertices_count = static_cast<U32>(shapes[0].mesh.indices.size());
                auto const indices_count = static_cast<U32>(shapes[0].mesh.indices.size());
                auto mesh_asset_blob = Memory::Alloc(Asset::MeshHeader::ComputeAssetSize(
                    vertices_count,      // For Vertices // TODO Recheck this part again
                    indices_count
                ));
                MFA_DEFER {
                    if(MFA_PTR_VALID(mesh_asset_blob.ptr)) {
                        Memory::Free(mesh_asset_blob);
                    }
                };
                mesh_asset = Asset::MeshAsset(mesh_asset_blob);
                auto * mesh_header = mesh_asset_blob.as<Asset::MeshHeader>();
                mesh_header->vertex_count = vertices_count;
                mesh_header->vertices_offset = sizeof(Asset::MeshHeader);
                mesh_header->index_count = indices_count;
                mesh_header->indices_offset = sizeof(Asset::MeshHeader) + vertices_count * sizeof(Asset::MeshVertices::Vertex);
                auto * mesh_vertices = mesh_asset.vertices_blob().as<Asset::MeshVertices>()->vertices;
                auto * mesh_indices = mesh_asset.indices_blob().as<Asset::MeshIndices>()->indices;
                for(
                    uintmax_t indices_index = 0;
                    indices_index < shapes[0].mesh.indices.size();
                    indices_index ++
                ) {
                    auto const vertex_index = shapes[0].mesh.indices[indices_index].vertex_index;
                    auto const uv_index = shapes[0].mesh.indices[indices_index].texcoord_index;
                    mesh_indices[indices_index] = shapes[0].mesh.indices[indices_index].vertex_index;
                    ::memcpy(mesh_vertices[vertex_index].position, positions[vertex_index].value, sizeof(positions[vertex_index].value));
                    ::memcpy(mesh_vertices[vertex_index].uv, coords[uv_index].value, sizeof(coords[uv_index].value));
                    mesh_vertices[vertex_index].uv[1] = 1.0f - mesh_vertices[vertex_index].uv[1];
                }
                MFA_ASSERT(mesh_asset.valid());
                if(true == mesh_asset.valid()) {
                    mesh_asset_blob = {};
                }
            } else if (!error.empty() && error.substr(0, 4) != "WARN") {
                MFA_CRASH("LoadObj returned error: %s, File: %s", error.c_str(), path);
            } else {
                MFA_CRASH("LoadObj failed");
            }
        }
    }
    return mesh_asset;
}

Asset::MeshAsset ImportGLTF(char const * path) {
    MFA_NOT_IMPLEMENTED_YET("Mohammad Fakhreddin");
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
    if(MFA_PTR_VALID(path)) {
        auto * file = FileSystem::OpenFile(path, FileSystem::Usage::Read);
        MFA_DEFER {FileSystem::CloseFile(file);};
        if(FileSystem::IsUsable(file)) {
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
