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
        MFA_ASSERT(resizeResult > 0);
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
    AS::Mesh mesh {};
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
                auto normalsCount = attributes.normals.size() / 3;
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
                    uintmax_t vertexIndex = 0; 
                    vertexIndex < positions_count; 
                    vertexIndex ++
                ) {
                    positions[vertexIndex].value[0] = attributes.vertices[vertexIndex * 3 + 0];
                    positions[vertexIndex].value[1] = attributes.vertices[vertexIndex * 3 + 1];
                    positions[vertexIndex].value[2] = attributes.vertices[vertexIndex * 3 + 2];
                }
                struct TextureCoordinates {
                    float value[2];
                };
                auto coords_blob = Memory::Alloc(sizeof(TextureCoordinates) * coords_count);
                MFA_DEFER {Memory::Free(coords_blob);};
                auto * coords = coords_blob.as<TextureCoordinates>();
                for(
                    uintmax_t texIndex = 0; 
                    texIndex < coords_count; 
                    texIndex ++
                ) {
                    coords[texIndex].value[0] = attributes.texcoords[texIndex * 2 + 0];
                    coords[texIndex].value[1] = attributes.texcoords[texIndex * 2 + 1];
                }
                struct Normals {
                    float value[3];
                };
                auto normalsBlob = Memory::Alloc(sizeof(Normals) * normalsCount);
                MFA_DEFER {Memory::Free(normalsBlob);};
                auto * normals = normalsBlob.as<Normals>();
                for(
                    uintmax_t normalIndex = 0; 
                    normalIndex < normalsCount; 
                    normalIndex ++
                ) {
                    normals[normalIndex].value[0] = attributes.normals[normalIndex * 3 + 0];
                    normals[normalIndex].value[1] = attributes.normals[normalIndex * 3 + 1];
                    normals[normalIndex].value[2] = attributes.normals[normalIndex * 3 + 2];
                }
                auto const vertexCount = static_cast<U32>(positions_count);
                auto const indexCount = static_cast<U32>(shapes[0].mesh.indices.size());
                mesh.initForWrite(
                    vertexCount, 
                    indexCount, 
                    Memory::Alloc(sizeof(AS::Mesh::Vertex) * vertexCount),
                    Memory::Alloc(sizeof(AS::Mesh::Index) * indexCount)
                );

                auto const subMeshIndex = mesh.insertSubMesh();
                
                MFA_DEFER {
                    if (mesh.isValid() == false) {
                        Blob vertexBuffer {};
                        Blob indexBuffer {};
                        mesh.revokeBuffers(vertexBuffer, indexBuffer);
                        Memory::Free(vertexBuffer);
                        Memory::Free(indexBuffer);
                    }
                };
                
                std::vector<AS::Mesh::Vertex> vertices {vertexCount};
                std::vector<AS::Mesh::Index> indices {indexCount};
                for(
                    uintmax_t indicesIndex = 0;
                    indicesIndex < shapes[0].mesh.indices.size();
                    indicesIndex ++
                ) {
                    auto const vertexIndex = shapes[0].mesh.indices[indicesIndex].vertex_index;
                    auto const uvIndex = shapes[0].mesh.indices[indicesIndex].texcoord_index;
                    indices[indicesIndex] = shapes[0].mesh.indices[indicesIndex].vertex_index;
                    ::memcpy(vertices[vertexIndex].position, positions[vertexIndex].value, sizeof(positions[vertexIndex].value));
                    ::memcpy(vertices[vertexIndex].baseColorUV, coords[uvIndex].value, sizeof(coords[uvIndex].value));
                    // TODO fill other uvs as well (If we used obj in anything serious enough)
                    vertices[vertexIndex].baseColorUV[1] = 1.0f - vertices[vertexIndex].baseColorUV[1];
                    ::memcpy(vertices[vertexIndex].normalValue, normals[vertexIndex].value, sizeof(normals[vertexIndex].value));
                }

                mesh.insertPrimitive(
                    subMeshIndex,
                    AS::Mesh::Primitive {
                        .vertexCount = vertexCount,
                        .indicesCount = indexCount,
                        .baseColorTextureIndex = 0,
                        .hasNormalBuffer = true
                    },
                    static_cast<U32>(vertices.size()),
                    vertices.data(),
                    static_cast<U32>(indices.size()),
                    indices.data()
                );

                mesh.insertNode(AS::MeshNode {
                    .subMeshIndex = 0,
                    .children {},
                    .transformMatrix = Matrix4X4Float::Identity(),
                });

                MFA_ASSERT(mesh.isValid());

            } else if (!error.empty() && error.substr(0, 4) != "WARN") {
                MFA_CRASH("LoadObj returned error: %s, File: %s", error.c_str(), path);
            } else {
                MFA_CRASH("LoadObj failed");
            }
        }
    }
    return mesh;
}

// Based on sasha willems solution and a comment in github
AS::Model ImportGLTF(char const * path) {
    // TODO Create separate functions for each part
    MFA_ASSERT(path != nullptr);
    AS::Model resultModel {};
    if (path != nullptr) {
        namespace TG = tinygltf;
        TG::TinyGLTF loader {};
        std::string error;
        std::string warning;
        TG::Model gltfModel {};
        auto const success = loader.LoadASCIIFromFile(
            &gltfModel, 
            &error,
            &warning,  
            std::string(path)
        );
        if(success) {
            if(false == gltfModel.meshes.empty()) {
                struct TextureRef {
                    std::string gltf_name;
                    U8 index;
                };
                std::vector<TextureRef> textureRefs {};
                auto const find_texture_by_name = [&textureRefs](char const * gltfName)-> I16 {
                    MFA_ASSERT(gltfName != nullptr);
                    if(false == textureRefs.empty()) {
                        for (auto const & textureRef : textureRefs) {
                            if(textureRef.gltf_name == gltfName) {
                                return textureRef.index;
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
                // Extracting textures
                if(false == gltfModel.textures.empty()) {
                    for (auto const & texture : gltfModel.textures) {
                        AS::TextureSampler sampler {.isValid = false};
                        {// Sampler
                            auto const & gltf_sampler = gltfModel.samplers[texture.sampler];
                            sampler.magFilter = gltf_sampler.magFilter;
                            sampler.minFilter = gltf_sampler.minFilter;
                            sampler.wrapS = gltf_sampler.wrapS;
                            sampler.wrapT = gltf_sampler.wrapT;
                            //sampler.sample_mode = gltf_sampler. // TODO
                            sampler.isValid = true;
                        }
                        AS::Texture assetSystemTexture {};
                        auto const & image = gltfModel.images[texture.source];
                        {// Texture
                            std::string image_path = directory_path + "/" + image.uri;
                            assetSystemTexture = ImportUncompressedImage(
                                image_path.c_str(),
                                // TODO generate_mipmaps = true;
                                ImportTextureOptions {.generate_mipmaps = false, .sampler = &sampler}
                            );
                        }
                        MFA_ASSERT(assetSystemTexture.isValid());
                        textureRefs.emplace_back(TextureRef {
                            .gltf_name = image.uri,
                            .index = static_cast<U8>(resultModel.textures.size())
                        });
                        resultModel.textures.emplace_back(assetSystemTexture);
                    }
                }
                {// Reading samplers values from materials
                    //auto const & sampler = model.samplers[base_color_gltf_texture.sampler];
                    //model_asset.textures[base_color_texture_index]  
                }
                // Step1: Iterate over all meshes and gather required information for asset buffer
                U32 sub_mesh_count = 0;
                U32 totalIndicesCount = 0;
                U32 totalVerticesCount = 0;
                U32 indicesStartingIndex = 0;
                for(auto & mesh : gltfModel.meshes) {
                    if(false == mesh.primitives.empty()) {
                        for(auto & primitive : mesh.primitives) {
                            ++sub_mesh_count;
                            {// Indices
                                MFA_REQUIRE((primitive.indices < gltfModel.accessors.size()));
                                auto const & accessor = gltfModel.accessors[primitive.indices];
                                totalIndicesCount += static_cast<U32>(accessor.count);
                            }
                            {// Positions
                                MFA_REQUIRE((primitive.attributes["POSITION"] < gltfModel.accessors.size()));
                                auto const & accessor = gltfModel.accessors[primitive.attributes["POSITION"]];
                                totalVerticesCount += static_cast<U32>(accessor.count);
                            }
                        }
                    }
                }
                resultModel.mesh.initForWrite(
                    totalVerticesCount, 
                    totalIndicesCount, 
                    Memory::Alloc(sizeof(AS::MeshVertex) * totalVerticesCount), 
                    Memory::Alloc(sizeof(AS::MeshIndex) * totalIndicesCount)
                );
                MFA_DEFER {
                    if(resultModel.mesh.isValid() == false) {
                        Blob vertexBuffer {};
                        Blob indicesBuffer {};
                        resultModel.mesh.revokeBuffers(vertexBuffer, indicesBuffer);
                        Memory::Free(vertexBuffer);
                        Memory::Free(indicesBuffer);
                    }
                };
                // Step2: Fill subMeshes
                U32 subMeshIndex = 0;
                std::vector<AS::Mesh::Vertex> subMeshVertices {};
                std::vector<AS::MeshIndex> subMeshIndices {};
                                   
                for(auto & mesh : gltfModel.meshes) {
                    if(false == mesh.primitives.empty()) {
                        auto const meshIndex = resultModel.mesh.insertSubMesh();
                        for(auto & primitive : mesh.primitives) {
                            subMeshIndices.erase(subMeshIndices.begin(), subMeshIndices.end());
                            subMeshVertices.erase(subMeshVertices.begin(), subMeshVertices.end());
                            I16 baseColorTextureIndex = -1;
                            int32_t baseColorUvIndex = -1;
                            I16 metallicRoughnessTextureIndex = -1;
                            int32_t metallicRoughnessUvIndex = -1;
                            I16 normalTextureIndex = -1;
                            int32_t normalUvIndex = -1;
                            I16 emissiveTextureIndex = -1;
                            int32_t emissiveUvIndex = -1;
                            float baseColorFactor[4] {};
                            float metallicFactor = 0;
                            float roughnessFactor = 0;
                            float emissiveFactor [3] {};
                            if(primitive.material >= 0) {// Material
                                auto const & material = gltfModel.materials[primitive.material];
                                if (material.pbrMetallicRoughness.baseColorTexture.index >= 0) {// Base color texture
                                    auto const & base_color_gltf_texture = gltfModel.textures[material.pbrMetallicRoughness.baseColorTexture.index];
                                    auto const & image = gltfModel.images[base_color_gltf_texture.source];
                                    baseColorTextureIndex = find_texture_by_name(image.uri.c_str());
                                    if (baseColorTextureIndex >= 0) {
                                        baseColorUvIndex = static_cast<uint16_t>(material.pbrMetallicRoughness.baseColorTexture.texCoord);
                                    }
                                }
                                if (material.pbrMetallicRoughness.metallicRoughnessTexture.index >= 0){// Metallic-roughness texture
                                    auto const & metallic_roughness_texture = gltfModel.textures[material.pbrMetallicRoughness.metallicRoughnessTexture.index];
                                    auto const & image = gltfModel.images[metallic_roughness_texture.source];
                                    metallicRoughnessTextureIndex = find_texture_by_name(image.uri.c_str());
                                    if(metallicRoughnessTextureIndex >= 0) {
                                        metallicRoughnessUvIndex = static_cast<uint16_t>(material.pbrMetallicRoughness.metallicRoughnessTexture.texCoord);
                                    }
                                }
                                if (material.normalTexture.index >= 0) {// Normal texture
                                    auto const & normal_texture = gltfModel.textures[material.normalTexture.index];
                                    auto const & image = gltfModel.images[normal_texture.source];
                                    normalTextureIndex = find_texture_by_name(image.uri.c_str());
                                    if(normalTextureIndex >= 0) {
                                        normalUvIndex = static_cast<uint16_t>(material.normalTexture.texCoord);
                                    }
                                }
                                if (material.emissiveTexture.index >= 0) {// Emissive texture
                                    auto const & emissive_texture = gltfModel.textures[material.emissiveTexture.index];
                                    auto const & image = gltfModel.images[emissive_texture.source];
                                    emissiveTextureIndex = find_texture_by_name(image.uri.c_str());
                                    if (emissiveTextureIndex >= 0) {
                                        emissiveUvIndex = static_cast<uint16_t>(material.emissiveTexture.texCoord);
                                    }
                                }
                                {// BaseColorFactor
                                    baseColorFactor[0] = static_cast<float>(material.pbrMetallicRoughness.baseColorFactor[0]);
                                    baseColorFactor[1] = static_cast<float>(material.pbrMetallicRoughness.baseColorFactor[1]);
                                    baseColorFactor[2] = static_cast<float>(material.pbrMetallicRoughness.baseColorFactor[2]);
                                    baseColorFactor[3] = static_cast<float>(material.pbrMetallicRoughness.baseColorFactor[3]);
                                }
                                metallicFactor = static_cast<float>(material.pbrMetallicRoughness.metallicFactor);
                                roughnessFactor = static_cast<float>(material.pbrMetallicRoughness.roughnessFactor);
                                {// EmissiveFactor
                                    emissiveFactor[0] = static_cast<float>(material.emissiveFactor[0]);
                                    emissiveFactor[1] = static_cast<float>(material.emissiveFactor[1]);
                                    emissiveFactor[2] = static_cast<float>(material.emissiveFactor[2]);
                                }
                            }
                            U32 primitiveIndicesCount = 0;
                            {// Indices
                                MFA_REQUIRE(primitive.indices < gltfModel.accessors.size());
                                auto const & accessor = gltfModel.accessors[primitive.indices];
                                auto const & bufferView = gltfModel.bufferViews[accessor.bufferView];
                                MFA_REQUIRE(bufferView.buffer < gltfModel.buffers.size());
                                auto const & buffer = gltfModel.buffers[bufferView.buffer];
                                primitiveIndicesCount = static_cast<U32>(accessor.count);
                                
                                switch(accessor.componentType) {
                                    case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT:
                                    {
                                        auto const * gltfIndices = reinterpret_cast<U32 const *>(
                                            &buffer.data[bufferView.byteOffset + accessor.byteOffset]
                                        );
                                        for (U32 i = 0; i < primitiveIndicesCount; i++) {
                                            subMeshIndices.emplace_back(gltfIndices[i] + indicesStartingIndex);
                                        }
                                    }
                                    break;
                                    case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT:
                                    {
                                        auto const * gltfIndices = reinterpret_cast<U16 const *>(
                                            &buffer.data[bufferView.byteOffset + accessor.byteOffset]
                                        );
                                        for (U32 i = 0; i < primitiveIndicesCount; i++) {
                                            subMeshIndices.emplace_back(gltfIndices[i] + indicesStartingIndex);
                                        }
                                    }
                                    break;
                                    case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE:
                                    {
                                        auto const * gltfIndices = reinterpret_cast<U8 const *>(
                                            &buffer.data[bufferView.byteOffset + accessor.byteOffset]
                                        );
                                        for (U32 i = 0; i < primitiveIndicesCount; i++) {
                                            subMeshIndices.emplace_back(gltfIndices[i] + indicesStartingIndex);
                                        }
                                    }
                                    break;
                                    default:
                                        MFA_NOT_IMPLEMENTED_YET("Mohammad Fakhreddin");
                                }
                            }
                            float const * positions = nullptr;
                            U32 primitiveVertexCount = 0;
                            float positionsMinValue [3] {};
                            float positionsMaxValue [3] {};
                            if(primitive.attributes.find("POSITION") != primitive.attributes.end()){// Positions
                                MFA_REQUIRE(primitive.attributes["POSITION"] < gltfModel.accessors.size());
                                auto const & accessor = gltfModel.accessors[primitive.attributes["POSITION"]];
                                MFA_ASSERT(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);
                                positionsMinValue[0] = static_cast<float>(accessor.minValues[0]);
                                positionsMinValue[1] = static_cast<float>(accessor.minValues[1]);
                                positionsMinValue[2] = static_cast<float>(accessor.minValues[2]);
                                positionsMaxValue[0] = static_cast<float>(accessor.maxValues[0]);
                                positionsMaxValue[1] = static_cast<float>(accessor.maxValues[1]);
                                positionsMaxValue[2] = static_cast<float>(accessor.maxValues[2]);
                                auto const & bufferView = gltfModel.bufferViews[accessor.bufferView];
                                MFA_REQUIRE(bufferView.buffer < gltfModel.buffers.size());
                                auto const & buffer = gltfModel.buffers[bufferView.buffer];
                                positions = reinterpret_cast<const float *>(
                                    &buffer.data[bufferView.byteOffset + accessor.byteOffset]
                                );
                                primitiveVertexCount = static_cast<U32>(accessor.count);
                            }
                            float const * baseColorUvs = nullptr;
                            float baseColorUvMin [2];
                            float baseColorUvMax [2];
                            if(baseColorUvIndex >= 0) {// BaseColor
                                auto texture_coordinate_key_name = generate_uv_keyword(baseColorUvIndex);
                                MFA_ASSERT(primitive.attributes[texture_coordinate_key_name] >= 0);
                                MFA_REQUIRE(primitive.attributes[texture_coordinate_key_name] < gltfModel.accessors.size());
                                auto const & accessor = gltfModel.accessors[primitive.attributes[texture_coordinate_key_name]];
                                MFA_ASSERT(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);
                                baseColorUvMin[0] = static_cast<float>(accessor.minValues[0]);
                                baseColorUvMin[1] = static_cast<float>(accessor.minValues[1]);
                                baseColorUvMax[0] = static_cast<float>(accessor.maxValues[0]);
                                baseColorUvMax[1] = static_cast<float>(accessor.maxValues[1]);
                                auto const & bufferView = gltfModel.bufferViews[accessor.bufferView];
                                MFA_REQUIRE(bufferView.buffer < gltfModel.buffers.size());
                                auto const & buffer = gltfModel.buffers[bufferView.buffer];
                                baseColorUvs = reinterpret_cast<const float *>(
                                    &buffer.data[bufferView.byteOffset + accessor.byteOffset]
                                );
                            }
                            float const * metallicRoughnessUvs = nullptr;
                            float metallicRoughnessUVMin [2];
                            float metallicRoughnessUVMax [2];
                            if(metallicRoughnessUvIndex >= 0) {// MetallicRoughness uvs
                                std::string texture_coordinate_key_name = generate_uv_keyword(metallicRoughnessUvIndex);
                                MFA_ASSERT(primitive.attributes[texture_coordinate_key_name] >= 0);
                                MFA_REQUIRE(primitive.attributes[texture_coordinate_key_name] < gltfModel.accessors.size());
                                auto const & accessor = gltfModel.accessors[primitive.attributes[texture_coordinate_key_name]];
                                MFA_ASSERT(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);
                                metallicRoughnessUVMin[0] = static_cast<float>(accessor.minValues[0]);
                                metallicRoughnessUVMin[1] = static_cast<float>(accessor.minValues[1]);
                                metallicRoughnessUVMax[0] = static_cast<float>(accessor.maxValues[0]);
                                metallicRoughnessUVMax[1] = static_cast<float>(accessor.maxValues[1]);
                                auto const & bufferView = gltfModel.bufferViews[accessor.bufferView];
                                MFA_REQUIRE(bufferView.buffer < gltfModel.buffers.size());
                                auto const & buffer = gltfModel.buffers[bufferView.buffer];
                                metallicRoughnessUvs = reinterpret_cast<const float *>(
                                    &buffer.data[bufferView.byteOffset + accessor.byteOffset]
                                );
                            }
                            // TODO Occlusion texture
                            float const * emissionUVs = nullptr;
                            float emission_uv_min [2] {};
                            float emission_uv_max [2] {};
                            if(emissiveUvIndex >= 0) {// Emission uvs
                                std::string texture_coordinate_key_name = generate_uv_keyword(emissiveUvIndex);
                                MFA_ASSERT(primitive.attributes[texture_coordinate_key_name] >= 0);
                                MFA_REQUIRE(primitive.attributes[texture_coordinate_key_name] < gltfModel.accessors.size());
                                auto const & accessor = gltfModel.accessors[primitive.attributes[texture_coordinate_key_name]];
                                MFA_ASSERT(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);
                                emission_uv_min[0] = static_cast<float>(accessor.minValues[0]);
                                emission_uv_min[1] = static_cast<float>(accessor.minValues[1]);
                                emission_uv_max[0] = static_cast<float>(accessor.maxValues[0]);
                                emission_uv_max[1] = static_cast<float>(accessor.maxValues[1]);
                                auto const & bufferView = gltfModel.bufferViews[accessor.bufferView];
                                MFA_REQUIRE(bufferView.buffer < gltfModel.buffers.size());
                                auto const & buffer = gltfModel.buffers[bufferView.buffer];
                                emissionUVs = reinterpret_cast<const float *>(
                                    &buffer.data[bufferView.byteOffset + accessor.byteOffset]
                                );
                            }
                            float const * normalsUVs = nullptr;
                            float normals_uv_min [2] {};
                            float normals_uv_max [2] {};
                            if(normalUvIndex >= 0) {
                                std::string texture_coordinate_key_name = generate_uv_keyword(normalUvIndex);
                                MFA_ASSERT(primitive.attributes[texture_coordinate_key_name] >= 0);
                                MFA_REQUIRE(primitive.attributes[texture_coordinate_key_name] < gltfModel.accessors.size());
                                auto const & accessor = gltfModel.accessors[primitive.attributes[texture_coordinate_key_name]];
                                MFA_ASSERT(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);
                                normals_uv_min[0] = static_cast<float>(accessor.minValues[0]);
                                normals_uv_min[1] = static_cast<float>(accessor.minValues[1]);
                                normals_uv_max[0] = static_cast<float>(accessor.maxValues[0]);
                                normals_uv_max[1] = static_cast<float>(accessor.maxValues[1]);
                                auto const & bufferView = gltfModel.bufferViews[accessor.bufferView];
                                MFA_REQUIRE(bufferView.buffer < gltfModel.buffers.size());
                                auto const & buffer = gltfModel.buffers[bufferView.buffer];
                                normalsUVs = reinterpret_cast<const float *>(
                                    &buffer.data[bufferView.byteOffset + accessor.byteOffset]
                                );
                            }
                            float const * normalValues = nullptr;
                            float normalsValuesMin [3] {};
                            float normalsValuesMax [3] {};
                            if(primitive.attributes.find("NORMAL") != primitive.attributes.end()) {
                                MFA_REQUIRE(primitive.attributes["NORMAL"] < gltfModel.accessors.size());
                                auto const & accessor = gltfModel.accessors[primitive.attributes["NORMAL"]];
                                MFA_ASSERT(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);
                                normalsValuesMin[0] = static_cast<float>(accessor.minValues[0]);
                                normalsValuesMin[1] = static_cast<float>(accessor.minValues[1]);
                                normalsValuesMin[2] = static_cast<float>(accessor.minValues[2]);
                                normalsValuesMax[0] = static_cast<float>(accessor.maxValues[0]);
                                normalsValuesMax[1] = static_cast<float>(accessor.maxValues[1]);
                                normalsValuesMax[2] = static_cast<float>(accessor.maxValues[2]);
                                auto const & bufferView = gltfModel.bufferViews[accessor.bufferView];
                                MFA_REQUIRE(bufferView.buffer < gltfModel.buffers.size());
                                auto const & buffer = gltfModel.buffers[bufferView.buffer];
                                normalValues = reinterpret_cast<const float *>(
                                    &buffer.data[bufferView.byteOffset + accessor.byteOffset]
                                );
                            }
                            float const * tangentValues = nullptr;
                            float tangentsValuesMin [4] {};
                            float tangentsValuesMax [4] {};
                            if(primitive.attributes.find("TANGENT") != primitive.attributes.end()) {
                                MFA_REQUIRE(primitive.attributes["TANGENT"] < gltfModel.accessors.size());
                                auto const & accessor = gltfModel.accessors[primitive.attributes["TANGENT"]];
                                MFA_ASSERT(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);
                                tangentsValuesMin[0] = static_cast<float>(accessor.minValues[0]);
                                tangentsValuesMin[1] = static_cast<float>(accessor.minValues[1]);
                                tangentsValuesMin[2] = static_cast<float>(accessor.minValues[2]);
                                tangentsValuesMin[3] = static_cast<float>(accessor.minValues[3]);
                                tangentsValuesMax[0] = static_cast<float>(accessor.maxValues[0]);
                                tangentsValuesMax[1] = static_cast<float>(accessor.maxValues[1]);
                                tangentsValuesMax[2] = static_cast<float>(accessor.maxValues[2]);
                                tangentsValuesMax[3] = static_cast<float>(accessor.maxValues[3]);
                                auto const & bufferView = gltfModel.bufferViews[accessor.bufferView];
                                MFA_REQUIRE(bufferView.buffer < gltfModel.buffers.size());
                                auto const & buffer = gltfModel.buffers[bufferView.buffer];
                                tangentValues = reinterpret_cast<const float *>(
                                    &buffer.data[bufferView.byteOffset + accessor.byteOffset]
                                );
                            }
                            float const * colors = nullptr;
                            float colorsMinValue [3] {0};
                            float colorsMaxValue [3] {1};
                            float colorsMinMaxDiff [3] {1};
                            if(primitive.attributes["COLOR"] >= 0) {
                                MFA_REQUIRE(primitive.attributes["COLOR"] < gltfModel.accessors.size());
                                auto const & accessor = gltfModel.accessors[primitive.attributes["COLOR"]];
                                MFA_ASSERT(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);
                                colorsMinValue[0] = static_cast<float>(accessor.minValues[0]);
                                colorsMinValue[1] = static_cast<float>(accessor.minValues[1]);
                                colorsMinValue[2] = static_cast<float>(accessor.minValues[2]);
                                colorsMaxValue[0] = static_cast<float>(accessor.maxValues[0]);
                                colorsMaxValue[1] = static_cast<float>(accessor.maxValues[1]);
                                colorsMaxValue[2] = static_cast<float>(accessor.maxValues[2]);
                                colorsMinMaxDiff[0] = colorsMaxValue[0] - colorsMinValue[0];
                                colorsMinMaxDiff[1] = colorsMaxValue[1] - colorsMinValue[1];
                                colorsMinMaxDiff[2] = colorsMaxValue[2] - colorsMinValue[2];
                                auto const & bufferView = gltfModel.bufferViews[accessor.bufferView];
                                MFA_REQUIRE(bufferView.buffer < gltfModel.buffers.size());
                                auto const & buffer = gltfModel.buffers[bufferView.buffer];
                                colors = reinterpret_cast<const float *>(
                                    &buffer.data[bufferView.byteOffset + accessor.byteOffset]
                                );
                            }
                            
                            indicesStartingIndex += primitiveIndicesCount;
                            bool hasPosition = positions != nullptr;
                            MFA_ASSERT(hasPosition == true);
                            bool hasBaseColorTexture = baseColorUvs != nullptr;
                            MFA_ASSERT(baseColorUvs != nullptr == baseColorTextureIndex >= 0);
                            MFA_ASSERT(hasBaseColorTexture == true);
                            bool hasNormalValue = normalValues != nullptr;
                            MFA_ASSERT(hasNormalValue == true);
                            bool hasNormalTexture = normalsUVs != nullptr;
                            MFA_ASSERT(normalsUVs != nullptr == normalTextureIndex >= 0);
                            bool hasCombinedMetallicRoughness = metallicRoughnessUvs != nullptr;
                            MFA_ASSERT(metallicRoughnessUvs != nullptr == metallicRoughnessTextureIndex >= 0);
                            bool hasEmissiveTexture = emissionUVs != nullptr;
                            MFA_ASSERT(emissionUVs != nullptr == emissiveTextureIndex >= 0);
                            bool hasTangentValue = tangentValues != nullptr;
                            MFA_ASSERT(hasTangentValue == hasNormalTexture);
                            for (U32 i = 0; i < primitiveVertexCount; ++i) {
                                subMeshVertices.emplace_back();
                                auto & vertex = subMeshVertices.back();
                                if (hasPosition) {// Vertices
                                    vertex.position[0] = positions[i * 3 + 0];
                                    MFA_ASSERT(vertex.position[0] >= positionsMinValue[0]);
                                    MFA_ASSERT(vertex.position[0] <= positionsMaxValue[0]);
                                    vertex.position[1] = positions[i * 3 + 1];
                                    MFA_ASSERT(vertex.position[1] >= positionsMinValue[1]);
                                    MFA_ASSERT(vertex.position[1] <= positionsMaxValue[1]);
                                    vertex.position[2] = positions[i * 3 + 2];
                                    MFA_ASSERT(vertex.position[2] >= positionsMinValue[2]);
                                    MFA_ASSERT(vertex.position[2] <= positionsMaxValue[2]);
                                }
                                if(hasNormalValue) {// Normal values
                                    vertex.normalValue[0] = normalValues[i * 3 + 0];
                                    MFA_ASSERT(vertex.normalValue[0] >= normalsValuesMin[0]);
                                    MFA_ASSERT(vertex.normalValue[0] <= normalsValuesMax[0]);
                                    vertex.normalValue[1] = normalValues[i * 3 + 1];
                                    MFA_ASSERT(vertex.normalValue[1] >= normalsValuesMin[1]);
                                    MFA_ASSERT(vertex.normalValue[1] <= normalsValuesMax[1]);
                                    vertex.normalValue[2] = normalValues[i * 3 + 2];
                                    MFA_ASSERT(vertex.normalValue[2] >= normalsValuesMin[2]);
                                    MFA_ASSERT(vertex.normalValue[2] <= normalsValuesMax[2]);
                                }
                                if(hasNormalTexture) {// Normal uvs
                                    vertex.normalMapUV[0] = normalsUVs[i * 2 + 0];
                                    MFA_ASSERT(vertex.normalMapUV[0] >= normals_uv_min[0]);
                                    MFA_ASSERT(vertex.normalMapUV[0] <= normals_uv_max[0]);
                                    vertex.normalMapUV[1] = normalsUVs[i * 2 + 1];
                                    MFA_ASSERT(vertex.normalMapUV[1] >= normals_uv_min[1]);
                                    MFA_ASSERT(vertex.normalMapUV[1] <= normals_uv_max[1]);
                                }
                                if(hasTangentValue) {// Tangent
                                    vertex.tangentValue[0] = tangentValues[i * 4 + 0];
                                    MFA_ASSERT(vertex.tangentValue[0] >= tangentsValuesMin[0]);
                                    MFA_ASSERT(vertex.tangentValue[0] <= tangentsValuesMax[0]);
                                    vertex.tangentValue[1] = tangentValues[i * 4 + 1];
                                    MFA_ASSERT(vertex.tangentValue[1] >= tangentsValuesMin[1]);
                                    MFA_ASSERT(vertex.tangentValue[1] <= tangentsValuesMax[1]);
                                    vertex.tangentValue[2] = tangentValues[i * 4 + 2];
                                    MFA_ASSERT(vertex.tangentValue[2] >= tangentsValuesMin[2]);
                                    MFA_ASSERT(vertex.tangentValue[2] <= tangentsValuesMax[2]);
                                    vertex.tangentValue[3] = tangentValues[i * 4 + 3];
                                    MFA_ASSERT(vertex.tangentValue[3] >= tangentsValuesMin[3]);
                                    MFA_ASSERT(vertex.tangentValue[3] <= tangentsValuesMax[3]);
                                }
                                if(hasEmissiveTexture) {// Emissive
                                    vertex.emissionUV[0] = emissionUVs[i * 2 + 0];
                                    MFA_ASSERT(vertex.emissionUV[0] >= emission_uv_min[0]);
                                    MFA_ASSERT(vertex.emissionUV[0] <= emission_uv_max[0]);
                                    vertex.emissionUV[1] = emissionUVs[i * 2 + 1];
                                    MFA_ASSERT(vertex.emissionUV[1] >= emission_uv_min[1]);
                                    MFA_ASSERT(vertex.emissionUV[1] <= emission_uv_max[1]);
                                }
                                if (hasBaseColorTexture) {// BaseColor
                                    vertex.baseColorUV[0] = baseColorUvs[i * 2 + 0];
                                    MFA_ASSERT(vertex.baseColorUV[0] >= baseColorUvMin[0]);
                                    MFA_ASSERT(vertex.baseColorUV[0] <= baseColorUvMax[0]);
                                    vertex.baseColorUV[1] = baseColorUvs[i * 2 + 1];
                                    MFA_ASSERT(vertex.baseColorUV[1] >= baseColorUvMin[1]);
                                    MFA_ASSERT(vertex.baseColorUV[1] <= baseColorUvMax[1]);
                                }
                                if (hasCombinedMetallicRoughness) {// MetallicRoughness
                                    vertex.roughnessUV[0] = metallicRoughnessUvs[i * 2 + 0];
                                    MFA_ASSERT(vertex.roughnessUV[0] >= metallicRoughnessUVMin[0]);
                                    MFA_ASSERT(vertex.roughnessUV[0] <= metallicRoughnessUVMax[0]);
                                    vertex.roughnessUV[1] = metallicRoughnessUvs[i * 2 + 1];
                                    MFA_ASSERT(vertex.roughnessUV[1] >= metallicRoughnessUVMin[1]);
                                    MFA_ASSERT(vertex.roughnessUV[1] <= metallicRoughnessUVMax[1]);

                                    ::memcpy(vertex.metallicUV, vertex.roughnessUV, sizeof(vertex.roughnessUV));
                                    static_assert(sizeof(vertex.roughnessUV) == sizeof(vertex.metallicUV));
                                }
                                // TODO WTF ?
                                vertex.color[0] = static_cast<U8>((256/(colorsMinMaxDiff[0])) * colors[i * 3 + 0]);
                                vertex.color[1] = static_cast<U8>((256/(colorsMinMaxDiff[1])) * colors[i * 3 + 1]);
                                vertex.color[2] = static_cast<U8>((256/(colorsMinMaxDiff[2])) * colors[i * 3 + 2]);
                            }

                            // Creating new subMesh
                            resultModel.mesh.insertPrimitive(
                                meshIndex,
                                AS::MeshPrimitive {
                                    .vertexCount = static_cast<U32>(subMeshVertices.size()),
                                    .indicesCount = static_cast<U32>(subMeshIndices.size()),
                                    .baseColorTextureIndex = baseColorTextureIndex,
                                    .metallicRoughnessTextureIndex = metallicRoughnessTextureIndex,
                                    .normalTextureIndex = normalTextureIndex,
                                    .emissiveTextureIndex = emissiveTextureIndex,
                                    .baseColorFactor = {baseColorFactor[0], baseColorFactor[1], baseColorFactor[2], baseColorFactor[3]},
                                    .metallicFactor = metallicFactor,
                                    .roughnessFactor = roughnessFactor,
                                    .emissiveFactor = {emissiveFactor[0], emissiveFactor[1], emissiveFactor[2]},
                                    .hasBaseColorTexture = hasBaseColorTexture,
                                    .hasEmissiveTexture = hasEmissiveTexture,
                                    .hasMixedMetallicRoughnessOcclusionTexture = hasCombinedMetallicRoughness,
                                    .hasNormalBuffer = hasNormalValue,
                                    .hasNormalTexture = hasNormalTexture,
                                    .hasTangentBuffer = hasTangentValue
                                }, 
                                static_cast<U32>(subMeshVertices.size()), 
                                subMeshVertices.data(), 
                                static_cast<U32>(subMeshIndices.size()), 
                                subMeshIndices.data()
                            );

                            ++ subMeshIndex;
                        }
                    }
                }
            }
            // Step3: Fill nodes
            if(false == gltfModel.nodes.empty()) {
                for (auto const & node : gltfModel.nodes) {
                    AS::MeshNode assetNode = {
                        .subMeshIndex = node.mesh,
                        .children = node.children,
                        .transformMatrix {static_cast<U32>(node.matrix.size()), node.matrix.data()}
                    };
                    resultModel.mesh.insertNode(assetNode);
                }
            }
        }
    }
    return resultModel;
}

bool FreeModel(AS::Model * model) {
    bool success = false;
    MFA_ASSERT(model != nullptr);
    bool freeResult = false;
    if(model != nullptr) {
        // Mesh
        freeResult = FreeMesh(&model->mesh);
        MFA_ASSERT(freeResult);
        // Textures
        if(false == model->textures.empty()) {
            for (auto & texture : model->textures) {
                freeResult = FreeTexture(&texture);
                MFA_ASSERT(freeResult);
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
        Blob vertexBuffer {};
        Blob indexBuffer {};
        mesh->revokeBuffers(vertexBuffer, indexBuffer);
        Memory::Free(vertexBuffer);
        Memory::Free(indexBuffer);
        success = true;
    }
    return success;
}

RawFile ReadRawFile(char const * path) {
    RawFile ret {};
    MFA_ASSERT(path != nullptr);
    if(path != nullptr) {
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

bool FreeRawFile (RawFile * rawFile) {
    bool ret = false;
    MFA_ASSERT(rawFile != nullptr);
    MFA_ASSERT(rawFile->valid());
    if(rawFile != nullptr && rawFile->valid()) {
        Memory::Free(rawFile->data);
        rawFile->data = {};
        ret = true; 
    }
    return ret;
}

}
