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
    CBlob data_memory,
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
                mesh_header->sub_meshes[0].indices_offset = header_size + vertices_count * sizeof(AssetSystem::MeshVertices::Vertex);
                auto * mesh_vertices = mesh_asset.vertices_blob(0).as<AssetSystem::MeshVertices>()->vertices;
                auto * mesh_indices = mesh_asset.indices_blob(0).as<AssetSystem::MeshIndices>()->indices;
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
                    ::memcpy(mesh_vertices[vertex_index].normal, normals[vertex_index].value, sizeof(normals[vertex_index].value));
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
    MFA_PTR_VALID(path);
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
                U8 result = 0;
                if(false == texture_refs.empty()) {
                    for (auto const & texture_ref : texture_refs) {
                        result = texture_ref.index;
                    }
                }
                return result;
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
            U64 indices_offset = vertices_offset + total_vertices_count * sizeof(AssetSystem::MeshVertices::Vertex);
            U32 indices_starting_index = 0;
            for(auto & mesh : model.meshes) {
                if(false == mesh.primitives.empty()) {
                    for(auto & primitive : mesh.primitives) {
                        U8 base_color_texture_index = 0;
                        {// Material
                            auto const & material = model.materials[primitive.material];
                            {// Base color texture
                                auto const & base_color_gltf_texture = model.textures[material.pbrMetallicRoughness.baseColorTexture.index];
                                auto const & image = model.images[base_color_gltf_texture.source];
                                base_color_texture_index = find_texture_by_name(image.name.c_str());
                            }
                            // TODO Support other texture types as well
                        }
                        Blob temp_indices_blob {};
                        U32 * temp_indices = nullptr;
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
                            temp_indices_blob = Memory::Alloc(accessor.count * sizeof(AssetSystem::Mesh::Data::Indices::IndexType));
                            temp_indices = temp_indices_blob.as<U32>();
                            primitive_indices_count = static_cast<U32>(accessor.count);
                            switch(accessor.componentType) {
                                case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT:
                                {
                                    auto const * gltf_indices = reinterpret_cast<U32 const *>(
                                        &buffer.data[buffer_view.byteOffset + accessor.byteOffset]
                                    );
                                    for (U32 i = 0; i < accessor.count; i++) {
                                        temp_indices[i] = gltf_indices[i];
                                    }
                                }
                                break;
                                case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT:
                                {
                                    auto const * gltf_indices = reinterpret_cast<const U16 *>(
                                        &buffer.data[buffer_view.byteOffset + accessor.byteOffset]
                                    );
                                    for (U32 i = 0; i < accessor.count; i++) {
                                        temp_indices[i] = gltf_indices[i];
                                    }
                                }
                                break;
                                case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE:
                                {
                                    auto const * gltf_indices = reinterpret_cast<const U16 *>(
                                        &buffer.data[buffer_view.byteOffset + accessor.byteOffset]
                                    );
                                    for (U32 i = 0; i < accessor.count; i++) {
                                        temp_indices[i] = gltf_indices[i];
                                    }
                                }
                                break;
                                default:
                                    MFA_NOT_IMPLEMENTED_YET("Mohammad Fakhreddin");
                            }
                        }
                        float const * positions = nullptr;
                        U32 primitive_vertex_count = 0;
                        {// Positions
                            MFA_REQUIRE(primitive.attributes["POSITION"] < model.accessors.size());
                            auto const & accessor = model.accessors[primitive.attributes["POSITION"]];
                            auto const & buffer_view = model.bufferViews[accessor.bufferView];
                            MFA_REQUIRE(buffer_view.buffer < model.buffers.size());
                            auto const & buffer = model.buffers[buffer_view.buffer];
                            positions = reinterpret_cast<const float *>(
                                &buffer.data[buffer_view.byteOffset + accessor.byteOffset]
                            );
                            primitive_vertex_count = static_cast<U32>(accessor.count);
                        }
                        float const * base_color_texture_coordinates = nullptr;
                        // TODO TEXCOORD_0 does not have to be hard coded, We need to support other textures as well
                        if(primitive.attributes["TEXCOORD_0"] >= 0) {
                            MFA_REQUIRE(primitive.attributes["TEXCOORD_0"] < model.accessors.size());
                            auto const & accessor = model.accessors[primitive.attributes["TEXCOORD_0"]];
                            auto const & buffer_view = model.bufferViews[accessor.bufferView];
                            MFA_REQUIRE(buffer_view.buffer < model.buffers.size());
                            auto const & buffer = model.buffers[buffer_view.buffer];
                            base_color_texture_coordinates = reinterpret_cast<const float *>(
                                &buffer.data[buffer_view.byteOffset + accessor.byteOffset]
                            );
                        }
                        float const * normals = nullptr;
                        if(primitive.attributes["NORMAL"] >= 0) {
                            MFA_REQUIRE(primitive.attributes["NORMAL"] < model.accessors.size());
                            auto const & accessor = model.accessors[primitive.attributes["NORMAL"]];
                            auto const & buffer_view = model.bufferViews[accessor.bufferView];
                            MFA_REQUIRE(buffer_view.buffer < model.buffers.size());
                            auto const & buffer = model.buffers[buffer_view.buffer];
                            normals = reinterpret_cast<const float *>(
                                &buffer.data[buffer_view.byteOffset + accessor.byteOffset]
                            );
                        }
                        // TODO GLTF color is float (Not sure), Out stored color is U8
                        //float const * colors = nullptr;
                        //if(primitive.attributes["COLOR"] >= 0) {
                        //    MFA_REQUIRE(primitive.attributes["COLOR"] < model.accessors.size());
                        //    auto const & accessor = model.accessors[primitive.attributes["COLOR"]];
                        //    auto const & buffer_view = model.bufferViews[accessor.bufferView];
                        //    MFA_REQUIRE(buffer_view.buffer < model.buffers.size());
                        //    auto const & buffer = model.buffers[buffer_view.buffer];
                        //    colors = reinterpret_cast<const float *>(
                        //        &buffer.data[buffer_view.byteOffset + accessor.byteOffset]
                        //    );
                        //}
                        /*
                        // Append data to model's vertex buffer
                        for (size_t v = 0; v < vertexCount; v++) {
                            Vertex vert{};
                            vert.pos = glm::vec4(glm::make_vec3(&positionBuffer[v * 3]), 1.0f);
                            vert.normal = glm::normalize(glm::vec3(normalsBuffer ? glm::make_vec3(&normalsBuffer[v * 3]) : glm::vec3(0.0f)));
                            vert.uv = texCoordsBuffer ? glm::make_vec2(&texCoordsBuffer[v * 2]) : glm::vec3(0.0f);
                            vert.color = glm::vec3(1.0f);
                            vertexBuffer.push_back(vert);
                        }
                        */
                        auto & current_sub_mesh = header_object->sub_meshes[sub_mesh_index];
                        current_sub_mesh.indices_offset = indices_offset;
                        current_sub_mesh.indices_starting_index = indices_starting_index;
                        current_sub_mesh.vertices_offset = vertices_offset;
                        current_sub_mesh.index_count = primitive_indices_count;
                        current_sub_mesh.vertex_count = primitive_vertex_count;
                        current_sub_mesh.base_color_texture_index = base_color_texture_index;
                        vertices_offset += primitive_vertex_count * sizeof(AssetSystem::MeshVertices::Vertex);
                        indices_offset += primitive_indices_count * sizeof(AssetSystem::MeshIndices::IndexType);
                        indices_starting_index += primitive_indices_count;
                        auto * vertices = model_asset.mesh.vertices(sub_mesh_index);
                        auto * indices = model_asset.mesh.indices(sub_mesh_index);
                        ::memcpy(indices, temp_indices_blob.ptr, temp_indices_blob.len);
                        ++ sub_mesh_index;
                        for (U32 i = 0; i < primitive_vertex_count; ++i) {

                            vertices[i].vertices->position[0] = positions[i * 3 + 0];
                            vertices[i].vertices->position[1] = positions[i * 3 + 1];
                            vertices[i].vertices->position[2] = positions[i * 3 + 2];

                            vertices[i].vertices->normal[0] = normals[i * 3 + 0];
                            vertices[i].vertices->normal[1] = normals[i * 3 + 1];
                            vertices[i].vertices->normal[2] = normals[i * 3 + 2];

                            vertices[i].vertices->base_color_uv[0] = base_color_texture_coordinates[i * 2 + 0];
                            vertices[i].vertices->base_color_uv[1] = base_color_texture_coordinates[i * 2 + 1];

                           /* vertices[i].vertices->color[0] = colors[i * 3 + 0];
                            vertices[i].vertices->color[1] = colors[i * 3 + 1];
                            vertices[i].vertices->color[2] = colors[i * 3 + 2];*/
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
