#include "Importer.hpp"

#include "../engine/BedrockMemory.hpp"
#include "../engine/BedrockFileSystem.hpp"
#include "../tools/ImageUtils.hpp"
#include "libs/tiny_obj_loader/tiny_obj_loader.h"
#include "libs/tiny_gltf_loader/tiny_gltf_loader.h"

#include <glm/gtx/quaternion.hpp>

namespace MFA::Importer {

namespace FS = FileSystem;

AS::Texture ImportUncompressedImage(
    char const * path,
    ImportTextureOptions const & options
) {
    AS::Texture texture {};
    Utils::UncompressedTexture::Data imageData {};
    MFA_DEFER {
        Utils::UncompressedTexture::Unload(&imageData);
    };
    auto const use_srgb = options.preferSrgb;
    auto const load_image_result = Utils::UncompressedTexture::Load(
        imageData, 
        path, 
        use_srgb
    );
    if(load_image_result == Utils::UncompressedTexture::LoadResult::Success) {
        MFA_ASSERT(imageData.valid());
        auto const image_width = imageData.width;
        auto const image_height = imageData.height;
        auto const pixels = imageData.pixels;
        auto const components = imageData.components;
        auto const format = imageData.format;
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
    int32_t const width,
    int32_t const height,
    AS::TextureFormat const format,
    uint32_t const components,
    uint16_t const depth,
    uint16_t const slices,
    ImportTextureOptions const & options
) {
    using namespace Utils::UncompressedTexture;

    auto const useSrgb = options.preferSrgb;
    AS::Texture::Dimensions const originalImageDimension {
        static_cast<uint32_t>(width),
        static_cast<uint32_t>(height),
        depth
    };
    uint8_t const mipCount = options.tryToGenerateMipmaps
        ? AS::Texture::ComputeMipCount(originalImageDimension)
        : 1;

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
        depth,
        options.sampler,
        Memory::Alloc(bufferSize)
    );

    // Generating mipmaps (TODO : Code needs debugging)
    texture.addMipmap(originalImageDimension, originalImagePixels);
    
    for (uint8_t mipLevel = 1; mipLevel < mipCount; mipLevel++) {
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

        {// Resize
            ResizeInputParams inputParams {};
            inputParams.inputImagePixels = originalImagePixels;
            inputParams.inputImageWidth = static_cast<int>(originalImageDimension.width);
            inputParams.inputImageHeight = static_cast<int>(originalImageDimension.height);
            inputParams.componentsCount = components;
            inputParams.outputImagePixels = mipMapPixels;
            inputParams.outputWidth = static_cast<int>(currentMipDims.width);
            inputParams.outputHeight = static_cast<int>(currentMipDims.height);
            auto const resizeResult = Utils::UncompressedTexture::Resize(inputParams);
            MFA_ASSERT(resizeResult == true);
        }

        texture.addMipmap(
            currentMipDims,
            mipMapPixels
        );
    }
    

    MFA_ASSERT(texture.isValid());
    if(texture.isValid() == false) {// Pointer being valid means that process has failed
        Memory::Free(texture.revokeBuffer());
    }

    return texture;
}


AS::Texture ImportKTXImage(char const * path, ImportTextureOptions const & options) {
    using namespace Utils::KTXTexture;

    AS::Texture result {};

    LoadResult loadResult = LoadResult::Invalid;
    auto * imageInfo = Load(loadResult, path);
    
    if(MFA_VERIFY(loadResult == LoadResult::Success && imageInfo != nullptr)) {

        MFA_DEFER {
            Unload(imageInfo);
        };

        result.initForWrite(
            imageInfo->format, 
            imageInfo->sliceCount, 
            imageInfo->depth,
            options.sampler,
            Memory::Alloc(imageInfo->totalImageSize)
        );

        auto width = imageInfo->width;
        auto height = imageInfo->height;
        auto depth = imageInfo->depth;

        for(auto i = 0u; i < imageInfo->mipmapCount; ++i) {
            MFA_ASSERT(width >= 1);
            MFA_ASSERT(height >= 1);
            MFA_ASSERT(depth >= 1);

            auto const mipBlob = GetMipBlob(imageInfo, i);
            if (!MFA_VERIFY(mipBlob.ptr != nullptr && mipBlob.len > 0)) {
                return AS::Texture {};
            }

            {
                AS::Texture::Dimensions dimensions {};
                dimensions.width = static_cast<uint32_t>(width);
                dimensions.height = static_cast<uint32_t>(height);
                dimensions.depth = static_cast<uint16_t>(depth);
                result.addMipmap(
                    dimensions, 
                    mipBlob
                );
            }

            width = Math::Max<uint16_t>(width / 2, 1);
            height = Math::Max<uint16_t>(height / 2, 1);
            depth = Math::Max<uint16_t>(depth / 2, 1);
        }
    }
    return result;
}

AS::Texture ImportDDSFile(char const * path) {
    // TODO
    MFA_NOT_IMPLEMENTED_YET("Mohammad Fakhreddin");
}

AS::Texture ImportImage(char const * path, ImportTextureOptions const & options) {
    AS::Texture texture {};

    MFA_ASSERT(path != nullptr);
    if (path != nullptr) {
        auto const extension = FS::ExtractExtensionFromPath(path);

        if (extension == ".png" || extension == ".jpg" || extension == ".jpeg") {
            texture = ImportUncompressedImage(path, options);
        } else if (extension == ".ktx") {
            texture = ImportKTXImage(path, options);
        }
    }

    return texture;
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
#if defined(__DESKTOP__) || defined(__IOS__)
        auto * file = FS::OpenFile(path,  FS::Usage::Read);
        MFA_DEFER{FS::CloseFile(file);};
        if(FS::FileIsUsable(file)) {
            auto const fileSize = FS::FileSize(file);
            MFA_ASSERT(fileSize > 0);

            auto const buffer = Memory::Alloc(fileSize);

            auto const readBytes = FS::Read(file, buffer);

            if(readBytes != buffer.len) {
                Memory::Free(buffer);
            } else {
                shader.init(entryPoint, stage, buffer);
            }
        }
#elif defined(__ANDROID__)
        auto * file = FS::Android_OpenAsset(path);
        MFA_DEFER {FS::Android_CloseAsset(file);};
        if (FS::Android_AssetIsUsable(file)) {
            auto const fileSize = FS::Android_AssetSize(file);
            MFA_ASSERT(fileSize > 0);

            auto const buffer = Memory::Alloc(fileSize);

            auto const readBytes = FS::Android_ReadAsset(file, buffer);
            if(readBytes != buffer.len) {
                shader.revokeData();
                Memory::Free(buffer);
            } else {
                shader.init(entryPoint, stage, buffer);
            }
        }
#else
#error "Os not handled"
#endif
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
    if(FS::Exists(path)){
        auto * file = FS::OpenFile(path, FS::Usage::Read);
        MFA_DEFER {FS::CloseFile(file);};
        if(FS::FileIsUsable(file)) {
            bool is_counter_clockwise = false;
            {//Check if normal vectors are reverse
                auto first_line_blob = Memory::Alloc(200);
                MFA_DEFER {Memory::Free(first_line_blob);};
                FS::Read(file, first_line_blob);
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
                auto const vertexCount = static_cast<uint32_t>(positions_count);
                auto const indexCount = static_cast<uint32_t>(shapes[0].mesh.indices.size());
                mesh.InitForWrite(
                    vertexCount, 
                    indexCount, 
                    Memory::Alloc(sizeof(AS::Mesh::Vertex) * vertexCount),
                    Memory::Alloc(sizeof(AS::Mesh::Index) * indexCount)
                );

                auto const subMeshIndex = mesh.InsertSubMesh();
                
                MFA_DEFER {
                    if (mesh.IsValid() == false) {
                        Blob vertexBuffer {};
                        Blob indexBuffer {};
                        mesh.RevokeBuffers(vertexBuffer, indexBuffer);
                        Memory::Free(vertexBuffer);
                        Memory::Free(indexBuffer);
                    }
                };
                
                std::vector<AS::Mesh::Vertex> vertices (vertexCount);
                std::vector<AS::Mesh::Index> indices (indexCount);
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

                {// Insert primitive
                    AS::Mesh::Primitive primitive {};
                    primitive.uniqueId = 0;
                    primitive.vertexCount = vertexCount;
                    primitive.indicesCount = indexCount;
                    primitive.baseColorTextureIndex = 0;
                    primitive.hasNormalBuffer = true;

                    mesh.InsertPrimitive(
                        subMeshIndex,
                        primitive,
                        static_cast<uint32_t>(vertices.size()),
                        vertices.data(),
                        static_cast<uint32_t>(indices.size()),
                        indices.data()
                    );
                }

                auto node = AS::MeshNode {};
                node.subMeshIndex = static_cast<int>(subMeshIndex);
                Copy<16>(node.transform, Matrix4X4Float::Identity().cells);
                mesh.InsertNode(node);
                MFA_ASSERT(mesh.IsValid());

            } else if (!error.empty() && error.substr(0, 4) != "WARN") {
                MFA_CRASH("LoadObj returned error: %s, File: %s", error.c_str(), path);
            } else {
                MFA_CRASH("LoadObj failed");
            }
        }
    }
    return mesh;
}

template<typename ItemType>
static void GLTF_extractDataFromBuffer(
    tinygltf::Model & gltfModel,
    int accessorIndex,
    int expectedComponentType,
    ItemType const * & outData,
    uint32_t & outDataCount 
) {
    MFA_ASSERT(accessorIndex >= 0);
    auto const & accessor = gltfModel.accessors[accessorIndex];
    MFA_ASSERT(accessor.componentType == expectedComponentType);
    auto const & bufferView = gltfModel.bufferViews[accessor.bufferView];
    MFA_REQUIRE(bufferView.buffer < gltfModel.buffers.size());
    auto const & buffer = gltfModel.buffers[bufferView.buffer];
    outData = reinterpret_cast<ItemType const *>(
        &buffer.data[bufferView.byteOffset + accessor.byteOffset]
    );
    outDataCount = static_cast<uint32_t>(accessor.count);
}

static void GLTF_extractDataAndTypeFromBuffer(
    tinygltf::Model & gltfModel,
    int accessorIndex,
    int expectedComponentType,
    int & outType,
    void const * & outData,
    uint32_t & outDataCount 
) {
    MFA_ASSERT(accessorIndex >= 0);
    auto const & accessor = gltfModel.accessors[accessorIndex];
    outType = accessor.type;
    MFA_ASSERT(expectedComponentType == accessor.componentType);
    auto const & bufferView = gltfModel.bufferViews[accessor.bufferView];
    MFA_REQUIRE(bufferView.buffer < gltfModel.buffers.size());
    auto const & buffer = gltfModel.buffers[bufferView.buffer];
    outData = reinterpret_cast<void const *>(
        &buffer.data[bufferView.byteOffset + accessor.byteOffset]
    );
    outDataCount = static_cast<uint32_t>(accessor.count);
}

template<typename ItemType>
static bool GLTF_extractPrimitiveDataFromBuffer(
    tinygltf::Model & gltfModel,
    tinygltf::Primitive & primitive,
    char const * fieldKey,
    size_t const expectedComponentCount,
    int expectedComponentType,
    ItemType const * & outData,
    uint32_t & outDataCount,
    bool & outHasMinMax,
    ItemType * outMin,
    ItemType * outMax
) {
    bool success = false;
    outHasMinMax = false;
    if(primitive.attributes.find(fieldKey) != primitive.attributes.end()){// Positions
        success = true;
        MFA_REQUIRE(static_cast<size_t>(primitive.attributes[fieldKey]) < gltfModel.accessors.size());
        auto const & accessor = gltfModel.accessors[primitive.attributes[fieldKey]];
        MFA_ASSERT(accessor.componentType == expectedComponentType);
        if (
            outMin != nullptr && outMax != nullptr &&
            accessor.minValues.size() == expectedComponentCount && 
            accessor.maxValues.size() == expectedComponentCount
        ) {
            for (size_t i = 0; i < expectedComponentCount; ++i) {
                outMin[i] = static_cast<ItemType>(accessor.minValues[i]);
                outMax[i] = static_cast<ItemType>(accessor.maxValues[i]);
            }
            outHasMinMax = true;
        }
        auto const & bufferView = gltfModel.bufferViews[accessor.bufferView];
        MFA_REQUIRE(static_cast<size_t>(bufferView.buffer) < gltfModel.buffers.size());
        auto const & buffer = gltfModel.buffers[bufferView.buffer];
        outData = reinterpret_cast<ItemType const *>(
            &buffer.data[bufferView.byteOffset + accessor.byteOffset]
        );
        outDataCount = static_cast<uint32_t>(accessor.count);
    }
    return success;
}

template<typename ItemType>
static bool GLTF_extractPrimitiveDataFromBuffer(
    tinygltf::Model & gltfModel,
    tinygltf::Primitive & primitive,
    char const * fieldKey,
    size_t const expectedComponentCount,
    int const expectedComponentType,
    ItemType const * & outData,
    uint32_t & outDataCount
) {
    bool hasMinMax = false;
    return GLTF_extractPrimitiveDataFromBuffer<ItemType>(
        gltfModel,
        primitive,
        fieldKey,
        expectedComponentCount,
        expectedComponentType,
        outData,
        outDataCount,
        hasMinMax,
        nullptr,
        nullptr
    );
}

struct TextureRef {
    std::string gltf_name;
    uint8_t index;
};

static void GLTF_extractTextures(
    char const * path,
    tinygltf::Model & gltfModel, 
    std::vector<TextureRef> & outTextureRefs,
    AS::Model & outResultModel
) {
    std::string const directoryPath = FS::ExtractDirectoryFromPath(path);
    // Extracting textures
    if(false == gltfModel.textures.empty()) {
        for (auto const & texture : gltfModel.textures) {
            AS::SamplerConfig sampler {};
            sampler.isValid = false;
            if (texture.sampler >= 0) {// Sampler
                auto const & gltfSampler = gltfModel.samplers[texture.sampler];
                sampler.magFilter = gltfSampler.magFilter;
                sampler.minFilter = gltfSampler.minFilter;
                sampler.wrapS = gltfSampler.wrapS;
                sampler.wrapT = gltfSampler.wrapT;
                //sampler.sample_mode = gltf_sampler. // TODO
                sampler.isValid = true;
            }
            AS::Texture assetSystemTexture {};
            auto const & image = gltfModel.images[texture.source];
            {// Texture
                ImportTextureOptions textureOptions {};
                textureOptions.tryToGenerateMipmaps = false;
                textureOptions.sampler = &sampler;
                std::string image_path = directoryPath + "/" + image.uri;
                assetSystemTexture = ImportImage(
                    image_path.c_str(),
                    // TODO tryToGenerateMipmaps takes too long (We should create .asset files)
                    textureOptions
                );
            }
            MFA_ASSERT(assetSystemTexture.isValid());
            {
                TextureRef textureRef {};
                textureRef.gltf_name = image.uri;
                textureRef.index = static_cast<uint8_t>(outResultModel.textures.size());
                outTextureRefs.emplace_back(textureRef);
            }
            outResultModel.textures.emplace_back(assetSystemTexture);
        }
    }
}

static int16_t GLTF_findTextureByName(
    char const * textureName,
    std::vector<TextureRef> const & textureRefs    
) {
    MFA_ASSERT(textureName != nullptr);
    if(false == textureRefs.empty()) {
        for (auto const & textureRef : textureRefs) {
            if(textureRef.gltf_name == textureName) {
                return textureRef.index;
            }
        }
    }
    return -1;
    //MFA_CRASH("Image not found: %s", gltf_name);
}

static void GLTF_extractSubMeshes(
    tinygltf::Model & gltfModel,
    std::vector<TextureRef> const & textureRefs,
    AS::Model & outResultModel
) {
    auto const generateUvKeyword = [](int32_t const uvIndex) -> std::string{
        return "TEXCOORD_" + std::to_string(uvIndex);
    };
    // Step1: Iterate over all meshes and gather required information for asset buffer
    uint32_t totalIndicesCount = 0;
    uint32_t totalVerticesCount = 0;
    uint32_t indicesVertexStartingIndex = 0;
    for(auto & mesh : gltfModel.meshes) {
        if(false == mesh.primitives.empty()) {
            for(auto & primitive : mesh.primitives) {
                {// Indices
                    MFA_REQUIRE((primitive.indices < gltfModel.accessors.size()));
                    auto const & accessor = gltfModel.accessors[primitive.indices];
                    totalIndicesCount += static_cast<uint32_t>(accessor.count);
                }
                {// Positions
                    MFA_REQUIRE((primitive.attributes["POSITION"] < gltfModel.accessors.size()));
                    auto const & accessor = gltfModel.accessors[primitive.attributes["POSITION"]];
                    totalVerticesCount += static_cast<uint32_t>(accessor.count);
                }
            }
        }
    }
    outResultModel.mesh.InitForWrite(
        totalVerticesCount, 
        totalIndicesCount, 
        Memory::Alloc(sizeof(AS::MeshVertex) * totalVerticesCount), 
        Memory::Alloc(sizeof(AS::MeshIndex) * totalIndicesCount)
    );
    // Step2: Fill subMeshes
    uint32_t primitiveUniqueId = 0;
    std::vector<AS::Mesh::Vertex> primitiveVertices {};
    std::vector<AS::MeshIndex> primitiveIndices {};
    for(auto & mesh : gltfModel.meshes) {
        auto const meshIndex = outResultModel.mesh.InsertSubMesh();
        if(false == mesh.primitives.empty()) {
            for(auto & primitive : mesh.primitives) {
                primitiveIndices.erase(primitiveIndices.begin(), primitiveIndices.end());
                primitiveVertices.erase(primitiveVertices.begin(), primitiveVertices.end());
                
                int16_t baseColorTextureIndex = -1;
                int32_t baseColorUvIndex = -1;
                int16_t metallicRoughnessTextureIndex = -1;
                int32_t metallicRoughnessUvIndex = -1;
                int16_t normalTextureIndex = -1;
                int32_t normalUvIndex = -1;
                int16_t emissiveTextureIndex = -1;
                int32_t emissiveUvIndex = -1;
                float baseColorFactor[4] {};
                float metallicFactor = 0;
                float roughnessFactor = 0;
                float emissiveFactor [3] {};
                uint32_t uniqueId = primitiveUniqueId;
                primitiveUniqueId++;
                if(primitive.material >= 0) {// Material
                    auto const & material = gltfModel.materials[primitive.material];
                    if (material.pbrMetallicRoughness.baseColorTexture.index >= 0) {// Base color texture
                        auto const & base_color_gltf_texture = gltfModel.textures[material.pbrMetallicRoughness.baseColorTexture.index];
                        auto const & image = gltfModel.images[base_color_gltf_texture.source];
                        baseColorTextureIndex = GLTF_findTextureByName(image.uri.c_str(), textureRefs);
                        if (baseColorTextureIndex >= 0) {
                            baseColorUvIndex = static_cast<uint16_t>(material.pbrMetallicRoughness.baseColorTexture.texCoord);
                        }
                    }
                    if (material.pbrMetallicRoughness.metallicRoughnessTexture.index >= 0){// Metallic-roughness texture
                        auto const & metallic_roughness_texture = gltfModel.textures[material.pbrMetallicRoughness.metallicRoughnessTexture.index];
                        auto const & image = gltfModel.images[metallic_roughness_texture.source];
                        metallicRoughnessTextureIndex = GLTF_findTextureByName(image.uri.c_str(), textureRefs);
                        if(metallicRoughnessTextureIndex >= 0) {
                            metallicRoughnessUvIndex = static_cast<uint16_t>(material.pbrMetallicRoughness.metallicRoughnessTexture.texCoord);
                        }
                    }
                    if (material.normalTexture.index >= 0) {// Normal texture
                        auto const & normal_texture = gltfModel.textures[material.normalTexture.index];
                        auto const & image = gltfModel.images[normal_texture.source];
                        normalTextureIndex = GLTF_findTextureByName(image.uri.c_str(), textureRefs);
                        if(normalTextureIndex >= 0) {
                            normalUvIndex = static_cast<uint16_t>(material.normalTexture.texCoord);
                        }
                    }
                    if (material.emissiveTexture.index >= 0) {// Emissive texture
                        auto const & emissive_texture = gltfModel.textures[material.emissiveTexture.index];
                        auto const & image = gltfModel.images[emissive_texture.source];
                        emissiveTextureIndex = GLTF_findTextureByName(image.uri.c_str(), textureRefs);
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
                uint32_t primitiveIndicesCount = 0;
                {// Indices
                    MFA_REQUIRE(primitive.indices < gltfModel.accessors.size());
                    auto const & accessor = gltfModel.accessors[primitive.indices];
                    auto const & bufferView = gltfModel.bufferViews[accessor.bufferView];
                    MFA_REQUIRE(bufferView.buffer < gltfModel.buffers.size());
                    auto const & buffer = gltfModel.buffers[bufferView.buffer];
                    primitiveIndicesCount = static_cast<uint32_t>(accessor.count);
                    
                    switch(accessor.componentType) {
                        case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT:
                        {
                            auto const * gltfIndices = reinterpret_cast<uint32_t const *>(
                                &buffer.data[bufferView.byteOffset + accessor.byteOffset]
                            );
                            for (uint32_t i = 0; i < primitiveIndicesCount; i++) {
                                primitiveIndices.emplace_back(gltfIndices[i] + indicesVertexStartingIndex);
                            }
                        }
                        break;
                        case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT:
                        {
                            auto const * gltfIndices = reinterpret_cast<uint16_t const *>(
                                &buffer.data[bufferView.byteOffset + accessor.byteOffset]
                            );
                            for (uint32_t i = 0; i < primitiveIndicesCount; i++) {
                                primitiveIndices.emplace_back(gltfIndices[i] + indicesVertexStartingIndex);
                            }
                        }
                        break;
                        case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE:
                        {
                            auto const * gltfIndices = reinterpret_cast<uint8_t const *>(
                                &buffer.data[bufferView.byteOffset + accessor.byteOffset]
                            );
                            for (uint32_t i = 0; i < primitiveIndicesCount; i++) {
                                primitiveIndices.emplace_back(gltfIndices[i] + indicesVertexStartingIndex);
                            }
                        }
                        break;
                        default:
                            MFA_NOT_IMPLEMENTED_YET("Mohammad Fakhreddin");
                    }
                }
                float const * positions = nullptr;
                uint32_t primitiveVertexCount = 0;
                float positionsMinValue [3] {};
                float positionsMaxValue [3] {};
                bool hasPositionMinMax = false;
                {// Position
                    auto const result = GLTF_extractPrimitiveDataFromBuffer(
                        gltfModel, 
                        primitive, 
                        "POSITION", 
                        3, 
                        TINYGLTF_COMPONENT_TYPE_FLOAT, 
                        positions, 
                        primitiveVertexCount, 
                        hasPositionMinMax, 
                        positionsMinValue, 
                        positionsMaxValue
                    );
                    MFA_ASSERT(result);
                }
                float const * baseColorUvs = nullptr;
                float baseColorUvMin [2] {};
                float baseColorUvMax [2] {};
                bool hasBaseColorUvMinMax = false;
                if(baseColorUvIndex >= 0) {// BaseColor
                    uint32_t baseColorUvsCount = 0;
                    auto texture_coordinate_key_name = generateUvKeyword(baseColorUvIndex);
                    auto const result = GLTF_extractPrimitiveDataFromBuffer(
                        gltfModel,
                        primitive,
                        texture_coordinate_key_name.c_str(),
                        2,
                        TINYGLTF_COMPONENT_TYPE_FLOAT,
                        baseColorUvs,
                        baseColorUvsCount,
                        hasBaseColorUvMinMax,
                        baseColorUvMin,
                        baseColorUvMax
                    );
                    MFA_ASSERT(result == true);
                    MFA_ASSERT(baseColorUvsCount == primitiveVertexCount);
                }
                float const * metallicRoughnessUvs = nullptr;
                float metallicRoughnessUVMin [2] {};
                float metallicRoughnessUVMax [2] {};
                bool hasMetallicRoughnessUvMinMax = false;
                if(metallicRoughnessUvIndex >= 0) {// MetallicRoughness uvs
                    std::string texture_coordinate_key_name = generateUvKeyword(metallicRoughnessUvIndex);
                    uint32_t metallicRoughnessUvsCount = 0;
                    auto const result = GLTF_extractPrimitiveDataFromBuffer(
                        gltfModel,
                        primitive,
                        texture_coordinate_key_name.c_str(),
                        2,
                        TINYGLTF_COMPONENT_TYPE_FLOAT,
                        metallicRoughnessUvs,
                        metallicRoughnessUvsCount,
                        hasMetallicRoughnessUvMinMax,
                        metallicRoughnessUVMin,
                        metallicRoughnessUVMax
                    );
                    MFA_ASSERT(result == true);
                    MFA_ASSERT(metallicRoughnessUvsCount == primitiveVertexCount);
                }
                // TODO Occlusion texture
                float const * emissionUVs = nullptr;
                float emission_uv_min [2] {};
                float emission_uv_max [2] {};
                bool hasEmissionUvMinMax = false;
                if(emissiveUvIndex >= 0) {// Emission uvs
                    std::string texture_coordinate_key_name = generateUvKeyword(emissiveUvIndex);
                    uint32_t emissionUvCount = 0;
                    auto const result = GLTF_extractPrimitiveDataFromBuffer(
                        gltfModel,
                        primitive,
                        texture_coordinate_key_name.c_str(),
                        2,
                        TINYGLTF_COMPONENT_TYPE_FLOAT,
                        emissionUVs,
                        emissionUvCount,
                        hasEmissionUvMinMax,
                        emission_uv_min,
                        emission_uv_max
                    );
                    MFA_ASSERT(result == true);
                    MFA_ASSERT(emissionUvCount == primitiveVertexCount);
                }
                float const * normalsUVs = nullptr;
                float normals_uv_min [2] {};
                float normals_uv_max [2] {};
                bool hasNormalUvMinMax = false;
                if(normalUvIndex >= 0) {// Normal uvs
                    std::string texture_coordinate_key_name = generateUvKeyword(normalUvIndex);
                    uint32_t normalUvsCount = 0;
                    auto const result = GLTF_extractPrimitiveDataFromBuffer(
                        gltfModel,
                        primitive,
                        texture_coordinate_key_name.c_str(),
                        2,
                        TINYGLTF_COMPONENT_TYPE_FLOAT,
                        normalsUVs,
                        normalUvsCount,
                        hasNormalUvMinMax,
                        normals_uv_min,
                        normals_uv_max
                    );
                    MFA_ASSERT(result == true);
                    MFA_ASSERT(normalUvsCount == primitiveVertexCount);
                }
                float const * normalValues = nullptr;
                float normalsValuesMin [3] {};
                float normalsValuesMax [3] {};
                bool hasNormalValueMinMax = false;
                {// Normal uvs
                    uint32_t normalValuesCount = 0;
                    auto const result = GLTF_extractPrimitiveDataFromBuffer(
                        gltfModel,
                        primitive,
                        "NORMAL",
                        3,
                        TINYGLTF_COMPONENT_TYPE_FLOAT,
                        normalValues,
                        normalValuesCount,
                        hasNormalValueMinMax,
                        normalsValuesMin,
                        normalsValuesMax
                    );
                    MFA_ASSERT(result == false || normalValuesCount == primitiveVertexCount);                           
                }
                float const * tangentValues = nullptr;
                float tangentsValuesMin [4] {};
                float tangentsValuesMax [4] {};
                bool hasTangentsValuesMinMax = false;
                {// Tangent values
                    uint32_t tangentValuesCount = 0;
                    auto const result = GLTF_extractPrimitiveDataFromBuffer(
                        gltfModel,
                        primitive,
                        "TANGENT",
                        4,
                        TINYGLTF_COMPONENT_TYPE_FLOAT,
                        tangentValues,
                        tangentValuesCount,
                        hasTangentsValuesMinMax,
                        tangentsValuesMin,
                        tangentsValuesMax
                    );
                    MFA_ASSERT(result == false || tangentValuesCount == primitiveVertexCount);                           
                }
                uint16_t const * jointValues = nullptr;
                uint32_t jointValuesCount = 0;
                {// Joints
                    GLTF_extractPrimitiveDataFromBuffer(
                        gltfModel,
                        primitive,
                        "JOINTS_0",
                        4,
                        TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT,
                        jointValues,
                        jointValuesCount
                    );
                }
                float const * weightValues = nullptr;
                uint32_t weightValuesCount = 0;
                {// Weights
                    GLTF_extractPrimitiveDataFromBuffer(
                        gltfModel,
                        primitive,
                        "WEIGHTS_0",
                        4,
                        TINYGLTF_COMPONENT_TYPE_FLOAT,
                        weightValues,
                        weightValuesCount
                    );
                }
                MFA_ASSERT(weightValuesCount == jointValuesCount);
                // TODO Start from here, Assign weight and joint
                float const * colors = nullptr;
                float colorsMinValue [3] {0};
                float colorsMaxValue [3] {1};
                float colorsMinMaxDiff [3] {1};
                if(primitive.attributes["COLOR"] >= 0) {
                    MFA_REQUIRE(primitive.attributes["COLOR"] < gltfModel.accessors.size());
                    auto const & accessor = gltfModel.accessors[primitive.attributes["COLOR"]];
                    //MFA_ASSERT(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);
                    //TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT
                    if (accessor.minValues.size() == 3 && accessor.maxValues.size() == 3) {
                        colorsMinValue[0] = static_cast<float>(accessor.minValues[0]);
                        colorsMinValue[1] = static_cast<float>(accessor.minValues[1]);
                        colorsMinValue[2] = static_cast<float>(accessor.minValues[2]);
                        colorsMaxValue[0] = static_cast<float>(accessor.maxValues[0]);
                        colorsMaxValue[1] = static_cast<float>(accessor.maxValues[1]);
                        colorsMaxValue[2] = static_cast<float>(accessor.maxValues[2]);
                        colorsMinMaxDiff[0] = colorsMaxValue[0] - colorsMinValue[0];
                        colorsMinMaxDiff[1] = colorsMaxValue[1] - colorsMinValue[1];
                        colorsMinMaxDiff[2] = colorsMaxValue[2] - colorsMinValue[2];
                    }
                    auto const & bufferView = gltfModel.bufferViews[accessor.bufferView];
                    MFA_REQUIRE(bufferView.buffer < gltfModel.buffers.size());
                    auto const & buffer = gltfModel.buffers[bufferView.buffer];
                    colors = reinterpret_cast<const float *>(
                        &buffer.data[bufferView.byteOffset + accessor.byteOffset]
                    );
                }
                
                bool hasPosition = positions != nullptr;
                MFA_ASSERT(hasPosition == true);
                bool hasBaseColorTexture = baseColorUvs != nullptr;
                MFA_ASSERT(baseColorUvs != nullptr == baseColorTextureIndex >= 0);
                bool hasNormalValue = normalValues != nullptr;
                MFA_ASSERT(hasNormalValue == true);
                bool hasNormalTexture = normalsUVs != nullptr;
                MFA_ASSERT(normalsUVs != nullptr == normalTextureIndex >= 0);
                bool hasCombinedMetallicRoughness = metallicRoughnessUvs != nullptr;
                MFA_ASSERT(metallicRoughnessUvs != nullptr == metallicRoughnessTextureIndex >= 0);
                bool hasEmissiveTexture = emissionUVs != nullptr;
                MFA_ASSERT(emissionUVs != nullptr == emissiveTextureIndex >= 0);
                bool hasTangentValue = tangentValues != nullptr;
                bool hasSkin = jointValues != nullptr && weightValues != nullptr;
                for (uint32_t i = 0; i < primitiveVertexCount; ++i) {
                    primitiveVertices.emplace_back();
                    auto & vertex = primitiveVertices.back();
                    if (hasPosition) {// Vertices
                        vertex.position[0] = positions[i * 3 + 0];
                        vertex.position[1] = positions[i * 3 + 1];
                        vertex.position[2] = positions[i * 3 + 2];
                        if (hasPositionMinMax) {
                            MFA_ASSERT(vertex.position[0] >= positionsMinValue[0]);
                            MFA_ASSERT(vertex.position[0] <= positionsMaxValue[0]);
                            MFA_ASSERT(vertex.position[1] >= positionsMinValue[1]);
                            MFA_ASSERT(vertex.position[1] <= positionsMaxValue[1]);
                            MFA_ASSERT(vertex.position[2] >= positionsMinValue[2]);
                            MFA_ASSERT(vertex.position[2] <= positionsMaxValue[2]);
                        }
                    }
                    if(hasNormalValue) {// Normal values
                        vertex.normalValue[0] = normalValues[i * 3 + 0];
                        vertex.normalValue[1] = normalValues[i * 3 + 1];
                        vertex.normalValue[2] = normalValues[i * 3 + 2];
                        if (hasNormalValueMinMax) {
                            MFA_ASSERT(vertex.normalValue[0] >= normalsValuesMin[0]);
                            MFA_ASSERT(vertex.normalValue[0] <= normalsValuesMax[0]);
                            MFA_ASSERT(vertex.normalValue[1] >= normalsValuesMin[1]);
                            MFA_ASSERT(vertex.normalValue[1] <= normalsValuesMax[1]);
                            MFA_ASSERT(vertex.normalValue[2] >= normalsValuesMin[2]);
                            MFA_ASSERT(vertex.normalValue[2] <= normalsValuesMax[2]);
                        }
                    }
                    if(hasNormalTexture) {// Normal uvs
                        vertex.normalMapUV[0] = normalsUVs[i * 2 + 0];
                        vertex.normalMapUV[1] = normalsUVs[i * 2 + 1];
                        if (hasNormalUvMinMax) {
                            MFA_ASSERT(vertex.normalMapUV[0] >= normals_uv_min[0]);
                            MFA_ASSERT(vertex.normalMapUV[0] <= normals_uv_max[0]);
                            MFA_ASSERT(vertex.normalMapUV[1] >= normals_uv_min[1]);
                            MFA_ASSERT(vertex.normalMapUV[1] <= normals_uv_max[1]);
                        }
                    }
                    if(hasTangentValue) {// Tangent
                        vertex.tangentValue[0] = tangentValues[i * 4 + 0];
                        vertex.tangentValue[1] = tangentValues[i * 4 + 1];
                        vertex.tangentValue[2] = tangentValues[i * 4 + 2];
                        vertex.tangentValue[3] = tangentValues[i * 4 + 3];
                        if (hasTangentsValuesMinMax) {
                            MFA_ASSERT(vertex.tangentValue[0] >= tangentsValuesMin[0]);
                            MFA_ASSERT(vertex.tangentValue[0] <= tangentsValuesMax[0]);
                            MFA_ASSERT(vertex.tangentValue[1] >= tangentsValuesMin[1]);
                            MFA_ASSERT(vertex.tangentValue[1] <= tangentsValuesMax[1]);
                            MFA_ASSERT(vertex.tangentValue[2] >= tangentsValuesMin[2]);
                            MFA_ASSERT(vertex.tangentValue[2] <= tangentsValuesMax[2]);
                            MFA_ASSERT(vertex.tangentValue[3] >= tangentsValuesMin[3]);
                            MFA_ASSERT(vertex.tangentValue[3] <= tangentsValuesMax[3]);
                        }
                    }
                    if(hasEmissiveTexture) {// Emissive
                        vertex.emissionUV[0] = emissionUVs[i * 2 + 0];
                        vertex.emissionUV[1] = emissionUVs[i * 2 + 1];
                        if (hasEmissionUvMinMax) {
                            MFA_ASSERT(vertex.emissionUV[0] >= emission_uv_min[0]);
                            MFA_ASSERT(vertex.emissionUV[0] <= emission_uv_max[0]);
                            MFA_ASSERT(vertex.emissionUV[1] >= emission_uv_min[1]);
                            MFA_ASSERT(vertex.emissionUV[1] <= emission_uv_max[1]);
                        }
                    }
                    if (hasBaseColorTexture) {// BaseColor
                        vertex.baseColorUV[0] = baseColorUvs[i * 2 + 0];
                        vertex.baseColorUV[1] = baseColorUvs[i * 2 + 1];
                        if (hasBaseColorUvMinMax) {
                            MFA_ASSERT(vertex.baseColorUV[0] >= baseColorUvMin[0]);
                            MFA_ASSERT(vertex.baseColorUV[0] <= baseColorUvMax[0]);
                            MFA_ASSERT(vertex.baseColorUV[1] >= baseColorUvMin[1]);
                            MFA_ASSERT(vertex.baseColorUV[1] <= baseColorUvMax[1]);
                        }
                    }
                    if (hasCombinedMetallicRoughness) {// MetallicRoughness
                        vertex.roughnessUV[0] = metallicRoughnessUvs[i * 2 + 0];
                        vertex.roughnessUV[1] = metallicRoughnessUvs[i * 2 + 1];
                        ::memcpy(vertex.metallicUV, vertex.roughnessUV, sizeof(vertex.roughnessUV));
                        static_assert(sizeof(vertex.roughnessUV) == sizeof(vertex.metallicUV));
                        if (hasMetallicRoughnessUvMinMax) {
                            MFA_ASSERT(vertex.roughnessUV[0] >= metallicRoughnessUVMin[0]);
                            MFA_ASSERT(vertex.roughnessUV[0] <= metallicRoughnessUVMax[0]);
                            MFA_ASSERT(vertex.roughnessUV[1] >= metallicRoughnessUVMin[1]);
                            MFA_ASSERT(vertex.roughnessUV[1] <= metallicRoughnessUVMax[1]);
                        }
                    }
                    // TODO WTF ? Outside of range error. Why do we need color range anyways ?
                    // vertex.color[0] = static_cast<uint8_t>((256/(colorsMinMaxDiff[0])) * colors[i * 3 + 0]);
                    // vertex.color[1] = static_cast<uint8_t>((256/(colorsMinMaxDiff[1])) * colors[i * 3 + 1]);
                    // vertex.color[2] = static_cast<uint8_t>((256/(colorsMinMaxDiff[2])) * colors[i * 3 + 2]);

                    vertex.hasSkin = hasSkin ? 1 : 0;

                    // Joint and weight
                    if (hasSkin) {
                        vertex.jointIndices[0] = jointValues[i * 4 + 0];
                        vertex.jointIndices[1] = jointValues[i * 4 + 1];
                        vertex.jointIndices[2] = jointValues[i * 4 + 2];
                        vertex.jointIndices[3] = jointValues[i * 4 + 3];

                        vertex.jointWeights[0] = weightValues[i * 4 + 0];
                        vertex.jointWeights[1] = weightValues[i * 4 + 1];
                        vertex.jointWeights[2] = weightValues[i * 4 + 2];
                        vertex.jointWeights[3] = weightValues[i * 4 + 3];
                    }
                }

                {// Creating new subMesh
                    AS::MeshPrimitive primitive = {};
                    primitive.uniqueId = uniqueId;
                    primitive.baseColorTextureIndex = baseColorTextureIndex;
                    primitive.mixedMetallicRoughnessOcclusionTextureIndex = metallicRoughnessTextureIndex;
                    primitive.normalTextureIndex = normalTextureIndex;
                    primitive.emissiveTextureIndex = emissiveTextureIndex;
                    Copy<4>(primitive.baseColorFactor, baseColorFactor);
                    primitive.metallicFactor = metallicFactor;
                    primitive.roughnessFactor = roughnessFactor;
                    Copy<3>(primitive.emissiveFactor, emissiveFactor);
                    primitive.hasBaseColorTexture = hasBaseColorTexture;
                    primitive.hasEmissiveTexture = hasEmissiveTexture;
                    primitive.hasMixedMetallicRoughnessOcclusionTexture = hasCombinedMetallicRoughness;
                    primitive.hasNormalBuffer = hasNormalValue;
                    primitive.hasNormalTexture = hasNormalTexture;
                    primitive.hasTangentBuffer = hasTangentValue;
                    primitive.hasSkin = hasSkin;

                    outResultModel.mesh.InsertPrimitive(
                        meshIndex,
                        primitive, 
                        static_cast<uint32_t>(primitiveVertices.size()), 
                        primitiveVertices.data(), 
                        static_cast<uint32_t>(primitiveIndices.size()), 
                        primitiveIndices.data()
                    );
                }
                indicesVertexStartingIndex += primitiveVertexCount;
                
            }
        }
    }
}

static void GLTF_extractNodes(
    tinygltf::Model & gltfModel,
    AS::Model & outResultModel
) {
    // Step3: Fill nodes
    if(false == gltfModel.nodes.empty()) {
        for (auto const & gltfNode : gltfModel.nodes) {
            AS::MeshNode node {};
            node.subMeshIndex = gltfNode.mesh;
            node.children = gltfNode.children;
            node.skin = gltfNode.skin;

            if (gltfNode.translation.empty() == false) {
                MFA_ASSERT(gltfNode.translation.size() == 3);
                node.translate[0] = static_cast<float>(gltfNode.translation[0]);
                node.translate[1] = static_cast<float>(gltfNode.translation[1]);
                node.translate[2] = static_cast<float>(gltfNode.translation[2]);
            }
            if (gltfNode.rotation.empty() == false) {
                MFA_ASSERT(gltfNode.rotation.size() == 4);
                node.rotation[0] = static_cast<float>(gltfNode.rotation[0]);
                node.rotation[1] = static_cast<float>(gltfNode.rotation[1]);
                node.rotation[2] = static_cast<float>(gltfNode.rotation[2]);
                node.rotation[3] = static_cast<float>(gltfNode.rotation[3]);
            }
            if (gltfNode.scale.empty() == false) {
                MFA_ASSERT(gltfNode.scale.size() == 3);
                node.scale[0] = static_cast<float>(gltfNode.scale[0]);
                node.scale[1] = static_cast<float>(gltfNode.scale[1]);
                node.scale[2] = static_cast<float>(gltfNode.scale[2]);
            }
            if (gltfNode.matrix.empty() == false) {
                for (int i = 0; i < 16; ++i) {
                    node.transform[i] = static_cast<float>(gltfNode.matrix[i]);
                }
            }
            outResultModel.mesh.InsertNode(node);
        }
    }
}

void GLTF_extractSkins(
    tinygltf::Model & gltfModel,
    AS::Model & outResultModel
) {
    for (auto const & gltfSkin : gltfModel.skins) {
        AS::MeshSkin skin {};

        // Joints
        skin.joints.insert(skin.joints.end(), gltfSkin.joints.begin(), gltfSkin.joints.end());

        // InverseBindMatrices
        MFA_REQUIRE(gltfSkin.inverseBindMatrices >= 0);
        uint32_t inverseBindMatricesCount = 0;
        float const * inverseBindMatricesPtr = nullptr;
        GLTF_extractDataFromBuffer(
            gltfModel, 
            gltfSkin.inverseBindMatrices, 
            TINYGLTF_COMPONENT_TYPE_FLOAT,
            inverseBindMatricesPtr,
            inverseBindMatricesCount
        );
        MFA_ASSERT(inverseBindMatricesPtr != nullptr);
        skin.inverseBindMatrices.resize(inverseBindMatricesCount);
        for (size_t i = 0; i < skin.inverseBindMatrices.size(); ++i) {
            auto & currentMatrix = skin.inverseBindMatrices[i];
            for (size_t j = 0; j < 16; ++j) {
                currentMatrix.value[j] = inverseBindMatricesPtr[i * 16 + j];
            }
        }
        MFA_ASSERT(skin.inverseBindMatrices.size() == skin.joints.size());
        skin.skeletonRootNode = gltfSkin.skeleton;

        outResultModel.mesh.InsertSkin(skin);
    }
}

void GLTF_extractAnimations(
    tinygltf::Model & gltfModel,
    AS::Model & outResultModel
) {
    using Sampler = AS::MeshAnimation::Sampler;
    using Channel = AS::MeshAnimation::Channel;
    using Interpolation = AssetSystem::Mesh::Animation::Interpolation;
    using Path = AssetSystem::Mesh::Animation::Path;
    using Channel = AssetSystem::Mesh::Animation::Channel;
    
    auto const convertInterpolationToEnum = [](char const * value)-> Interpolation {
        if (strcmp("LINEAR", value) == 0) {
            return Interpolation::Linear;
        }
        if (strcmp("STEP", value) == 0) {
            return Interpolation::Step;
        }
        if (strcmp("CUBICSPLINE", value) == 0) {
            return Interpolation::CubicSpline;
        }
        MFA_CRASH("Unhandled test case");
        return Interpolation::Invalid;
    };

    auto const convertPathToEnum = [](char const * value)-> Path {
        if (strcmp("translation", value) == 0) {
            return Path::Translation;
        }
        if (strcmp("rotation", value) == 0) {
            return Path::Rotation;
        }
        if (strcmp("scale", value) == 0) {
            return Path::Scale;    
        }
        MFA_CRASH("Unhandled test case");
        return Path::Invalid;
    };

    for (auto const & gltfAnimation : gltfModel.animations) {
        AS::MeshAnimation animation {};
        animation.name = gltfAnimation.name;
        // Samplers
        for (auto const & gltfSampler : gltfAnimation.samplers)
        {
            Sampler sampler {};
            sampler.interpolation = convertInterpolationToEnum(gltfSampler.interpolation.c_str());

            {// Read sampler keyframe input time values
                float const * inputData = nullptr;
                uint32_t inputCount = 0;
                GLTF_extractDataFromBuffer(
                    gltfModel, 
                    gltfSampler.input, 
                    TINYGLTF_COMPONENT_TYPE_FLOAT,
                    inputData,
                    inputCount
                );
                MFA_ASSERT(inputCount > 0);

                sampler.inputAndOutput.resize(inputCount);
                for (size_t index = 0; index < inputCount; index++)
                {
                    auto const input = inputData[index];
                    sampler.inputAndOutput[index].input = input;
                    if (animation.startTime == -1.0f || animation.startTime > input) {
                        animation.startTime = input;
                    }
                    if (animation.endTime == -1.0f || animation.endTime < input) {
                        animation.endTime = input;
                    }
                }
            }

            {// Read sampler keyframe output translate/rotate/scale values
                void const * outputData = nullptr;
                uint32_t outputCount = 0;
                int outputDataType = 0;
                GLTF_extractDataAndTypeFromBuffer(
                    gltfModel, 
                    gltfSampler.output,
                    TINYGLTF_COMPONENT_TYPE_FLOAT,
                    outputDataType,
                    outputData,
                    outputCount
                );
                MFA_ASSERT(outputCount == sampler.inputAndOutput.size());
                
                struct Output3 {
                    float value[3];
                };

                struct Output4 {
                    float value[4];
                };

                switch (outputDataType)
                {
                    case TINYGLTF_TYPE_VEC3: {
                        auto const * output = static_cast<Output3 const *>(outputData);
                        for (size_t index = 0; index < outputCount; index++)
                        {
                            MFA::Copy<3>(sampler.inputAndOutput[index].output, output[index].value);
                        }
                        break;
                    }
                    case TINYGLTF_TYPE_VEC4: {
                        auto const * output = static_cast<Output4 const *>(outputData);
                        for (size_t index = 0; index < outputCount; index++)
                        {
                            MFA::Copy<4>(sampler.inputAndOutput[index].output, output[index].value);
                        }
                        break;
                    }
                    default: {
                        MFA_REQUIRE(false);
                        break;
                    }
                }
            }
            animation.samplers.emplace_back(sampler);
        }

        // Channels
        for (auto const & gltfChannel : gltfAnimation.channels)
        {
            Channel channel {};
            channel.path = convertPathToEnum(gltfChannel.target_path.c_str());
            channel.samplerIndex = gltfChannel.sampler;
            channel.nodeIndex = gltfChannel.target_node;
            animation.channels.emplace_back(channel);
        }

        outResultModel.mesh.InsertAnimation(animation);
    }
}

// Based on sasha willems solution and a comment in github
AS::Model ImportGLTF(char const * path) {
    MFA_ASSERT(path != nullptr);
    AS::Model resultModel {};
    if (path != nullptr) {
        namespace TG = tinygltf;
        TG::TinyGLTF loader {};
        std::string error;
        std::string warning;
        TG::Model gltfModel {};

        auto const extension = FS::ExtractExtensionFromPath(path);

        bool success = false;

        if (extension == ".gltf") {
            success = loader.LoadASCIIFromFile(
                    &gltfModel,
                    &error,
                    &warning,
                    path
            );
        } else if (extension == ".glb") {
            success = loader.LoadBinaryFromFile(
                    &gltfModel,
                    &error,
                    &warning,
                    path
            );
        } else {
            MFA_CRASH("ImportGLTF format is not support: %s", extension.c_str());
        }

        if (error.empty() == false) {
            MFA_LOG_ERROR("ImportGltf Error: %s", error.c_str());
        }
        if (warning.empty() == false) {
            MFA_LOG_WARN("ImportGltf Warning: %s", warning.c_str());
        }
        if(success) {
            // TODO Camera
            if(false == gltfModel.meshes.empty()) {
                std::vector<TextureRef> textureRefs {};
                // Textures
                GLTF_extractTextures(
                    path,
                    gltfModel,
                    textureRefs,
                    resultModel
                );
                {// Reading samplers values from materials
                    //auto const & sampler = model.samplers[base_color_gltf_texture.sampler];
                    //model_asset.textures[base_color_texture_index]  
                }
                // SubMeshes
                GLTF_extractSubMeshes(gltfModel, textureRefs, resultModel);
            }
            // Nodes
            GLTF_extractNodes(gltfModel, resultModel);
            // Fill skin
            GLTF_extractSkins(gltfModel, resultModel);
            // Animation
            GLTF_extractAnimations(gltfModel, resultModel);
            resultModel.mesh.FinalizeData();
            // Remove mesh buffers if invalid
            if(resultModel.mesh.IsValid() == false) {
                Blob vertexBuffer {};
                Blob indicesBuffer {};
                resultModel.mesh.RevokeBuffers(vertexBuffer, indicesBuffer);
                Memory::Free(vertexBuffer);
                Memory::Free(indicesBuffer);
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
    MFA_ASSERT(mesh->IsValid());
    if (mesh != nullptr && mesh->IsValid()) {
        Blob vertexBuffer {};
        Blob indexBuffer {};
        mesh->RevokeBuffers(vertexBuffer, indexBuffer);
        Memory::Free(vertexBuffer);
        Memory::Free(indexBuffer);
        success = true;
    }
    return success;
}

RawFile ReadRawFile(char const * path) {
    RawFile rawFile {};
    MFA_ASSERT(path != nullptr);
    if(path != nullptr) {
#if defined(__DESKTOP__) || defined(__IOS__)
        auto * file = FS::OpenFile(path, FS::Usage::Read);
        MFA_DEFER {FS::CloseFile(file);};
        if(FS::FileIsUsable(file)) {
            auto const file_size = FS::FileSize(file);
            // TODO Allocate using a memory pool system
            auto const memory_blob = Memory::Alloc(file_size);
            auto const read_bytes = FS::Read(file, memory_blob);
            // Means that reading is successful
            if(read_bytes == file_size) {
                rawFile.data = memory_blob;
            } else {
                Memory::Free(memory_blob);
            }
        }
#elif defined(__ANDROID__)
        auto * file = FS::Android_OpenAsset(path);
        MFA_DEFER {FS::Android_CloseAsset(file);};
        if (FS::Android_AssetIsUsable(file)) {
            auto const fileSize = FS::Android_AssetSize(file);
            auto const memoryBlob = Memory::Alloc(fileSize);
            auto const readBytes = FS::Android_ReadAsset(file, memoryBlob);
            // Means that reading is successful
            if(readBytes == fileSize) {
                rawFile.data = memoryBlob;
            } else {
                Memory::Free(memoryBlob);
            }
        }
#else
    #error Os not handled
#endif
    }
    return rawFile;
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
