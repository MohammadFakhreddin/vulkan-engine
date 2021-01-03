#include "Importer.hpp"

#include "../engine/BedrockMemory.hpp"
#include "../engine/BedrockFileSystem.hpp"
#include "../tools/ImageUtils.hpp"
#include "libs/stb_image/stb_image_resize.h"

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
            auto current_mip_map_location = texture_descriptor_size;
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
            ::memcpy(resource_blob.ptr + current_mip_map_location, pixels.ptr, pixels.len);
        }

        texture = Asset::TextureAsset(resource_blob);
        if(false == texture.valid()) {
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
            shader_header->entry_point = entry_point;
            Blob const data_memory = Blob {asset_memory.ptr + Asset::ShaderHeader::Size(), file_size};
            auto const read_bytes = FileSystem::Read(file, data_memory);
            if(read_bytes == asset_memory.len) {
                return Asset::ShaderAsset {asset_memory};
            }
            Memory::Free(asset_memory);
        }
    }
    return Asset::ShaderAsset {{}};
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
