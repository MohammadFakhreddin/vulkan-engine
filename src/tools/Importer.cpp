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
                    ::memcpy(mesh_vertices[vertex_index].uv, coords[uv_index].value, sizeof(coords[uv_index].value));
                    mesh_vertices[vertex_index].uv[1] = 1.0f - mesh_vertices[vertex_index].uv[1];
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
/*
 *
const tinygltf::Model& model;
// Assuming you loaded the gltf model.
 ...
for each primitive in a mesh...
const tinygltf::Accessor& accessor = model.accessors[primitive.attributes["POSITION"]];
const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
 *
// cast to float type read only. Use accessor and bufview byte offsets to determine where position data 
// is located in the buffer.
const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];
// bufferView byteoffset + accessor byteoffset tells you where the actual position data is within the buffer. From there
// you should already know how the data needs to be interpreted.
const float* positions = reinterpret_cast<const float*>(&buffer.data[bufferView.byteOffset + accessor.byteOffset]);
// From here, you choose what you wish to do with this position data. In this case, we  will display it out.
for (size_t i = 0; i < accessor.count; ++i) {
          // Positions are Vec3 components, so for each vec3 stride, offset for x, y, and z.
           std::cout << "(" << positions[i * 3 + 0] << ", "// x
                            << positions[i * 3 + 1] << ", " // y
                            << positions[i * 3 + 2] << ")" // z
                            << "\n";
}
proceed to next primitive...
 */
ImportMeshResult ImportMeshGLTF(char const * path) {
    ImportMeshResult import_result {};
    using namespace tinygltf;
    TinyGLTF loader {};
    Model model;
    std::string error;
    std::string warning;
    auto const success = loader.LoadASCIIFromFile(&model, &error, &warning,  std::string(path));
    if(success) {
        //struct FileContent {
        //    RawFile file;
        //    std::string uri;
        //};
        //std::vector<FileContent> file_contents {};
        //auto search_for_uri = [&file_contents](std::string const & uri)->FileContent const * {
        //    FileContent const * result = nullptr;
        //    if(false == file_contents.empty()) {
        //        for(auto const & file_content : file_contents) {
        //            if(file_content.uri == uri) {
        //                result = &file_content;
        //                break;
        //            }
        //        }
        //    }
        //    if(false == MFA_PTR_VALID(result)) {
        //        // TODO I think it will crash, uri must be relative to path address
        //        auto const & raw_file = ReadRawFile(uri.c_str());
        //        MFA_REQUIRE(raw_file.valid());
        //        file_contents.emplace_back(FileContent {.file = raw_file, .uri = uri});
        //    }
        //    return result;
        //};
        //MFA_DEFER {
        //    if(false == file_contents.empty()) {
        //        for(auto & file_content : file_contents) {
        //            FreeRawFile(&file_content.file);
        //        }
        //    }
        //};
        if(model.meshes.empty()) {
            // Step1: Iterate over all meshes and gather required information for asset buffer
            U32 total_indices_count = 0;
            U32 total_vertices_count = 0;
            for(auto & mesh : model.meshes) {
                if(false == mesh.primitives.empty()) {
                    for(auto & primitive : mesh.primitives) {
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
                        /*MFA_REQUIRE(accessor.type == TINYGLTF_TYPE_SCALAR);
                        MFA_REQUIRE(accessor.bufferView < model.bufferViews.size());
                        auto const & buffer_view = model.bufferViews[accessor.bufferView];
                        MFA_REQUIRE(buffer_view.buffer < model.buffers.size());
                        auto const & buffer = model.buffers[buffer_view.buffer];
                        reinterpret_cast<const U32 *>(&buffer.data[buffer_view.byteOffset + accessor.byteOffset]);*/
                    }
                }
                //if(false == model.meshes[0].primitives[0].attributes.empty()) {
                    //for(auto & attribute : model.meshes[0].primitives[0].attributes) {
                        //attribute.second
                    //}
                //}
                //model.meshes[0].primitives[0].attributes.
            }
            auto const header_size = AssetSystem::MeshHeader::ComputeHeaderSize(static_cast<U32>(model.meshes.size()));
            auto const asset_size = AssetSystem::MeshHeader::ComputeAssetSize(
                header_size,
                total_vertices_count,
                total_indices_count
            );
            auto asset_blob = Memory::Alloc(asset_size);
            MFA_DEFER {
                if(MFA_PTR_VALID(asset_blob.ptr)) {
                    Memory::Free(asset_blob);
                }
            };
            import_result.mesh_asset = AssetSystem::MeshAsset {asset_blob};
            // Step2: Fill asset buffer from model buffers
            for(auto & mesh : model.meshes) {
                if(false == mesh.primitives.empty()) {
                    for(auto & primitive : mesh.primitives) {
                        {// Indices
                            MFA_REQUIRE(primitive.indices < model.accessors.size());
                            auto const & accessor = model.accessors[primitive.indices];
                            auto const & buffer_view = model.bufferViews[accessor.bufferView];
                            MFA_REQUIRE(buffer_view.buffer < model.buffers.size());
                            auto const & buffer = model.buffers[buffer_view.buffer];
                            auto const * indices = reinterpret_cast<const U32 *>(
                                &buffer.data[buffer_view.byteOffset + accessor.byteOffset]
                            );
                        }
                        {// Positions
                            MFA_REQUIRE(primitive.attributes["POSITION"] < model.accessors.size());
                            auto const & accessor = model.accessors[primitive.attributes["POSITION"]];
                            auto const & buffer_view = model.bufferViews[accessor.bufferView];
                            MFA_REQUIRE(buffer_view.buffer < model.buffers.size());
                            auto const & buffer = model.buffers[buffer_view.buffer];
                            auto const * positions = reinterpret_cast<const float *>(
                                &buffer.data[buffer_view.byteOffset + accessor.byteOffset]
                            );
                        }
                        if(primitive.attributes["TEXCOORD"] > 0) {//
                            MFA_REQUIRE(primitive.attributes["TEXCOORD"] < model.accessors.size());
                            auto const & accessor = model.accessors[primitive.attributes["POSITION"]];
                            auto const & buffer_view = model.bufferViews[accessor.bufferView];
                            MFA_REQUIRE(buffer_view.buffer < model.buffers.size());
                            auto const & buffer = model.buffers[buffer_view.buffer];
                            auto const * tex_coords = reinterpret_cast<const float *>(
                                &buffer.data[buffer_view.byteOffset + accessor.byteOffset]
                            );
                        }
                        // TODO Normal
                        // TODO Color
                        // TODO Start from here copy these values to mesh vertices
                    }
                }
            }
        }
    }
    return import_result;
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
