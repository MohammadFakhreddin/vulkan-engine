#include "Importer.hpp"

#include "../engine/BedrockMemory.hpp"
#include "../engine/BedrockFileSystem.hpp"
#include "../tools/ImageUtils.hpp"
#include "libs/stb_image/stb_image_resize.h"
#include "libs/tiny_obj_loader/tiny_obj_loader.h"
#include "libs/tiny_gltf_loader/tiny_gltf_loader.h"

namespace MFA::Importer {

AssetSystem::TextureAsset ImportUncompressedImage(
    char const * path,
    ImportTextureOptions const & options
) {
    AssetSystem::TextureAsset texture {};
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
        texture = ImportInMemoryTexture(
            pixels,
            image_width,
            image_height,
            format,
            components,
            depth,
            slices,
            options
        );
    }
    // TODO: Handle errors
    return texture;
}

AssetSystem::TextureAsset ImportInMemoryTexture(
    CBlob const pixels,
    I32 const width,
    I32 const height,
    AssetSystem::TextureFormat const format,
    U32 const components,
    U16 const depth,
    I32 const slices,
    ImportTextureOptions const & options
) {
    auto const use_srgb = options.prefer_srgb;
    AssetSystem::TextureHeader::Dimensions const original_image_dims {
        static_cast<U32>(width),
        static_cast<U32>(height),
        depth
    };
    U8 const mip_count = options.generate_mipmaps
        ? AssetSystem::TextureHeader::ComputeMipCount(original_image_dims)
        : 1;
    //if (static_cast<unsigned>(options.usage_flags) &
    //    static_cast<unsigned>(YUGA::TextureUsageFlags::Normal | 
    //        YUGA::TextureUsageFlags::Metalness |
    //        YUGA::TextureUsageFlags::Roughness)) {
    //    use_srgb = false;
    //}
    auto const texture_descriptor_size = AssetSystem::TextureHeader::Size(mip_count);
    // TODO Maybe we should add a check to make sure asset has uncompressed type
    auto const texture_data_size = AssetSystem::TextureHeader::CalculateUncompressedTextureRequiredDataSize(
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
    auto * texture_descriptor = resource_blob.as<AssetSystem::TextureHeader>();
    texture_descriptor->mip_count = mip_count;
    texture_descriptor->format = format;
    texture_descriptor->slices = slices;
    //texture_descriptor->usage_flags = options.usage_flags;        // TODO
    {
        size_t current_mip_map_location = 0;//texture_descriptor_size;
        bool resize_result {}; MFA_CONSUME_VAR(resize_result);
        for (U8 mip_level = 0; mip_level < mip_count - 1; mip_level++) {
            Byte * current_mipmap_ptr = resource_blob.ptr + current_mip_map_location;
            auto const current_mip_dims = AssetSystem::TextureHeader::MipDimensions(
                mip_level,
                mip_count,
                original_image_dims
            );
            auto const current_mip_size_bytes = AssetSystem::TextureHeader::MipSizeBytes(
                format,
                slices,
                current_mip_dims
            );
            // TODO What should we do for 3d assets ?
            resize_result = use_srgb ? stbir_resize_uint8_srgb(
                pixels.ptr,
                width,
                height,
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
                width,
                height,
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

    auto texture = AssetSystem::TextureAsset(resource_blob);
    if(MFA_PTR_VALID(options.sampler)) {
        MFA_ASSERT(options.sampler->is_valid);
        ::memcpy(&texture_descriptor->sampler, options.sampler, sizeof(&options.sampler));
    }
    MFA_ASSERT(texture.valid());
    if(true == texture.valid()) {
        resource_blob = {};
    }
    return texture;
}

AssetSystem::TextureAsset ImportDDSFile(char const * path) {
    // TODO
    MFA_NOT_IMPLEMENTED_YET("Mohammad Fakhreddin");
}

AssetSystem::ShaderAsset ImportShaderFromHLSL(char const * path) {
    MFA_NOT_IMPLEMENTED_YET("Mohammad Fakhreddin");
}

AssetSystem::ShaderAsset ImportShaderFromSPV(
    char const * path,
    AssetSystem::ShaderStage const stage,
    char const * entry_point
) {
    if(MFA_PTR_VALID(path)) {
        auto * file = FileSystem::OpenFile(path,  FileSystem::Usage::Read);
        MFA_DEFER{FileSystem::CloseFile(file);};
        if(FileSystem::IsUsable(file)) {
            auto const file_size = FileSystem::FileSize(file);
            MFA_ASSERT(file_size > 0);
            auto const asset_memory = Memory::Alloc(file_size + AssetSystem::ShaderHeader::Size());
            MFA_ASSERT(MFA_PTR_VALID(asset_memory.ptr) && asset_memory.len == file_size + AssetSystem::ShaderHeader::Size());
            auto * shader_header = asset_memory.as<AssetSystem::ShaderHeader>();
            shader_header->stage = stage;
            ::strncpy_s(shader_header->entry_point, entry_point, AssetSystem::ShaderHeader::EntryPointLength);
            Blob const data_memory = Blob {asset_memory.ptr + AssetSystem::ShaderHeader::Size(), file_size};
            auto const read_bytes = FileSystem::Read(file, data_memory);
            if(read_bytes == data_memory.len) {
                return AssetSystem::ShaderAsset {asset_memory};
            }
            Memory::Free(asset_memory);
        }
    }
    return AssetSystem::ShaderAsset {{}};
}

AssetSystem::ShaderAsset ImportShaderFromSPV(
    CBlob const data_memory,
    AssetSystem::ShaderStage const stage,
    char const * entry_point
) {
    MFA_BLOB_ASSERT(data_memory);
    auto const shader_header_size = AssetSystem::ShaderHeader::Size();
    auto const asset_memory = Memory::Alloc(data_memory.len + shader_header_size);
    ::memcpy(asset_memory.ptr + shader_header_size, data_memory.ptr, data_memory.len);
    auto * shader_header = asset_memory.as<AssetSystem::ShaderHeader>();
    shader_header->stage = stage;
    ::strncpy_s(shader_header->entry_point, entry_point, AssetSystem::ShaderHeader::EntryPointLength);
    return AssetSystem::ShaderAsset {asset_memory};
}

AssetSystem::MeshAsset ImportObj(char const * path) {
    AssetSystem::MeshAsset mesh_asset {};
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
            std::vector<tinyobj::material_t> materials;
            std::string error;
            auto const load_obj_result = tinyobj::LoadObj(
                &attributes,
                &shapes,
                &materials,
                &error, 
                path
            );
            // TODO Handle materials
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
                auto positions_count = attributes.vertices.size() / 3;
                auto coords_count = attributes.texcoords.size() / 2;
                auto normals_count = attributes.normals.size() / 3;
                if (positions_count != coords_count) {
                    MFA_CRASH("Vertices and texture coordinates must have same size");
                }
                // TODO I think normals have issues
                struct Position {
                    float value[3];
                };
                auto positions_blob = Memory::Alloc(sizeof(Position) * positions_count);
                MFA_DEFER {Memory::Free(positions_blob);};
                auto * positions = positions_blob.as<Position>();
                for(
                    uintmax_t vertex_index = 0; 
                    vertex_index < positions_count; 
                    vertex_index ++
                ) {
                    positions[vertex_index].value[0] = attributes.vertices[vertex_index * 3 + 0];
                    positions[vertex_index].value[1] = attributes.vertices[vertex_index * 3 + 1];
                    positions[vertex_index].value[2] = attributes.vertices[vertex_index * 3 + 2];
                }
                struct TextureCoordinates {
                    float value[2];
                };
                auto coords_blob = Memory::Alloc(sizeof(TextureCoordinates) * coords_count);
                MFA_DEFER {Memory::Free(coords_blob);};
                auto * coords = coords_blob.as<TextureCoordinates>();
                for(
                    uintmax_t tex_index = 0; 
                    tex_index < coords_count; 
                    tex_index ++
                ) {
                    coords[tex_index].value[0] = attributes.texcoords[tex_index * 2 + 0];
                    coords[tex_index].value[1] = attributes.texcoords[tex_index * 2 + 1];
                }
                struct Normals {
                    float value[3];
                };
                auto normals_blob = Memory::Alloc(sizeof(Normals) * normals_count);
                MFA_DEFER {Memory::Free(normals_blob);};
                auto * normals = normals_blob.as<Normals>();
                for(
                    uintmax_t normal_index = 0; 
                    normal_index < normals_count; 
                    normal_index ++
                ) {
                    normals[normal_index].value[0] = attributes.normals[normal_index * 3 + 0];
                    normals[normal_index].value[1] = attributes.normals[normal_index * 3 + 1];
                    normals[normal_index].value[2] = attributes.normals[normal_index * 3 + 2];
                }
                auto const vertices_count = static_cast<U32>(positions_count);
                auto const indices_count = static_cast<U32>(shapes[0].mesh.indices.size());
                auto const header_size = AssetSystem::MeshHeader::ComputeHeaderSize(1);
                auto mesh_asset_blob = Memory::Alloc(AssetSystem::MeshHeader::ComputeAssetSize(
                    header_size,
                    vertices_count,      // For Vertices // TODO Recheck this part again
                    indices_count
                ));
                MFA_DEFER {
                    if(MFA_PTR_VALID(mesh_asset_blob.ptr)) {
                        Memory::Free(mesh_asset_blob);
                    }
                };
                // TODO Multiple mesh support
                mesh_asset = AssetSystem::MeshAsset(mesh_asset_blob);
                auto * mesh_header = mesh_asset_blob.as<AssetSystem::MeshHeader>();
                mesh_header->sub_mesh_count = 1;
                mesh_header->total_vertex_count = vertices_count;
                mesh_header->total_index_count = indices_count;
                mesh_header->sub_meshes[0].vertex_count = vertices_count;
                mesh_header->sub_meshes[0].vertices_offset = header_size;
                mesh_header->sub_meshes[0].index_count = indices_count;
                mesh_header->sub_meshes[0].indices_offset = header_size + vertices_count * sizeof(AssetSystem::MeshVertex);
                mesh_header->sub_meshes[0].has_normal_texture = true;
                mesh_header->sub_meshes[0].has_normal_buffer = true;
                auto * mesh_vertices = mesh_asset.vertices_blob(0).as<AssetSystem::MeshVertex>();
                auto * mesh_indices = mesh_asset.indices_blob(0).as<AssetSystem::MeshIndex>();
                MFA_ASSERT(mesh_asset.indices_blob(0).ptr + mesh_asset.indices_blob(0).len == mesh_asset_blob.ptr + mesh_asset_blob.len);
                for(
                    uintmax_t indices_index = 0;
                    indices_index < shapes[0].mesh.indices.size();
                    indices_index ++
                ) {
                    auto const vertex_index = shapes[0].mesh.indices[indices_index].vertex_index;
                    auto const uv_index = shapes[0].mesh.indices[indices_index].texcoord_index;
                    mesh_indices[indices_index] = shapes[0].mesh.indices[indices_index].vertex_index;
                    ::memcpy(mesh_vertices[vertex_index].position, positions[vertex_index].value, sizeof(positions[vertex_index].value));
                    ::memcpy(mesh_vertices[vertex_index].base_color_uv, coords[uv_index].value, sizeof(coords[uv_index].value));
                    mesh_vertices[vertex_index].base_color_uv[1] = 1.0f - mesh_vertices[vertex_index].base_color_uv[1];
                    ::memcpy(mesh_vertices[vertex_index].normal_value, normals[vertex_index].value, sizeof(normals[vertex_index].value));
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
// TODO We need data-types called model and model-assets
// Based on sasha willems and a comment in github
AssetSystem::ModelAsset ImportMeshGLTF(char const * path) {
    MFA_PTR_ASSERT(path);
    AssetSystem::ModelAsset model_asset {};
    using namespace tinygltf;
    TinyGLTF loader {};
    Model model;
    std::string error;
    std::string warning;
    auto const success = loader.LoadASCIIFromFile(
        &model, 
        &error,
        &warning,  
        std::string(path)
    );
    if(success) {
        if(false == model.meshes.empty()) {
            struct TextureRef {
                std::string gltf_name;
                U8 index;
            };
            std::vector<TextureRef> texture_refs {};
            auto const find_texture_by_name = [&texture_refs](char const * gltf_name)-> U8 {
                MFA_PTR_ASSERT(gltf_name);
                if(false == texture_refs.empty()) {
                    for (auto const & texture_ref : texture_refs) {
                        if(texture_ref.gltf_name == gltf_name) {
                            return texture_ref.index;
                        }
                    }
                }
                MFA_CRASH("Image not found: %s", gltf_name);
            };
            auto const generate_uv_keyword = [](int32_t const uv_index) -> std::string{
                return "TEXCOORD_" + uv_index;
            };
            std::string directory_path = FileSystem::ExtractDirectoryFromPath(path);
            {// Extracting textures
                if(false == model.textures.empty()) {
                    for (auto const & texture : model.textures) {
                        AssetSystem::TextureSampler sampler {.is_valid = false};
                        {// Sampler
                            auto const & gltf_sampler = model.samplers[texture.sampler];
                            sampler.mag_filter = gltf_sampler.magFilter;
                            sampler.min_filter = gltf_sampler.minFilter;
                            sampler.wrap_s = gltf_sampler.wrapS;
                            sampler.wrap_t = gltf_sampler.wrapT;
                            //sampler.sample_mode = gltf_sampler. // TODO
                            sampler.is_valid = true;
                        }
                        AssetSystem::TextureAsset texture_asset {};
                        {// Texture
                            auto const & image = model.images[texture.source];
                            std::string image_path = directory_path + "/" + image.uri;
                            texture_asset = ImportUncompressedImage(
                                image_path.c_str(),
                                // TODO generate_mipmaps = true;
                                ImportTextureOptions {.generate_mipmaps = false, .sampler = &sampler}
                            );
                        }
                        MFA_ASSERT(texture_asset.valid());
                        texture_refs.emplace_back(TextureRef {
                            .gltf_name = texture.name,
                            .index = static_cast<U8>(model_asset.textures.size())
                        });
                        model_asset.textures.emplace_back(texture_asset);
                    }
                }
            }
            {// Reading samplers values from materials
                //auto const & sampler = model.samplers[base_color_gltf_texture.sampler];
                //model_asset.textures[base_color_texture_index]  
            }
            U32 sub_mesh_count = 0;
            // Step1: Iterate over all meshes and gather required information for asset buffer
            U32 total_indices_count = 0;
            U32 total_vertices_count = 0;
            for(auto & mesh : model.meshes) {
                if(false == mesh.primitives.empty()) {
                    for(auto & primitive : mesh.primitives) {
                        ++sub_mesh_count;
                        {// Indices
                            MFA_REQUIRE((primitive.indices < model.accessors.size()));
                            auto const & accessor = model.accessors[primitive.indices];
                            total_indices_count += static_cast<U32>(accessor.count);
                        }
                        {// Positions
                            MFA_REQUIRE((primitive.attributes["POSITION"] < model.accessors.size()));
                            auto const & accessor = model.accessors[primitive.attributes["POSITION"]];
                            total_vertices_count += static_cast<U32>(accessor.count);
                        }
                    }
                }
            }
            auto const header_size = AssetSystem::MeshHeader::ComputeHeaderSize(static_cast<U32>(sub_mesh_count));
            auto const asset_size = AssetSystem::MeshHeader::ComputeAssetSize(
                header_size,
                total_vertices_count,
                total_indices_count
            );
            auto asset_blob = Memory::Alloc(asset_size);
            auto * header_object = asset_blob.as<AssetSystem::MeshHeader>();
            header_object->sub_mesh_count = sub_mesh_count;
            header_object->total_index_count = total_indices_count;
            header_object->total_vertex_count = total_vertices_count;
            MFA_DEFER {
                if(MFA_PTR_VALID(asset_blob.ptr)) {
                    Memory::Free(asset_blob);
                }
            };
            model_asset.mesh = AssetSystem::MeshAsset {asset_blob};
            // Step2: Fill asset buffer from model buffers
            U32 sub_mesh_index = 0;
            U64 vertices_offset = header_size;
            U64 indices_offset = vertices_offset + total_vertices_count * sizeof(AssetSystem::MeshVertex);
            U32 indices_starting_index = 0;
            U32 vertices_starting_index = 0;
            for(auto & mesh : model.meshes) {
                if(false == mesh.primitives.empty()) {
                    for(auto & primitive : mesh.primitives) {
                        U8 base_color_texture_index = 0;
                        int32_t base_color_uv_index = 0;
                        U8 metallic_roughness_texture_index = 0;
                        int32_t metallic_roughness_uv_index = 0;
                        U8 normal_texture_index = 0;
                        int32_t normal_uv_index = 0; // TODO Start from here , It seems we can have both normal and normal map
                        U8 emissive_texture_index = 0;
                        int32_t emissive_uv_index = 0;
                        float base_color_factor[4] {};
                        float metallic_factor = 0;
                        float roughness_factor = 0;
                        float emissive_factor [3] {};
                        {// Material
                            auto const & material = model.materials[primitive.material];
                            {// Base color texture
                                auto const & base_color_gltf_texture = model.textures[material.pbrMetallicRoughness.baseColorTexture.index];
                                base_color_uv_index = static_cast<uint16_t>(material.pbrMetallicRoughness.baseColorTexture.texCoord);
                                auto const & image = model.images[base_color_gltf_texture.source];
                                base_color_texture_index = find_texture_by_name(image.name.c_str());
                            }
                            {// Metallic-roughness texture
                                auto const & metallic_roughness_texture = model.textures[material.pbrMetallicRoughness.metallicRoughnessTexture.index];
                                metallic_roughness_uv_index = static_cast<uint16_t>(material.pbrMetallicRoughness.metallicRoughnessTexture.texCoord);
                                auto const & image = model.images[metallic_roughness_texture.source];
                                metallic_roughness_texture_index = find_texture_by_name(image.name.c_str());
                            }
                            {// Normal texture
                                auto const & normal_texture = model.textures[material.normalTexture.index];
                                normal_uv_index = static_cast<uint16_t>(material.normalTexture.texCoord);
                                auto const & image = model.images[normal_texture.source];
                                normal_texture_index = find_texture_by_name(image.name.c_str());
                            }
                            {// Emissive texture
                                auto const & emissive_texture = model.textures[material.emissiveTexture.index];
                                emissive_uv_index = static_cast<uint16_t>(material.emissiveTexture.texCoord);
                                auto const & image = model.images[emissive_texture.source];
                                emissive_texture_index = find_texture_by_name(image.name.c_str());
                            }
                            {// BaseColorFactor
                                base_color_factor[0] = static_cast<float>(material.pbrMetallicRoughness.baseColorFactor[0]);
                                base_color_factor[1] = static_cast<float>(material.pbrMetallicRoughness.baseColorFactor[1]);
                                base_color_factor[2] = static_cast<float>(material.pbrMetallicRoughness.baseColorFactor[2]);
                                base_color_factor[3] = static_cast<float>(material.pbrMetallicRoughness.baseColorFactor[3]);
                            }
                            metallic_factor = static_cast<float>(material.pbrMetallicRoughness.metallicFactor);
                            roughness_factor = static_cast<float>(material.pbrMetallicRoughness.roughnessFactor);
                            {// EmissiveFactor
                                emissive_factor[0] = static_cast<float>(material.emissiveFactor[0]);
                                emissive_factor[1] = static_cast<float>(material.emissiveFactor[1]);
                                emissive_factor[2] = static_cast<float>(material.emissiveFactor[2]);
                            }
                        }
                        Blob temp_indices_blob {};
                        AssetSystem::MeshIndex * temp_indices = nullptr;
                        MFA_DEFER {
                            if(MFA_BLOB_VALID(temp_indices_blob)) {
                                Memory::Free(temp_indices_blob);
                            }
                        };
                        U32 primitive_indices_count = 0;
                        {// Indices
                            MFA_REQUIRE(primitive.indices < model.accessors.size());
                            auto const & accessor = model.accessors[primitive.indices];
                            auto const & buffer_view = model.bufferViews[accessor.bufferView];
                            MFA_REQUIRE(buffer_view.buffer < model.buffers.size());
                            auto const & buffer = model.buffers[buffer_view.buffer];
                            temp_indices_blob = Memory::Alloc(accessor.count * sizeof(AssetSystem::MeshIndex));
                            temp_indices = temp_indices_blob.as<AssetSystem::MeshIndex>();
                            primitive_indices_count = static_cast<U32>(accessor.count);
                            switch(accessor.componentType) {
                                case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT:
                                {
                                    auto const * gltf_indices = reinterpret_cast<U32 const *>(
                                        &buffer.data[buffer_view.byteOffset + accessor.byteOffset]
                                    );
                                    for (U32 i = 0; i < primitive_indices_count; i++) {
                                        temp_indices[i] = gltf_indices[i] + vertices_starting_index;
                                    }
                                }
                                break;
                                case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT:
                                {
                                    auto const * gltf_indices = reinterpret_cast<U16 const *>(
                                        &buffer.data[buffer_view.byteOffset + accessor.byteOffset]
                                    );
                                    for (U32 i = 0; i < primitive_indices_count; i++) {
                                        temp_indices[i] = gltf_indices[i] + vertices_starting_index;
                                    }
                                }
                                break;
                                case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE:
                                {
                                    auto const * gltf_indices = reinterpret_cast<U8 const *>(
                                        &buffer.data[buffer_view.byteOffset + accessor.byteOffset]
                                    );
                                    for (U32 i = 0; i < primitive_indices_count; i++) {
                                        temp_indices[i] = gltf_indices[i] + vertices_starting_index;
                                    }
                                }
                                break;
                                default:
                                    MFA_NOT_IMPLEMENTED_YET("Mohammad Fakhreddin");
                            }
                        }
                        float const * positions = nullptr;
                        U32 primitive_vertex_count = 0;
                        float positions_min_value [3];
                        float positions_max_value [3];
                        {// Positions
                            MFA_REQUIRE(primitive.attributes["POSITION"] < model.accessors.size());
                            auto const & accessor = model.accessors[primitive.attributes["POSITION"]];
                            MFA_ASSERT(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);
                            positions_min_value[0] = static_cast<float>(accessor.minValues[0]);
                            positions_min_value[1] = static_cast<float>(accessor.minValues[1]);
                            positions_min_value[2] = static_cast<float>(accessor.minValues[2]);
                            positions_max_value[0] = static_cast<float>(accessor.maxValues[0]);
                            positions_max_value[1] = static_cast<float>(accessor.maxValues[1]);
                            positions_max_value[2] = static_cast<float>(accessor.maxValues[2]);
                            auto const & buffer_view = model.bufferViews[accessor.bufferView];
                            MFA_REQUIRE(buffer_view.buffer < model.buffers.size());
                            auto const & buffer = model.buffers[buffer_view.buffer];
                            positions = reinterpret_cast<const float *>(
                                &buffer.data[buffer_view.byteOffset + accessor.byteOffset]
                            );
                            primitive_vertex_count = static_cast<U32>(accessor.count);
                        }
                        float const * base_color_uvs = nullptr;
                        float base_color_uv_min [2];
                        float base_color_uv_max [2];
                        {// BaseColor
                            auto texture_coordinate_key_name = generate_uv_keyword(base_color_uv_index);
                            MFA_ASSERT(primitive.attributes[texture_coordinate_key_name] >= 0);
                            MFA_REQUIRE(primitive.attributes[texture_coordinate_key_name] < model.accessors.size());
                            auto const & accessor = model.accessors[primitive.attributes[texture_coordinate_key_name]];
                            MFA_ASSERT(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);
                            base_color_uv_min[0] = static_cast<float>(accessor.minValues[0]);
                            base_color_uv_min[1] = static_cast<float>(accessor.minValues[1]);
                            base_color_uv_max[0] = static_cast<float>(accessor.maxValues[0]);
                            base_color_uv_max[1] = static_cast<float>(accessor.maxValues[1]);
                            auto const & buffer_view = model.bufferViews[accessor.bufferView];
                            MFA_REQUIRE(buffer_view.buffer < model.buffers.size());
                            auto const & buffer = model.buffers[buffer_view.buffer];
                            base_color_uvs = reinterpret_cast<const float *>(
                                &buffer.data[buffer_view.byteOffset + accessor.byteOffset]
                            );
                        }
                        float const * metallic_roughness_uvs = nullptr;
                        float metallic_roughness_uv_min [2];
                        float metallic_roughness_uv_max [2];
                        if(metallic_roughness_uv_index >= 0) {// MetallicRoughness uvs
                            std::string texture_coordinate_key_name = generate_uv_keyword(metallic_roughness_uv_index);
                            MFA_ASSERT(primitive.attributes[texture_coordinate_key_name] >= 0);
                            MFA_REQUIRE(primitive.attributes[texture_coordinate_key_name] < model.accessors.size());
                            auto const & accessor = model.accessors[primitive.attributes[texture_coordinate_key_name]];
                            MFA_ASSERT(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);
                            metallic_roughness_uv_min[0] = static_cast<float>(accessor.minValues[0]);
                            metallic_roughness_uv_min[1] = static_cast<float>(accessor.minValues[1]);
                            metallic_roughness_uv_max[0] = static_cast<float>(accessor.maxValues[0]);
                            metallic_roughness_uv_max[1] = static_cast<float>(accessor.maxValues[1]);
                            auto const & buffer_view = model.bufferViews[accessor.bufferView];
                            MFA_REQUIRE(buffer_view.buffer < model.buffers.size());
                            auto const & buffer = model.buffers[buffer_view.buffer];
                            metallic_roughness_uvs = reinterpret_cast<const float *>(
                                &buffer.data[buffer_view.byteOffset + accessor.byteOffset]
                            );
                        }
                        // TODO Occlusion texture
                        float const * emission_uvs = nullptr;
                        float emission_uv_min [2] {};
                        float emission_uv_max [2] {};
                        {// Emission uvs
                            std::string texture_coordinate_key_name = generate_uv_keyword(emissive_uv_index);
                            MFA_ASSERT(primitive.attributes[texture_coordinate_key_name] >= 0);
                            MFA_REQUIRE(primitive.attributes[texture_coordinate_key_name] < model.accessors.size());
                            auto const & accessor = model.accessors[primitive.attributes[texture_coordinate_key_name]];
                            MFA_ASSERT(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);
                            emission_uv_min[0] = static_cast<float>(accessor.minValues[0]);
                            emission_uv_min[1] = static_cast<float>(accessor.minValues[1]);
                            emission_uv_max[0] = static_cast<float>(accessor.maxValues[0]);
                            emission_uv_max[1] = static_cast<float>(accessor.maxValues[1]);
                            auto const & buffer_view = model.bufferViews[accessor.bufferView];
                            MFA_REQUIRE(buffer_view.buffer < model.buffers.size());
                            auto const & buffer = model.buffers[buffer_view.buffer];
                            emission_uvs = reinterpret_cast<const float *>(
                                &buffer.data[buffer_view.byteOffset + accessor.byteOffset]
                            );
                        }
                        float const * normals_uvs = nullptr;
                        float normals_uv_min [3] {};
                        float normals_uv_max [3] {};
                        if(normal_uv_index >= 0) {
                            std::string texture_coordinate_key_name = generate_uv_keyword(normal_uv_index);
                            MFA_ASSERT(primitive.attributes[texture_coordinate_key_name] >= 0);
                            MFA_REQUIRE(primitive.attributes[texture_coordinate_key_name] < model.accessors.size());
                            auto const & accessor = model.accessors[primitive.attributes[texture_coordinate_key_name]];
                            MFA_ASSERT(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);
                            normals_uv_min[0] = static_cast<float>(accessor.minValues[0]);
                            normals_uv_min[1] = static_cast<float>(accessor.minValues[1]);
                            normals_uv_max[0] = static_cast<float>(accessor.maxValues[0]);
                            normals_uv_max[1] = static_cast<float>(accessor.maxValues[1]);
                            auto const & buffer_view = model.bufferViews[accessor.bufferView];
                            MFA_REQUIRE(buffer_view.buffer < model.buffers.size());
                            auto const & buffer = model.buffers[buffer_view.buffer];
                            normals_uvs = reinterpret_cast<const float *>(
                                &buffer.data[buffer_view.byteOffset + accessor.byteOffset]
                            );
                        }
                        float const * normal_values = nullptr;
                        float normals_values_min [3] {};
                        float normals_values_max [3] {};
                        if(primitive.attributes["NORMAL"] >= 0) {
                            MFA_REQUIRE(primitive.attributes["NORMAL"] < model.accessors.size());
                            auto const & accessor = model.accessors[primitive.attributes["NORMAL"]];
                            MFA_ASSERT(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);
                            normals_values_min[0] = static_cast<float>(accessor.minValues[0]);
                            normals_values_min[1] = static_cast<float>(accessor.minValues[1]);
                            normals_values_min[2] = static_cast<float>(accessor.minValues[2]);
                            normals_values_max[0] = static_cast<float>(accessor.maxValues[0]);
                            normals_values_max[1] = static_cast<float>(accessor.maxValues[1]);
                            normals_values_max[2] = static_cast<float>(accessor.maxValues[2]);
                            auto const & buffer_view = model.bufferViews[accessor.bufferView];
                            MFA_REQUIRE(buffer_view.buffer < model.buffers.size());
                            auto const & buffer = model.buffers[buffer_view.buffer];
                            normal_values = reinterpret_cast<const float *>(
                                &buffer.data[buffer_view.byteOffset + accessor.byteOffset]
                            );
                        }
                        float const * colors = nullptr;
                        float colors_min_value [3] {0};
                        float colors_max_value [3] {1};
                        float colors_min_max_dif [3] {1};
                        if(primitive.attributes["COLOR"] >= 0) {
                            MFA_REQUIRE(primitive.attributes["COLOR"] < model.accessors.size());
                            auto const & accessor = model.accessors[primitive.attributes["COLOR"]];
                            MFA_ASSERT(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);
                            colors_min_value[0] = static_cast<float>(accessor.minValues[0]);
                            colors_min_value[1] = static_cast<float>(accessor.minValues[1]);
                            colors_min_value[2] = static_cast<float>(accessor.minValues[2]);
                            colors_max_value[0] = static_cast<float>(accessor.maxValues[0]);
                            colors_max_value[1] = static_cast<float>(accessor.maxValues[1]);
                            colors_max_value[2] = static_cast<float>(accessor.maxValues[2]);
                            colors_min_max_dif[0] = colors_max_value[0] - colors_min_value[0];
                            colors_min_max_dif[1] = colors_max_value[1] - colors_min_value[1];
                            colors_min_max_dif[2] = colors_max_value[2] - colors_min_value[2];
                            auto const & buffer_view = model.bufferViews[accessor.bufferView];
                            MFA_REQUIRE(buffer_view.buffer < model.buffers.size());
                            auto const & buffer = model.buffers[buffer_view.buffer];
                            colors = reinterpret_cast<const float *>(
                                &buffer.data[buffer_view.byteOffset + accessor.byteOffset]
                            );
                        }
                        auto & current_sub_mesh = header_object->sub_meshes[sub_mesh_index];
                        {
                            current_sub_mesh.indices_offset = indices_offset;
                            current_sub_mesh.indices_starting_index = indices_starting_index;
                            current_sub_mesh.vertices_offset = vertices_offset;
                            current_sub_mesh.index_count = primitive_indices_count;
                            current_sub_mesh.vertex_count = primitive_vertex_count;
                            current_sub_mesh.base_color_texture_index = base_color_texture_index;
                            current_sub_mesh.metallic_roughness_texture_index = metallic_roughness_texture_index;
                            current_sub_mesh.normal_texture_index = normal_texture_index;
                            current_sub_mesh.emissive_texture_index = emissive_texture_index;
                            current_sub_mesh.base_color_texture_index = base_color_texture_index;
                            static_assert(sizeof(current_sub_mesh.base_color_factor) == sizeof(base_color_factor));
                            ::memcpy(current_sub_mesh.base_color_factor, base_color_factor, sizeof(current_sub_mesh.base_color_factor));
                            current_sub_mesh.metallic_factor = metallic_factor;
                            current_sub_mesh.roughness_factor = roughness_factor;
                            static_assert(sizeof(current_sub_mesh.emissive_factor) == sizeof(emissive_factor));
                            ::memcpy(current_sub_mesh.emissive_factor, emissive_factor, sizeof(emissive_factor));
                            current_sub_mesh.has_normal_texture = nullptr != normals_uvs;
                            current_sub_mesh.has_normal_buffer = nullptr != normal_values;
                            current_sub_mesh.has_emissive_texture = nullptr != emission_uvs;
                            current_sub_mesh.has_metallic_roughness = nullptr != metallic_roughness_uvs;
                        }
                        vertices_offset += primitive_vertex_count * sizeof(AssetSystem::MeshVertex);
                        indices_offset += primitive_indices_count * sizeof(AssetSystem::MeshIndex);
                        indices_starting_index += primitive_indices_count;
                        vertices_starting_index += primitive_vertex_count;
                        auto * vertices = model_asset.mesh.vertices(sub_mesh_index);
                        auto * indices = model_asset.mesh.indices(sub_mesh_index);
                        ::memcpy(indices, temp_indices_blob.ptr, temp_indices_blob.len);
                        ++ sub_mesh_index;
                        MFA_PTR_ASSERT(positions);
                        MFA_PTR_ASSERT(base_color_uvs);
                        MFA_PTR_ASSERT(normals_uvs);
                        MFA_PTR_ASSERT(emission_uvs);
                        MFA_PTR_ASSERT(metallic_roughness_uvs);
                        for (U32 i = 0; i < primitive_vertex_count; ++i) {
                            {// Vertices
                                vertices[i].position[0] = positions[i * 3 + 0];
                                MFA_ASSERT(vertices[i].position[0] >= positions_min_value[0]);
                                MFA_ASSERT(vertices[i].position[0] <= positions_max_value[0]);
                                vertices[i].position[1] = positions[i * 3 + 1];
                                MFA_ASSERT(vertices[i].position[1] >= positions_min_value[1]);
                                MFA_ASSERT(vertices[i].position[1] <= positions_max_value[1]);
                                vertices[i].position[2] = positions[i * 3 + 2];
                                MFA_ASSERT(vertices[i].position[2] >= positions_min_value[2]);
                                MFA_ASSERT(vertices[i].position[2] <= positions_max_value[2]);
                            }
                            if(current_sub_mesh.has_normal_buffer) {// Normal values
                                vertices[i].normal_map_uv[0] = normals_uvs[i * 3 + 0];
                                MFA_ASSERT(vertices[i].normal_map_uv[0] >= normals_values_min[0]);
                                MFA_ASSERT(vertices[i].normal_map_uv[0] <= normals_values_max[0]);
                                vertices[i].normal_map_uv[1] = normals_uvs[i * 3 + 1];
                                MFA_ASSERT(vertices[i].normal_map_uv[1] >= normals_values_min[1]);
                                MFA_ASSERT(vertices[i].normal_map_uv[1] <= normals_values_max[1]);
                                vertices[i].normal_map_uv[2] = normals_uvs[i * 3 + 2];
                                MFA_ASSERT(vertices[i].normal_map_uv[2] >= normals_values_min[2]);
                                MFA_ASSERT(vertices[i].normal_map_uv[2] <= normals_values_max[2]);
                            }
                            if(current_sub_mesh.has_normal_texture) {// Normal uvs
                                vertices[i].normal_map_uv[0] = normals_uvs[i * 2 + 0];
                                MFA_ASSERT(vertices[i].normal_map_uv[0] >= normals_uv_min[0]);
                                MFA_ASSERT(vertices[i].normal_map_uv[0] <= normals_uv_max[0]);
                                vertices[i].normal_map_uv[1] = normals_uvs[i * 2 + 1];
                                MFA_ASSERT(vertices[i].normal_map_uv[1] >= normals_uv_min[1]);
                                MFA_ASSERT(vertices[i].normal_map_uv[1] <= normals_uv_max[1]);
                            }
                            if(current_sub_mesh.has_emissive_texture) {// Emissive
                                vertices[i].emission_uv[0] = emission_uvs[i * 2 + 0];
                                MFA_ASSERT(vertices[i].emission_uv[0] >= emission_uv_min[0]);
                                MFA_ASSERT(vertices[i].emission_uv[0] <= emission_uv_max[0]);
                                vertices[i].emission_uv[1] = emission_uvs[i * 2 + 1];
                                MFA_ASSERT(vertices[i].emission_uv[1] >= emission_uv_min[1]);
                                MFA_ASSERT(vertices[i].emission_uv[1] <= emission_uv_max[1]);
                            }
                            {// BaseColor
                                vertices[i].base_color_uv[0] = base_color_uvs[i * 2 + 0];
                                MFA_ASSERT(vertices[i].base_color_uv[0] >= base_color_uv_min[0]);
                                MFA_ASSERT(vertices[i].base_color_uv[0] <= base_color_uv_max[0]);
                                vertices[i].base_color_uv[1] = base_color_uvs[i * 2 + 1];
                                MFA_ASSERT(vertices[i].base_color_uv[1] >= base_color_uv_min[1]);
                                MFA_ASSERT(vertices[i].base_color_uv[1] <= base_color_uv_max[1]);
                            }
                            if (current_sub_mesh.has_metallic_roughness) {// MetallicRoughness
                                vertices[i].metallic_roughness_uv[0] = metallic_roughness_uvs[i * 2 + 0];
                                MFA_ASSERT(vertices[i].metallic_roughness_uv[0] >= metallic_roughness_uv_min[0]);
                                MFA_ASSERT(vertices[i].metallic_roughness_uv[0] <= metallic_roughness_uv_max[0]);
                                vertices[i].metallic_roughness_uv[1] = metallic_roughness_uvs[i * 2 + 1];
                                MFA_ASSERT(vertices[i].metallic_roughness_uv[1] >= metallic_roughness_uv_min[1]);
                                MFA_ASSERT(vertices[i].metallic_roughness_uv[1] <= metallic_roughness_uv_max[1]);
                            }
                            vertices[i].color[0] = static_cast<U8>((256/(colors_min_max_dif[0])) * colors[i * 3 + 0]);
                            vertices[i].color[1] = static_cast<U8>((256/(colors_min_max_dif[1])) * colors[i * 3 + 1]);
                            vertices[i].color[2] = static_cast<U8>((256/(colors_min_max_dif[2])) * colors[i * 3 + 2]);
                        }
                    }
                }
            }
            if(model_asset.mesh.valid()) {
                asset_blob = {};
            }
        }
    }
    return model_asset;
}

bool FreeModel(AssetSystem::ModelAsset * model) {
    bool ret = false;
    if(MFA_PTR_VALID(model)) {
        {// Mesh
            auto const result = FreeAsset(&model->mesh);
            MFA_ASSERT(result); MFA_CONSUME_VAR(result);
        }
        {// Textures
            if(false == model->textures.empty()) {
                for (auto & texture : model->textures) {
                    auto const result = FreeAsset(&texture);
                    MFA_ASSERT(result); MFA_CONSUME_VAR(result);
                }
            }
        }
        ret = true;
    }
    return ret;
}

// Temporary function for freeing imported assets // Resource system will be used instead
bool FreeAsset(AssetSystem::GenericAsset * asset) {
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
