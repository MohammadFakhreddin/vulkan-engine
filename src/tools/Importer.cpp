#include "Importer.hpp"

#include "../engine/BedrockMemory.hpp"
#include "../engine/BedrockFileSystem.hpp"
#include "../tools/ImageUtils.hpp"
#include "libs/stb_image/stb_image_resize.h"
#include "libs/tiny_obj_loader/tiny_obj_loader.h"
#include "libs/tiny_gltf_loader/tiny_gltf_loader.h"

namespace MFA::Importer {

AS::Texture ImportUncompressedImage(
    char const * path,
    ImportTextureOptions const & options
) {
    AS::Texture texture {};
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
AS::Texture CreateErrorTexture() {
    auto const data = Memory::Alloc(4);
    auto * pixel = data.as<uint8_t>();
    pixel[0] = 1;
    pixel[1] = 1;
    pixel[2] = 1;
    pixel[3] = 1;

    return ImportInMemoryTexture(
        data,
        1,
        1,
        AS::TextureFormat::UNCOMPRESSED_UNORM_R8G8B8A8_LINEAR,
        4,
        1,
        1
    );
}

AS::Texture ImportInMemoryTexture(
    CBlob const originalImagePixels,
    I32 const width,
    I32 const height,
    AS::TextureFormat const format,
    U32 const components,
    U16 const depth,
    U16 const slices,
    ImportTextureOptions const & options
) {
    auto const useSrgb = options.prefer_srgb;
    AS::Texture::Dimensions const originalImageDimension {
        static_cast<U32>(width),
        static_cast<U32>(height),
        depth
    };
    U8 const mipCount = options.generate_mipmaps
        ? AS::Texture::ComputeMipCount(originalImageDimension)
        : 1;
    //if (static_cast<unsigned>(options.usage_flags) &
    //    static_cast<unsigned>(YUGA::TextureUsageFlags::Normal | 
    //        YUGA::TextureUsageFlags::Metalness |
    //        YUGA::TextureUsageFlags::Roughness)) {
    //    use_srgb = false;
    //}
    // TODO Maybe we should add a check to make sure asset has uncompressed type
    auto const bufferSize = AS::Texture::CalculateUncompressedTextureRequiredDataSize(
        format,
        slices,
        originalImageDimension,
        mipCount
    );
    AS::Texture texture {};
    texture.initForWrite(
        format,
        slices,
        mipCount,
        depth,
        options.sampler,
        Memory::Alloc(bufferSize)
    );
    MFA_DEFER {
        if(texture.isValid() == false) {// Pointer being valid means that process has failed
            Memory::Free(texture.revokeBuffer());
        }
    };

    // Generating mipmaps (TODO : Code needs debugging)
    for (U8 mipLevel = 0; mipLevel < mipCount - 1; mipLevel++) {
        auto const currentMipDims = AS::Texture::MipDimensions(
            mipLevel,
            mipCount,
            originalImageDimension
        );
        auto const currentMipSizeBytes = AS::Texture::MipSizeBytes(
            format,
            slices,
            currentMipDims
        );
        auto const mipMapPixels = Memory::Alloc(currentMipSizeBytes);
        MFA_DEFER {
            Memory::Free(mipMapPixels);
        };
        
        // TODO What should we do for 3d assets ?
        auto const resizeResult = useSrgb ? stbir_resize_uint8_srgb(
            originalImagePixels.ptr,
            width,
            height,
            0,
            mipMapPixels.ptr,
            currentMipDims.width,
            currentMipDims.height,
            0,
            components,
            components > 3 ? 3 : STBIR_ALPHA_CHANNEL_NONE,
            0
        ) : stbir_resize_uint8(
            originalImagePixels.ptr,
            width,
            height,
            0,
            mipMapPixels.ptr,
            currentMipDims.width,
            currentMipDims.height, 
            0,
            components
        );
        MFA_ASSERT(true == resizeResult);
        texture.addMipmap(
            currentMipDims,
            mipMapPixels
        );
    }
    texture.addMipmap(originalImageDimension, originalImagePixels);
    

    MFA_ASSERT(texture.isValid());

    return texture;
}

AS::Texture ImportDDSFile(char const * path) {
    // TODO
    MFA_NOT_IMPLEMENTED_YET("Mohammad Fakhreddin");
}

AS::Shader ImportShaderFromHLSL(char const * path) {
    MFA_NOT_IMPLEMENTED_YET("Mohammad Fakhreddin");
}

AS::Shader ImportShaderFromSPV(
    char const * path,
    AS::ShaderStage const stage,
    char const * entryPoint
) {
    MFA_ASSERT(path != nullptr);
    AS::Shader shader {};
    if (path != nullptr) {
        auto * file = FileSystem::OpenFile(path,  FileSystem::Usage::Read);
        MFA_DEFER{FileSystem::CloseFile(file);};
        if(FileSystem::IsUsable(file)) {
            auto const fileSize = FileSystem::FileSize(file);
            MFA_ASSERT(fileSize > 0);

            auto const buffer = Memory::Alloc(fileSize);

            shader.init(entryPoint, stage, buffer);

            auto const readBytes = FileSystem::Read(file, buffer);

            if(readBytes != buffer.len) {
                shader.revokeData();
                Memory::Free(buffer);
            }
        }
    }
    return shader;
}

AS::Shader ImportShaderFromSPV(
    CBlob const dataMemory,
    AS::ShaderStage const stage,
    char const * entryPoint
) {
    MFA_ASSERT(dataMemory.ptr != nullptr);
    MFA_ASSERT(dataMemory.len > 0);
    AS::Shader shader {};
    if (dataMemory.ptr != nullptr && dataMemory.len > 0) {
        auto const buffer = Memory::Alloc(dataMemory.len);
        ::memcpy(buffer.ptr, dataMemory.ptr, buffer.len);
        shader.init(entryPoint, stage, buffer);
    }
    return shader;
}

AS::Mesh ImportObj(char const * path) {
    AS::Mesh mesh_asset {};
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
                auto const header_size = AS::MeshHeader::ComputeHeaderSize(1);
                auto mesh_asset_blob = Memory::Alloc(AS::MeshHeader::ComputeAssetSize(
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
                mesh_asset = AS::MeshAsset(mesh_asset_blob);
                auto * mesh_header = mesh_asset_blob.as<AS::MeshHeader>();
                mesh_header->sub_mesh_count = 1;
                mesh_header->total_vertex_count = vertices_count;
                mesh_header->total_index_count = indices_count;
                mesh_header->sub_meshes[0].vertex_count = vertices_count;
                mesh_header->sub_meshes[0].vertices_offset = header_size;
                mesh_header->sub_meshes[0].index_count = indices_count;
                mesh_header->sub_meshes[0].indices_offset = header_size + vertices_count * sizeof(AS::MeshVertex);
                mesh_header->sub_meshes[0].has_normal_texture = true;
                mesh_header->sub_meshes[0].has_normal_buffer = true;
                // copying identity into subMesh
                Matrix4X4Float identityMatrix {};
                Matrix4X4Float::identity(identityMatrix);
                static_assert(sizeof(identityMatrix.cells) == sizeof(mesh_header->parent.matrix));
                ::memcpy(mesh_header->parent.matrix, identityMatrix.cells, sizeof(identityMatrix.cells));

                auto * mesh_vertices = mesh_asset.vertices_blob(0).as<AS::MeshVertex>();
                auto * mesh_indices = mesh_asset.indices_blob(0).as<AS::MeshIndex>();
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
                    // TODO fill other uvs as well (If we used obj in anything serious enough)
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
AS::Mesh ImportMeshGLTF(char const * path) {
    MFA_PTR_ASSERT(path);
    AS::ModelAsset model_asset {};
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
            auto const find_texture_by_name = [&texture_refs](char const * gltf_name)-> I16 {
                MFA_PTR_ASSERT(gltf_name);
                if(false == texture_refs.empty()) {
                    for (auto const & texture_ref : texture_refs) {
                        if(texture_ref.gltf_name == gltf_name) {
                            return texture_ref.index;
                        }
                    }
                }
                return -1;
                //MFA_CRASH("Image not found: %s", gltf_name);
            };
            auto const generate_uv_keyword = [](int32_t const uv_index) -> std::string{
                return "TEXCOORD_" + std::to_string(uv_index);
            };
            std::string directory_path = FileSystem::ExtractDirectoryFromPath(path);
            {// Extracting textures
                if(false == model.textures.empty()) {
                    for (auto const & texture : model.textures) {
                        AS::TextureSampler sampler {.is_valid = false};
                        {// Sampler
                            auto const & gltf_sampler = model.samplers[texture.sampler];
                            sampler.mag_filter = gltf_sampler.magFilter;
                            sampler.min_filter = gltf_sampler.minFilter;
                            sampler.wrap_s = gltf_sampler.wrapS;
                            sampler.wrap_t = gltf_sampler.wrapT;
                            //sampler.sample_mode = gltf_sampler. // TODO
                            sampler.is_valid = true;
                        }
                        AS::Texture texture_asset {};
                        auto const & image = model.images[texture.source];
                        {// Texture
                            std::string image_path = directory_path + "/" + image.uri;
                            texture_asset = ImportUncompressedImage(
                                image_path.c_str(),
                                // TODO generate_mipmaps = true;
                                ImportTextureOptions {.generate_mipmaps = false, .sampler = &sampler}
                            );
                        }
                        MFA_ASSERT(texture_asset.valid());
                        texture_refs.emplace_back(TextureRef {
                            .gltf_name = image.uri,
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
            auto const header_size = AS::MeshHeader::ComputeHeaderSize(static_cast<U32>(sub_mesh_count));
            auto const asset_size = AS::MeshHeader::ComputeAssetSize(
                header_size,
                total_vertices_count,
                total_indices_count
            );
            auto asset_blob = Memory::Alloc(asset_size);
            auto * header_object = asset_blob.as<AS::MeshHeader>();
            header_object->sub_mesh_count = sub_mesh_count;
            header_object->total_index_count = total_indices_count;
            header_object->total_vertex_count = total_vertices_count;
            MFA_DEFER {
                if(MFA_PTR_VALID(asset_blob.ptr)) {
                    Memory::Free(asset_blob);
                }
            };
            model_asset.mesh = AS::MeshAsset {asset_blob};
            // Step2: Fill asset buffer from model buffers
            U32 sub_mesh_index = 0;
            U64 vertices_offset = header_size;
            U64 indices_offset = vertices_offset + total_vertices_count * sizeof(AS::MeshVertex);
            U32 indices_starting_index = 0;
            U32 vertices_starting_index = 0;
            for(auto & mesh : model.meshes) {
                if(false == mesh.primitives.empty()) {
                    for(auto & primitive : mesh.primitives) {
                        I16 base_color_texture_index = -1;
                        int32_t base_color_uv_index = -1;
                        I16 metallic_roughness_texture_index = -1;
                        int32_t metallic_roughness_uv_index = -1;
                        I16 normal_texture_index = -1;
                        int32_t normal_uv_index = -1;
                        I16 emissive_texture_index = -1;
                        int32_t emissive_uv_index = -1;
                        float base_color_factor[4] {};
                        float metallic_factor = 0;
                        float roughness_factor = 0;
                        float emissive_factor [3] {};
                        if(primitive.material >= 0) {// Material
                            auto const & material = model.materials[primitive.material];
                            if (material.pbrMetallicRoughness.baseColorTexture.index >= 0) {// Base color texture
                                auto const & base_color_gltf_texture = model.textures[material.pbrMetallicRoughness.baseColorTexture.index];
                                auto const & image = model.images[base_color_gltf_texture.source];
                                base_color_texture_index = find_texture_by_name(image.uri.c_str());
                                if (base_color_texture_index >= 0) {
                                    base_color_uv_index = static_cast<uint16_t>(material.pbrMetallicRoughness.baseColorTexture.texCoord);
                                }
                            }
                            if (material.pbrMetallicRoughness.metallicRoughnessTexture.index >= 0){// Metallic-roughness texture
                                auto const & metallic_roughness_texture = model.textures[material.pbrMetallicRoughness.metallicRoughnessTexture.index];
                                auto const & image = model.images[metallic_roughness_texture.source];
                                metallic_roughness_texture_index = find_texture_by_name(image.uri.c_str());
                                if(metallic_roughness_texture_index >= 0) {
                                    metallic_roughness_uv_index = static_cast<uint16_t>(material.pbrMetallicRoughness.metallicRoughnessTexture.texCoord);
                                }
                            }
                            if (material.normalTexture.index >= 0) {// Normal texture
                                auto const & normal_texture = model.textures[material.normalTexture.index];
                                auto const & image = model.images[normal_texture.source];
                                normal_texture_index = find_texture_by_name(image.uri.c_str());
                                if(normal_texture_index >= 0) {
                                    normal_uv_index = static_cast<uint16_t>(material.normalTexture.texCoord);
                                }
                            }
                            if (material.emissiveTexture.index >= 0) {// Emissive texture
                                auto const & emissive_texture = model.textures[material.emissiveTexture.index];
                                auto const & image = model.images[emissive_texture.source];
                                emissive_texture_index = find_texture_by_name(image.uri.c_str());
                                if (emissive_texture_index >= 0) {
                                    emissive_uv_index = static_cast<uint16_t>(material.emissiveTexture.texCoord);
                                }
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
                        AS::MeshIndex * temp_indices = nullptr;
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
                            temp_indices_blob = Memory::Alloc(accessor.count * sizeof(AS::MeshIndex));
                            temp_indices = temp_indices_blob.as<AS::MeshIndex>();
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
                        if(primitive.attributes.find("POSITION") != primitive.attributes.end()){// Positions
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
                        if(base_color_uv_index >= 0) {// BaseColor
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
                        if(emissive_uv_index >= 0) {// Emission uvs
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
                        float normals_uv_min [2] {};
                        float normals_uv_max [2] {};
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
                        if(primitive.attributes.find("NORMAL") != primitive.attributes.end()) {
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
                        float const * tangent_values = nullptr;
                        float tangents_values_min [4] {};
                        float tangents_values_max [4] {};
                        if(primitive.attributes.find("TANGENT") != primitive.attributes.end()) {
                            MFA_REQUIRE(primitive.attributes["TANGENT"] < model.accessors.size());
                            auto const & accessor = model.accessors[primitive.attributes["TANGENT"]];
                            MFA_ASSERT(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);
                            tangents_values_min[0] = static_cast<float>(accessor.minValues[0]);
                            tangents_values_min[1] = static_cast<float>(accessor.minValues[1]);
                            tangents_values_min[2] = static_cast<float>(accessor.minValues[2]);
                            tangents_values_min[3] = static_cast<float>(accessor.minValues[3]);
                            tangents_values_max[0] = static_cast<float>(accessor.maxValues[0]);
                            tangents_values_max[1] = static_cast<float>(accessor.maxValues[1]);
                            tangents_values_max[2] = static_cast<float>(accessor.maxValues[2]);
                            tangents_values_max[3] = static_cast<float>(accessor.maxValues[3]);
                            auto const & buffer_view = model.bufferViews[accessor.bufferView];
                            MFA_REQUIRE(buffer_view.buffer < model.buffers.size());
                            auto const & buffer = model.buffers[buffer_view.buffer];
                            tangent_values = reinterpret_cast<const float *>(
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
                        {// Assigning constant values
                            current_sub_mesh.indices_offset = indices_offset;
                            current_sub_mesh.indices_starting_index = indices_starting_index;
                            current_sub_mesh.vertices_offset = vertices_offset;
                            current_sub_mesh.index_count = primitive_indices_count;
                            current_sub_mesh.vertex_count = primitive_vertex_count;
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

                            current_sub_mesh.has_base_color_texture = nullptr != base_color_uvs;
                            current_sub_mesh.has_normal_texture = nullptr != normals_uvs;
                            current_sub_mesh.has_normal_buffer = nullptr != normal_values;
                            current_sub_mesh.has_tangent_buffer = nullptr != tangent_values;
                            current_sub_mesh.has_emissive_texture = nullptr != emission_uvs;
                            current_sub_mesh.has_combined_metallic_roughness_texture = nullptr != metallic_roughness_uvs;
                            MFA_ASSERT(current_sub_mesh.has_tangent_buffer == current_sub_mesh.has_normal_texture);
                        }
                        vertices_offset += primitive_vertex_count * sizeof(AS::Mesh::Vertex);
                        indices_offset += primitive_indices_count * sizeof(AS::MeshIndex);
                        indices_starting_index += primitive_indices_count;
                        vertices_starting_index += primitive_vertex_count;
                        auto * vertices = model_asset.mesh.vertices(sub_mesh_index);
                        auto * indices = model_asset.mesh.indices(sub_mesh_index);
                        ::memcpy(indices, temp_indices_blob.ptr, temp_indices_blob.len);
                        ++ sub_mesh_index;
                        MFA_PTR_ASSERT(positions);
                        MFA_ASSERT(current_sub_mesh.has_base_color_texture == false || base_color_uvs != nullptr);
                        MFA_ASSERT(current_sub_mesh.has_normal_texture == false || normals_uvs != nullptr);
                        MFA_ASSERT(current_sub_mesh.has_combined_metallic_roughness_texture == false || metallic_roughness_uvs != nullptr);
                        MFA_ASSERT(current_sub_mesh.has_emissive_texture == false || emission_uvs != nullptr);
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
                                vertices[i].normal_value[0] = normal_values[i * 3 + 0];
                                MFA_ASSERT(vertices[i].normal_value[0] >= normals_values_min[0]);
                                MFA_ASSERT(vertices[i].normal_value[0] <= normals_values_max[0]);
                                vertices[i].normal_value[1] = normal_values[i * 3 + 1];
                                MFA_ASSERT(vertices[i].normal_value[1] >= normals_values_min[1]);
                                MFA_ASSERT(vertices[i].normal_value[1] <= normals_values_max[1]);
                                vertices[i].normal_value[2] = normal_values[i * 3 + 2];
                                MFA_ASSERT(vertices[i].normal_value[2] >= normals_values_min[2]);
                                MFA_ASSERT(vertices[i].normal_value[2] <= normals_values_max[2]);
                            }
                            if(current_sub_mesh.has_normal_texture) {// Normal uvs
                                vertices[i].normal_map_uv[0] = normals_uvs[i * 2 + 0];
                                MFA_ASSERT(vertices[i].normal_map_uv[0] >= normals_uv_min[0]);
                                MFA_ASSERT(vertices[i].normal_map_uv[0] <= normals_uv_max[0]);
                                vertices[i].normal_map_uv[1] = normals_uvs[i * 2 + 1];
                                MFA_ASSERT(vertices[i].normal_map_uv[1] >= normals_uv_min[1]);
                                MFA_ASSERT(vertices[i].normal_map_uv[1] <= normals_uv_max[1]);
                            }
                            if(current_sub_mesh.has_tangent_buffer) {// Tangent
                                vertices[i].tangent_value[0] = tangent_values[i * 4 + 0];
                                MFA_ASSERT(vertices[i].tangent_value[0] >= tangents_values_min[0]);
                                MFA_ASSERT(vertices[i].tangent_value[0] <= tangents_values_max[0]);
                                vertices[i].tangent_value[1] = tangent_values[i * 4 + 1];
                                MFA_ASSERT(vertices[i].tangent_value[1] >= tangents_values_min[1]);
                                MFA_ASSERT(vertices[i].tangent_value[1] <= tangents_values_max[1]);
                                vertices[i].tangent_value[2] = tangent_values[i * 4 + 2];
                                MFA_ASSERT(vertices[i].tangent_value[2] >= tangents_values_min[2]);
                                MFA_ASSERT(vertices[i].tangent_value[2] <= tangents_values_max[2]);
                                vertices[i].tangent_value[3] = tangent_values[i * 4 + 3];
                                MFA_ASSERT(vertices[i].tangent_value[3] >= tangents_values_min[3]);
                                MFA_ASSERT(vertices[i].tangent_value[3] <= tangents_values_max[3]);
                            }
                            if(current_sub_mesh.has_emissive_texture) {// Emissive
                                vertices[i].emission_uv[0] = emission_uvs[i * 2 + 0];
                                MFA_ASSERT(vertices[i].emission_uv[0] >= emission_uv_min[0]);
                                MFA_ASSERT(vertices[i].emission_uv[0] <= emission_uv_max[0]);
                                vertices[i].emission_uv[1] = emission_uvs[i * 2 + 1];
                                MFA_ASSERT(vertices[i].emission_uv[1] >= emission_uv_min[1]);
                                MFA_ASSERT(vertices[i].emission_uv[1] <= emission_uv_max[1]);
                            }
                            if (current_sub_mesh.has_base_color_texture) {// BaseColor
                                vertices[i].base_color_uv[0] = base_color_uvs[i * 2 + 0];
                                MFA_ASSERT(vertices[i].base_color_uv[0] >= base_color_uv_min[0]);
                                MFA_ASSERT(vertices[i].base_color_uv[0] <= base_color_uv_max[0]);
                                vertices[i].base_color_uv[1] = base_color_uvs[i * 2 + 1];
                                MFA_ASSERT(vertices[i].base_color_uv[1] >= base_color_uv_min[1]);
                                MFA_ASSERT(vertices[i].base_color_uv[1] <= base_color_uv_max[1]);
                            }
                            if (current_sub_mesh.has_combined_metallic_roughness_texture) {// MetallicRoughness
                                vertices[i].roughness_uv[0] = metallic_roughness_uvs[i * 2 + 0];
                                MFA_ASSERT(vertices[i].roughness_uv[0] >= metallic_roughness_uv_min[0]);
                                MFA_ASSERT(vertices[i].roughness_uv[0] <= metallic_roughness_uv_max[0]);
                                vertices[i].roughness_uv[1] = metallic_roughness_uvs[i * 2 + 1];
                                MFA_ASSERT(vertices[i].roughness_uv[1] >= metallic_roughness_uv_min[1]);
                                MFA_ASSERT(vertices[i].roughness_uv[1] <= metallic_roughness_uv_max[1]);

                                ::memcpy(vertices[i].metallic_uv, vertices[i].roughness_uv, sizeof(vertices[i].roughness_uv));
                                static_assert(sizeof(vertices[i].roughness_uv) == sizeof(vertices[i].metallic_uv));
                                    
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

bool FreeModel(AS::Model * model) {
    bool success = false;
    MFA_PTR_ASSERT(model);
    if(MFA_PTR_VALID(model)) {
        // Mesh
        auto const result = FreeMesh(&model->mesh);
        MFA_ASSERT(result); MFA_CONSUME_VAR(result);
        // Textures
        if(false == model->textures.empty()) {
            for (auto & texture : model->textures) {
                auto const result = FreeTexture(&texture);
                MFA_ASSERT(result); MFA_CONSUME_VAR(result);
            }
        }
        success = true;
    }
    return success;
}

// Temporary function for freeing imported assets // Resource system will be used instead
bool FreeTexture(AS::Texture * texture) {
    bool success = false;
    MFA_ASSERT(texture != nullptr);
    MFA_ASSERT(texture->isValid());
    if(texture != nullptr && texture->isValid()) {
        // TODO This is RCMGMT task
        Memory::Free(texture->revokeBuffer());
        success = true;
    }
    return success;
}

bool FreeShader(AS::Shader * shader) {
    bool success = false;
    MFA_ASSERT(shader != nullptr);
    MFA_ASSERT(shader->isValid());
    if (shader != nullptr && shader->isValid()) {
        Memory::Free(shader->revokeData());
        success = true;
    }
    return success;
}

bool FreeMesh(AS::Mesh * mesh) {
    bool success = false;
    MFA_ASSERT(mesh != nullptr);
    MFA_ASSERT(mesh->isValid());
    if (mesh != nullptr && mesh->isValid()) {
        mesh->revokeData();
        success = true;
    }
    return success;
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
