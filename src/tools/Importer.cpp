#include "Importer.hpp"

#include "engine/BedrockMemory.hpp"
#include "engine/BedrockFileSystem.hpp"
#include "tools/ImageUtils.hpp"
#include "engine/BedrockAssert.hpp"
#include "engine/BedrockMath.hpp"
#include "engine/BedrockMatrix.hpp"
#include "engine/job_system/JobSystem.hpp"
#include "engine/asset_system/Asset_PBR_Mesh.hpp"
#include "engine/BedrockCommon.hpp"
#include "engine/BedrockPath.hpp"
#include "engine/asset_system/AssetTypes.hpp"
#include "engine/asset_system/AssetTexture.hpp"
#include "engine/asset_system/AssetBaseMesh.hpp"
#include "engine/asset_system/AssetModel.hpp"
#include "engine/asset_system/AssetShader.hpp"

#include "libs/tiny_obj_loader/tiny_obj_loader.h"
#include "libs/tiny_gltf_loader/tiny_gltf_loader.h"

#include <utility>

namespace MFA::Importer
{

    //-------------------------------------------------------------------------------------------------

    std::shared_ptr<AS::Texture> ImportUncompressedImage(
        std::string const & path,
        ImportTextureOptions const & options
    )
    {
        std::shared_ptr<AS::Texture> texture{};
        Utils::UncompressedTexture::Data imageData{};
        auto const use_srgb = options.preferSrgb;
        auto const load_image_result = Utils::UncompressedTexture::Load(
            imageData,
            path,
            use_srgb
        );
        if (load_image_result == Utils::UncompressedTexture::LoadResult::Success)
        {
            MFA_ASSERT(imageData.valid());
            auto const image_width = imageData.width;
            auto const image_height = imageData.height;
            auto const pixels = imageData.pixels;
            auto const components = imageData.components;
            auto const format = imageData.format;
            auto const depth = 1; // TODO We need to support depth
            auto const slices = 1;
            texture = ImportInMemoryTexture(
                path,
                pixels->memory,
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

    //-------------------------------------------------------------------------------------------------

    std::shared_ptr<AS::Texture> CreateErrorTexture()
    {
        auto const data = Memory::Alloc(4);
        auto * pixel = data->memory.as<uint8_t>();
        pixel[0] = 1;
        pixel[1] = 1;
        pixel[2] = 1;
        pixel[3] = 1;

        return ImportInMemoryTexture(
            "Error",
            data->memory,
            1,
            1,
            AS::TextureFormat::UNCOMPRESSED_UNORM_R8G8B8A8_LINEAR,
            4,
            1,
            1
        );
    }

    //-------------------------------------------------------------------------------------------------

    std::shared_ptr<AS::Texture> ImportInMemoryTexture(
        std::string const & nameOrAddress,
        CBlob const originalImagePixels,
        int32_t const width,
        int32_t const height,
        AS::TextureFormat const format,
        uint32_t const components,
        uint16_t const depth,
        uint16_t const slices,
        ImportTextureOptions const & options
    )
    {
        using namespace Utils::UncompressedTexture;

        auto const useSrgb = options.preferSrgb;            // TODO: Variable not used! Why!
        AS::Texture::Dimensions const originalImageDimension{
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

        std::string nameId = Path::RelativeToAssetFolder(nameOrAddress);

        std::shared_ptr<AS::Texture> texture = std::make_shared<AS::Texture>(nameId);

        texture->initForWrite(
            format,
            slices,
            depth,
            Memory::Alloc(bufferSize)
        );

        // Generating mipmaps (TODO : Code needs debugging)
        texture->addMipmap(originalImageDimension, originalImagePixels);

        for (uint8_t mipLevel = 1; mipLevel < mipCount; mipLevel++)
        {
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

            // Resize
            ResizeInputParams inputParams{
                .inputImagePixels = originalImagePixels,
                .inputImageWidth = static_cast<int>(originalImageDimension.width),
                .inputImageHeight = static_cast<int>(originalImageDimension.height),
                .componentsCount = components,
                .outputImagePixels = mipMapPixels->memory,
                .outputWidth = static_cast<int>(currentMipDims.width),
                .outputHeight = static_cast<int>(currentMipDims.height),
            };
            auto const resizeResult = Utils::UncompressedTexture::Resize(inputParams);
            MFA_ASSERT(resizeResult == true);

            texture->addMipmap(
                currentMipDims,
                mipMapPixels->memory
            );
        }

        MFA_ASSERT(texture->isValid());

        return texture;
    }

    //-------------------------------------------------------------------------------------------------

    std::shared_ptr<AS::Texture> ImportKTXImage(std::string const & path)
    {
        using namespace Utils::KTXTexture;

        std::string nameId = Path::RelativeToAssetFolder(path);

        auto texture = std::make_shared<AS::Texture>(nameId);

        LoadResult loadResult = LoadResult::Invalid;
        auto imageData = Load(loadResult, path);

        if (MFA_VERIFY(loadResult == LoadResult::Success && imageData != nullptr))
        {
            texture->initForWrite(
                imageData->format,
                imageData->sliceCount,
                imageData->depth,
                Memory::Alloc(imageData->totalImageSize)
            );

            auto width = imageData->width;
            auto height = imageData->height;
            auto depth = imageData->depth;

            for (auto i = 0u; i < imageData->mipmapCount; ++i)
            {
                MFA_ASSERT(width >= 1);
                MFA_ASSERT(height >= 1);
                MFA_ASSERT(depth >= 1);

                auto const mipBlob = GetMipBlob(imageData.get(), i);
                if (!MFA_VERIFY(mipBlob.ptr != nullptr && mipBlob.len > 0))
                {
                    return nullptr;
                }

                texture->addMipmap(
                    AS::Texture::Dimensions{
                        .width = static_cast<uint32_t>(width),
                        .height = static_cast<uint32_t>(height),
                        .depth = static_cast<uint16_t>(depth),
                    },
                    mipBlob
                    );

                width = Math::Max<uint16_t>(width / 2, 1);
                height = Math::Max<uint16_t>(height / 2, 1);
                depth = Math::Max<uint16_t>(depth / 2, 1);
            }
        }
        return texture;
    }

    //-------------------------------------------------------------------------------------------------

    AS::Texture ImportDDSFile(std::string const & path)
    {
        // TODO
        MFA_NOT_IMPLEMENTED_YET("Mohammad Fakhreddin");
    }

    //-------------------------------------------------------------------------------------------------

    std::shared_ptr<AS::Texture> ImportImage(std::string const & path, ImportTextureOptions const & options)
    {
        std::shared_ptr<AS::Texture> texture{};

        if (MFA_VERIFY(path.empty() == false))
        {
            auto const extension = Path::ExtractExtensionFromPath(path);

            if (extension == ".png" || extension == ".jpg" || extension == ".jpeg")
            {
                texture = ImportUncompressedImage(path, options);
            }
            else if (extension == ".ktx")
            {
                texture = ImportKTXImage(path);
            }
        }

        return texture;
    }

    //-------------------------------------------------------------------------------------------------

    std::shared_ptr<AS::Shader> ImportShaderFromHLSL(std::string const & path)
    {
        MFA_NOT_IMPLEMENTED_YET("Mohammad Fakhreddin");
    }

    //-------------------------------------------------------------------------------------------------

    std::shared_ptr<AS::Shader> ImportShaderFromSPV(
        std::string const & path,
        AS::ShaderStage const stage,
        std::string const & entryPoint
    )
    {
        std::shared_ptr<AS::Shader> shader = nullptr;
        if (MFA_VERIFY(path.empty() == false))
        {
#if defined(__DESKTOP__) || defined(__IOS__)
            auto file = FS::OpenFile(path, FS::Usage::Read);
            if (MFA_VERIFY(file != nullptr))
            {
                auto const fileSize = file->size();
                MFA_ASSERT(fileSize > 0);

                auto const buffer = Memory::Alloc(fileSize);

                auto const readBytes = file->read(buffer->memory);

                if (readBytes == buffer->memory.len)
                {
                    shader = std::make_shared<AS::Shader>(entryPoint, stage, buffer);
                }
            }
#elif defined(__ANDROID__)
            auto * file = FS::Android_OpenAsset(path);
            MFA_DEFER{ FS::Android_CloseAsset(file); };
            if (FS::Android_AssetIsUsable(file))
            {
                auto const fileSize = FS::Android_AssetSize(file);
                MFA_ASSERT(fileSize > 0);

                auto const buffer = Memory::Alloc(fileSize);
                auto const readBytes = FS::Android_ReadAsset(file, buffer->memory);

                if (readBytes == buffer->memory.len)
                {
                    shader = std::make_shared<AS::Shader>(entryPoint, stage, buffer);
                }
            }
#else
#error "Os not handled"
#endif
        }
        return shader;
    }

    //-------------------------------------------------------------------------------------------------

    std::shared_ptr<AS::Shader> ImportShaderFromSPV(
        CBlob const dataMemory,
        AS::ShaderStage const stage,
        std::string const & entryPoint
    )
    {
        MFA_ASSERT(dataMemory.ptr != nullptr);
        MFA_ASSERT(dataMemory.len > 0);
        std::shared_ptr<AS::Shader> shader = nullptr;
        if (dataMemory.ptr != nullptr && dataMemory.len > 0)
        {
            auto const buffer = Memory::Alloc(dataMemory.len);
            ::memcpy(buffer->memory.ptr, dataMemory.ptr, buffer->memory.len);
            shader = std::make_shared<AS::Shader>(entryPoint, stage, buffer);
        }
        return shader;
    }

    //-------------------------------------------------------------------------------------------------

    std::shared_ptr<AS::MeshBase> ImportObj(std::string const & path)
    {
        using namespace AS::PBR;

        std::shared_ptr<Mesh> mesh = nullptr;

        if (MFA_VERIFY(path.empty() == false))
        {
            if (FS::Exists(path))
            {
                mesh = std::make_shared<Mesh>();;

                auto file = FS::OpenFile(path, FS::Usage::Read);
                if (MFA_VERIFY(file != nullptr))
                {
                    bool is_counter_clockwise = false;
                    {//Check if normal vectors are reverse
                        // TODO: We could allocate from stack instead
                        auto firstLineBlob = Memory::Alloc(200);
                        file->read(firstLineBlob->memory);
                        std::string const firstLine = std::string(firstLineBlob->memory.as<char>());
                        if (firstLine.find("ccw") != std::string::npos)
                        {
                            is_counter_clockwise = true;            // TODO: Variable not used! Why?
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
                        path.c_str()
                    );
                    // TODO Handle materials
                    if (load_obj_result)
                    {
                        if (shapes.empty())
                        {
                            MFA_CRASH("Object has no shape");
                        }
                        if (0 != attributes.vertices.size() % 3)
                        {
                            MFA_CRASH("Vertices must be dividable by 3");
                        }
                        if (0 != attributes.texcoords.size() % 2)
                        {
                            MFA_CRASH("Attributes must be dividable by 3");
                        }
                        if (0 != shapes[0].mesh.indices.size() % 3)
                        {
                            MFA_CRASH("Indices must be dividable by 3");
                        }
                        auto positions_count = attributes.vertices.size() / 3;
                        auto coords_count = attributes.texcoords.size() / 2;
                        auto normalsCount = attributes.normals.size() / 3;
                        if (positions_count != coords_count)
                        {
                            MFA_CRASH("Vertices and texture coordinates must have same size");
                        }
                        // TODO I think normals have issues
                        struct Position
                        {
                            float value[3];
                        };

                        auto positionsBlob = Memory::Alloc(sizeof(Position) * positions_count);
                        auto * positions = positionsBlob->memory.as<Position>();
                        for (
                            uintmax_t vertexIndex = 0;
                            vertexIndex < positions_count;
                            vertexIndex++
                        )
                        {
                            positions[vertexIndex].value[0] = attributes.vertices[vertexIndex * 3 + 0];
                            positions[vertexIndex].value[1] = attributes.vertices[vertexIndex * 3 + 1];
                            positions[vertexIndex].value[2] = attributes.vertices[vertexIndex * 3 + 2];
                        }

                        struct TextureCoordinates
                        {
                            float value[2];
                        };
                        auto coordsBlob = Memory::Alloc(sizeof(TextureCoordinates) * coords_count);
                        auto * coords = coordsBlob->memory.as<TextureCoordinates>();
                        for (
                            uintmax_t texIndex = 0;
                            texIndex < coords_count;
                            texIndex++
                        )
                        {
                            coords[texIndex].value[0] = attributes.texcoords[texIndex * 2 + 0];
                            coords[texIndex].value[1] = attributes.texcoords[texIndex * 2 + 1];
                        }
                        struct Normals
                        {
                            float value[3];
                        };
                        auto normalsBlob = Memory::Alloc(sizeof(Normals) * normalsCount);
                        auto * normals = normalsBlob->memory.as<Normals>();
                        for (
                            uintmax_t normalIndex = 0;
                            normalIndex < normalsCount;
                            normalIndex++
                        )
                        {
                            normals[normalIndex].value[0] = attributes.normals[normalIndex * 3 + 0];
                            normals[normalIndex].value[1] = attributes.normals[normalIndex * 3 + 1];
                            normals[normalIndex].value[2] = attributes.normals[normalIndex * 3 + 2];
                        }
                        auto const vertexCount = static_cast<uint32_t>(positions_count);
                        auto const indexCount = static_cast<uint32_t>(shapes[0].mesh.indices.size());

                        mesh->initForWrite(
                            vertexCount,
                            indexCount,
                            Memory::Alloc(sizeof(Vertex) * vertexCount),
                            Memory::Alloc(sizeof(AS::Index) * indexCount)
                        );

                        auto const subMeshIndex = mesh->insertSubMesh();

                        std::vector<Vertex> vertices(vertexCount);
                        std::vector<AS::Index> indices(indexCount);
                        for (
                            uintmax_t indicesIndex = 0;
                            indicesIndex < shapes[0].mesh.indices.size();
                            indicesIndex++
                        )
                        {
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
                            Primitive primitive{};
                            primitive.uniqueId = 0;
                            primitive.vertexCount = vertexCount;
                            primitive.indicesCount = indexCount;
                            primitive.baseColorTextureIndex = 0;
                            primitive.hasNormalBuffer = true;

                            mesh->insertPrimitive(
                                subMeshIndex,
                                primitive,
                                static_cast<uint32_t>(vertices.size()),
                                vertices.data(),
                                static_cast<uint32_t>(indices.size()),
                                indices.data()
                            );
                        }

                        Node node{};
                        node.subMeshIndex = static_cast<int>(subMeshIndex);
                        Copy<16>(node.transform, glm::identity<glm::mat4>());
                        mesh->insertNode(node);
                        MFA_ASSERT(mesh->isValid());

                    }
                    else if (!error.empty() && error.substr(0, 4) != "WARN")
                    {
                        MFA_CRASH("LoadObj returned error: %s, File: %s", error.c_str(), path.c_str());
                    }
                    else
                    {
                        MFA_CRASH("LoadObj failed");
                    }
                }
            }
        }
        return mesh;
    }

    //-------------------------------------------------------------------------------------------------

    template<typename ItemType>
    static void GLTF_extractDataFromBuffer(
        tinygltf::Model & gltfModel,
        int const accessorIndex,
        int const expectedComponentType,
        ItemType const *& outData,
        uint32_t & outDataCount
    )
    {
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
        tinygltf::Model const & gltfModel,
        int accessorIndex,
        int expectedComponentType,
        int & outType,
        void const *& outData,
        uint32_t & outDataCount
    )
    {
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

    //-------------------------------------------------------------------------------------------------

    static bool GLTF_extractPrimitiveDataAndTypeFromBuffer(
        tinygltf::Model const & gltfModel,
        tinygltf::Primitive & primitive,
        char const * fieldKey,
        int & outType,
        int & outComponentType,
        void const *& outData,
        uint32_t & outDataCount
    )
    {
        bool success = false;
        auto const findAttributeResult = primitive.attributes.find(fieldKey);
        if (findAttributeResult != primitive.attributes.end())
        {// Positions
            success = true;
            auto const attributeValue = findAttributeResult->second;
            MFA_REQUIRE(static_cast<size_t>(attributeValue) < gltfModel.accessors.size());
            auto const & accessor = gltfModel.accessors[attributeValue];
            outType = accessor.type;
            outComponentType = accessor.componentType;
            auto const & bufferView = gltfModel.bufferViews[accessor.bufferView];
            MFA_REQUIRE(bufferView.buffer < gltfModel.buffers.size());
            auto const & buffer = gltfModel.buffers[bufferView.buffer];
            outData = reinterpret_cast<void const *>(
                &buffer.data[bufferView.byteOffset + accessor.byteOffset]
            );
            outDataCount = static_cast<uint32_t>(accessor.count);
        }
        return success;
    }

    //-------------------------------------------------------------------------------------------------

    template<typename ItemType>
    static bool GLTF_extractPrimitiveDataFromBuffer(
        tinygltf::Model & gltfModel,
        tinygltf::Primitive & primitive,
        char const * fieldKey,
        size_t const expectedComponentCount,
        int expectedComponentType,
        ItemType const *& outData,
        uint32_t & outDataCount,
        bool & outHasMinMax,
        ItemType * outMin,
        ItemType * outMax
    )
    {
        bool success = false;
        outHasMinMax = false;
        if (primitive.attributes.find(fieldKey) != primitive.attributes.end())
        {// Positions
            success = true;
            MFA_REQUIRE(static_cast<size_t>(primitive.attributes[fieldKey]) < gltfModel.accessors.size());
            auto const & accessor = gltfModel.accessors[primitive.attributes[fieldKey]];
            MFA_ASSERT(accessor.componentType == expectedComponentType);
            if (
                outMin != nullptr && outMax != nullptr &&
                accessor.minValues.size() == expectedComponentCount &&
                accessor.maxValues.size() == expectedComponentCount
            )
            {
                for (size_t i = 0; i < expectedComponentCount; ++i)
                {
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

    //-------------------------------------------------------------------------------------------------

    template<typename ItemType>
    static bool GLTF_extractPrimitiveDataFromBuffer(
        tinygltf::Model & gltfModel,
        tinygltf::Primitive & primitive,
        char const * fieldKey,
        size_t const expectedComponentCount,
        int const expectedComponentType,
        ItemType const *& outData,
        uint32_t & outDataCount
    )
    {
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

    //-------------------------------------------------------------------------------------------------

    struct TextureRef
    {
        std::string const gltfName {};
        uint8_t const index = 0;
        std::string const relativePath {};
    };

    static void GLTF_extractTextures(
        std::string const & path,
        tinygltf::Model const & gltfModel,
        std::vector<TextureRef> & outTextureRefs,
        std::vector<AS::SamplerConfig> & outSamplers
    )
    {
        std::string directoryPath = Path::ExtractDirectoryFromPath(path);

        directoryPath = Path::RelativeToAssetFolder(directoryPath);

        // Extracting textures
        if (false == gltfModel.textures.empty())
        {
            for (auto const & texture : gltfModel.textures)
            {
                //sampler.isValid = false;
                if (texture.sampler >= 0)
                {// Sampler
                    auto const & gltfSampler = gltfModel.samplers[texture.sampler];
                    //sampler.sample_mode = gltf_sampler. // TODO
                    //sampler.isValid = true;
                    outSamplers.emplace_back(AS::SamplerConfig {
                        .isValid = true,
                        .magFilter = gltfSampler.magFilter,
                        .minFilter = gltfSampler.minFilter,
                        .wrapS = gltfSampler.wrapS,
                        .wrapT = gltfSampler.wrapT

                    });
                } else
                {
                    outSamplers.emplace_back(AS::SamplerConfig {
                        .isValid = false
                    });
                }
                
                auto const & image = gltfModel.images[texture.source];
                
                std::string const imagePath = directoryPath + "/" + image.uri;
                
                TextureRef textureRef {
                    .gltfName = image.uri,
                    .index = static_cast<uint8_t>(outTextureRefs.size()),
                    .relativePath = imagePath
                };
                outTextureRefs.emplace_back(textureRef);
            }

            //JS::WaitForThreadsToFinish();

        }

    }

    //-------------------------------------------------------------------------------------------------

    static int16_t GLTF_findTextureByName(
        char const * textureName,
        std::vector<TextureRef> const & textureRefs
    )
    {
        MFA_ASSERT(textureName != nullptr);
        if (false == textureRefs.empty())
        {
            for (auto const & textureRef : textureRefs)
            {
                if (textureRef.gltfName == textureName)
                {
                    return textureRef.index;
                }
            }
        }
        return -1;
        //MFA_CRASH("Image not found: %s", gltf_name);
    }


    //-------------------------------------------------------------------------------------------------

#define extractTextureAndUV_Index(gltfModel, textureInfo, textureRefs, outTextureIndex, outUV_Index)    \
    if (textureInfo.index >= 0)                                                                             \
    {                                                                                                       \
        auto const & emissive_texture = gltfModel.textures[textureInfo.index];                              \
        auto const & image = gltfModel.images[emissive_texture.source];                                     \
        outTextureIndex = GLTF_findTextureByName(image.uri.c_str(), textureRefs);                           \
        if (outTextureIndex >= 0)                                                                           \
        {                                                                                                   \
            outUV_Index = static_cast<uint16_t>(textureInfo.texCoord);                                      \
        }                                                                                                   \
    }

    //-------------------------------------------------------------------------------------------------

    static void copyDataIntoVertexMember(
        float * vertexMember,
        uint8_t const componentCount,
        float const * items,
        uint32_t const dataIndex,
        bool const hasMinMax,
        float const * minValue,
        float const * maxValue
    )
    {
        Copy(vertexMember, &items[dataIndex * componentCount], componentCount);
#ifdef MFA_DEBUG
        if (hasMinMax)
        {
            for (int i = 0; i < componentCount; ++i)
            {
                MFA_ASSERT(vertexMember[i] >= minValue[i]);
                MFA_ASSERT(vertexMember[i] <= maxValue[i]);
            }
        }
#endif
    }

    //-------------------------------------------------------------------------------------------------

    static std::shared_ptr<AS::PBR::Mesh> GLTF_extractSubMeshes(
        tinygltf::Model & gltfModel,
        std::vector<TextureRef> const & textureRefs
    )
    {
        using namespace AS::PBR;

        auto const generateUvKeyword = [](int32_t const uvIndex) -> std::string
        {
            return "TEXCOORD_" + std::to_string(uvIndex);
        };
        // Step1: Iterate over all meshes and gather required information for asset buffer
        uint32_t totalIndicesCount = 0;
        uint32_t totalVerticesCount = 0;
        uint32_t indicesVertexStartingIndex = 0;
        for (auto & mesh : gltfModel.meshes)
        {
            if (false == mesh.primitives.empty())
            {
                for (auto & primitive : mesh.primitives)
                {
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

        auto mesh = std::make_shared<Mesh>();
        mesh->initForWrite(
            totalVerticesCount,
            totalIndicesCount,
            Memory::Alloc(sizeof(Vertex) * totalVerticesCount),
            Memory::Alloc(sizeof(AS::Index) * totalIndicesCount)
        );
        // Step2: Fill subMeshes
        uint32_t primitiveUniqueId = 0;
        std::vector<Vertex> primitiveVertices{};
        std::vector<AS::Index> primitiveIndices{};
        for (auto & gltfMesh : gltfModel.meshes)
        {
            auto const meshIndex = mesh->insertSubMesh();
            if (false == gltfMesh.primitives.empty())
            {
                for (auto & gltfPrimitive : gltfMesh.primitives)
                {
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
                    int16_t occlusionTextureIndex = -1;
                    int32_t occlusionUV_Index = -1;
                    float baseColorFactor[4]{};
                    float metallicFactor = 0;
                    float roughnessFactor = 0;
                    float emissiveFactor[3]{};
                    bool doubleSided = false;
                    float alphaCutoff = 0.0f;

                    using AlphaMode = AS::AlphaMode;
                    AlphaMode alphaMode = AlphaMode::Opaque;

                    uint32_t uniqueId = primitiveUniqueId;
                    primitiveUniqueId++;
                    if (gltfPrimitive.material >= 0)
                    {// Material
                        auto const & material = gltfModel.materials[gltfPrimitive.material];

                        // Base color texture
                        extractTextureAndUV_Index(
                            gltfModel,
                            material.pbrMetallicRoughness.baseColorTexture,
                            textureRefs,
                            baseColorTextureIndex,
                            baseColorUvIndex
                        );

                        // Metallic-roughness texture
                        extractTextureAndUV_Index(
                            gltfModel,
                            material.pbrMetallicRoughness.metallicRoughnessTexture,
                            textureRefs,
                            metallicRoughnessTextureIndex,
                            metallicRoughnessUvIndex
                        );

                        // Normal texture
                        extractTextureAndUV_Index(
                            gltfModel,
                            material.normalTexture,
                            textureRefs,
                            normalTextureIndex,
                            normalUvIndex
                        )

                            // Emissive texture
                            extractTextureAndUV_Index(
                                gltfModel,
                                material.emissiveTexture,
                                textureRefs,
                                emissiveTextureIndex,
                                emissiveUvIndex
                            )

                            // Occlusion texture
                            extractTextureAndUV_Index(
                                gltfModel,
                                material.occlusionTexture,
                                textureRefs,
                                occlusionTextureIndex,
                                occlusionUV_Index
                            )

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

                        alphaCutoff = static_cast<float>(material.alphaCutoff);
                        alphaMode = [&material]()->AlphaMode
                        {
                            if (material.alphaMode == "OPAQUE")
                            {
                                return AlphaMode::Opaque;
                            }
                            if (material.alphaMode == "BLEND")
                            {
                                return AlphaMode::Blend;
                            }
                            if (material.alphaMode == "MASK")
                            {
                                return AlphaMode::Mask;
                            }
                            MFA_LOG_ERROR("Unhandled format detected: %s", material.alphaMode.c_str());
                        }();
                        doubleSided = material.doubleSided;
                    }
                    uint32_t primitiveIndicesCount = 0;
                    {// Indices
                        MFA_REQUIRE(gltfPrimitive.indices < gltfModel.accessors.size());
                        auto const & accessor = gltfModel.accessors[gltfPrimitive.indices];
                        auto const & bufferView = gltfModel.bufferViews[accessor.bufferView];
                        MFA_REQUIRE(bufferView.buffer < gltfModel.buffers.size());
                        auto const & buffer = gltfModel.buffers[bufferView.buffer];
                        primitiveIndicesCount = static_cast<uint32_t>(accessor.count);

                        switch (accessor.componentType)
                        {
                        case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT:
                        {
                            auto const * gltfIndices = reinterpret_cast<uint32_t const *>(
                                &buffer.data[bufferView.byteOffset + accessor.byteOffset]
                            );
                            for (uint32_t i = 0; i < primitiveIndicesCount; i++)
                            {
                                primitiveIndices.emplace_back(gltfIndices[i] + indicesVertexStartingIndex);
                            }
                        }
                        break;
                        case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT:
                        {
                            auto const * gltfIndices = reinterpret_cast<uint16_t const *>(
                                &buffer.data[bufferView.byteOffset + accessor.byteOffset]
                            );
                            for (uint32_t i = 0; i < primitiveIndicesCount; i++)
                            {
                                primitiveIndices.emplace_back(gltfIndices[i] + indicesVertexStartingIndex);
                            }
                        }
                        break;
                        case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE:
                        {
                            auto const * gltfIndices = reinterpret_cast<uint8_t const *>(
                                &buffer.data[bufferView.byteOffset + accessor.byteOffset]
                            );
                            for (uint32_t i = 0; i < primitiveIndicesCount; i++)
                            {
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
                    float positionsMinValue[3]{};
                    float positionsMaxValue[3]{};
                    bool hasPositionMinMax = false;
                    {// Position
                        auto const result = GLTF_extractPrimitiveDataFromBuffer(
                            gltfModel,
                            gltfPrimitive,
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

                    float const * baseColorUVs = nullptr;
                    float baseColorUV_Min[2]{};
                    float baseColorUV_Max[2]{};
                    bool hasBaseColorUvMinMax = false;
                    if (baseColorUvIndex >= 0)
                    {// BaseColor
                        uint32_t baseColorUvsCount = 0;
                        auto texture_coordinate_key_name = generateUvKeyword(baseColorUvIndex);
                        auto const result = GLTF_extractPrimitiveDataFromBuffer(
                            gltfModel,
                            gltfPrimitive,
                            texture_coordinate_key_name.c_str(),
                            2,
                            TINYGLTF_COMPONENT_TYPE_FLOAT,
                            baseColorUVs,
                            baseColorUvsCount,
                            hasBaseColorUvMinMax,
                            baseColorUV_Min,
                            baseColorUV_Max
                        );
                        MFA_ASSERT(result == true);
                        MFA_ASSERT(baseColorUvsCount == primitiveVertexCount);
                    }

                    float const * metallicRoughnessUvs = nullptr;
                    float metallicRoughnessUVMin[2]{};
                    float metallicRoughnessUVMax[2]{};
                    bool hasMetallicRoughnessUvMinMax = false;
                    if (metallicRoughnessUvIndex >= 0)
                    {// MetallicRoughness uvs
                        std::string texture_coordinate_key_name = generateUvKeyword(metallicRoughnessUvIndex);
                        uint32_t metallicRoughnessUvsCount = 0;
                        auto const result = GLTF_extractPrimitiveDataFromBuffer(
                            gltfModel,
                            gltfPrimitive,
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

                    float const * emissionUVs = nullptr;
                    float emissionUV_Min[2]{};
                    float emissionUV_Max[2]{};
                    bool hasEmissionUvMinMax = false;
                    if (emissiveUvIndex >= 0)
                    {// Emission uvs
                        std::string textureCoordinateKeyName = generateUvKeyword(emissiveUvIndex);
                        uint32_t emissionUvCount = 0;
                        auto const result = GLTF_extractPrimitiveDataFromBuffer(
                            gltfModel,
                            gltfPrimitive,
                            textureCoordinateKeyName.c_str(),
                            2,
                            TINYGLTF_COMPONENT_TYPE_FLOAT,
                            emissionUVs,
                            emissionUvCount,
                            hasEmissionUvMinMax,
                            emissionUV_Min,
                            emissionUV_Max
                        );
                        MFA_ASSERT(result == true);
                        MFA_ASSERT(emissionUvCount == primitiveVertexCount);
                    }

                    float const * occlusionUVs = nullptr;
                    float occlusionUV_Min[2]{};
                    float occlusionUV_Max[2]{};
                    bool hasOcclusionUV_MinMax = false;
                    if (occlusionUV_Index >= 0)
                    {// Occlusion uvs
                        std::string textureCoordinateKeyName = generateUvKeyword(occlusionUV_Index);
                        uint32_t occlusionUV_Count = 0;
                        auto const result = GLTF_extractPrimitiveDataFromBuffer(
                            gltfModel,
                            gltfPrimitive,
                            textureCoordinateKeyName.c_str(),
                            2,
                            TINYGLTF_COMPONENT_TYPE_FLOAT,
                            occlusionUVs,
                            occlusionUV_Count,
                            hasOcclusionUV_MinMax,
                            occlusionUV_Min,
                            occlusionUV_Max
                        );
                        MFA_ASSERT(result == true);
                        MFA_ASSERT(occlusionUV_Count == primitiveVertexCount);
                    }

                    float const * normalsUVs = nullptr;
                    float normalsUV_Min[2]{};
                    float normalsUV_Max[2]{};
                    bool hasNormalUvMinMax = false;
                    if (normalUvIndex >= 0)
                    {// Normal uvs
                        std::string texture_coordinate_key_name = generateUvKeyword(normalUvIndex);
                        uint32_t normalUvsCount = 0;
                        auto const result = GLTF_extractPrimitiveDataFromBuffer(
                            gltfModel,
                            gltfPrimitive,
                            texture_coordinate_key_name.c_str(),
                            2,
                            TINYGLTF_COMPONENT_TYPE_FLOAT,
                            normalsUVs,
                            normalUvsCount,
                            hasNormalUvMinMax,
                            normalsUV_Min,
                            normalsUV_Max
                        );
                        MFA_ASSERT(result == true);
                        MFA_ASSERT(normalUvsCount == primitiveVertexCount);
                    }
                    float const * normalValues = nullptr;
                    float normalsValuesMin[3]{};
                    float normalsValuesMax[3]{};
                    bool hasNormalValueMinMax = false;
                    {// Normal values
                        uint32_t normalValuesCount = 0;
                        auto const result = GLTF_extractPrimitiveDataFromBuffer(
                            gltfModel,
                            gltfPrimitive,
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
                    float tangentsValuesMin[4]{};
                    float tangentsValuesMax[4]{};
                    bool hasTangentsValuesMinMax = false;
                    {// Tangent values
                        uint32_t tangentValuesCount = 0;
                        auto const result = GLTF_extractPrimitiveDataFromBuffer(
                            gltfModel,
                            gltfPrimitive,
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


                    uint32_t jointItemCount = 0;
                    std::vector<uint16_t> jointValues{};
                    int jointAccessorType = 0;
                    {// Joints
                        void const * rawJointValues = nullptr;
                        int componentType = 0;
                        uint32_t jointValuesCount = 0;
                        GLTF_extractPrimitiveDataAndTypeFromBuffer(
                            gltfModel,
                            gltfPrimitive,
                            "JOINTS_0",
                            jointAccessorType,
                            componentType,
                            rawJointValues,
                            jointValuesCount
                        );
                        jointItemCount = jointValuesCount * jointAccessorType;
                        jointValues.resize(jointItemCount);
                        switch (componentType)
                        {
                        case 0:
                            break;
                        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
                        {
                            auto const * shortJointValues = static_cast<uint16_t const *>(rawJointValues);
                            for (uint32_t i = 0; i < jointItemCount; ++i)
                            {
                                jointValues[i] = shortJointValues[i];
                            }
                        }
                        break;
                        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
                        {
                            auto const * byteJointValues = static_cast<uint8_t const *>(rawJointValues);
                            for (uint32_t i = 0; i < jointItemCount; ++i)
                            {
                                jointValues[i] = byteJointValues[i];
                            }
                        }
                        break;
                        default:
                            MFA_CRASH("Unhandled type");
                        }
                        MFA_ASSERT((jointAccessorType > 0 || jointItemCount == 0));
                    }
                    std::vector<float> weightValues{};
                    {// Weights
                        int componentType = 0;
                        int accessorType = 0;
                        uint32_t rawValuesCount = 0;
                        void const * rawValues = nullptr;

                        GLTF_extractPrimitiveDataAndTypeFromBuffer(
                            gltfModel,
                            gltfPrimitive,
                            "WEIGHTS_0",
                            accessorType,
                            componentType,
                            rawValues,
                            rawValuesCount
                        );

                        auto itemCount = rawValuesCount * accessorType;

                        MFA_ASSERT(itemCount == jointItemCount);
                        MFA_ASSERT(accessorType == jointAccessorType);

                        weightValues.resize(itemCount);
                        switch (componentType)
                        {
                        case 0:
                            break;
                        case TINYGLTF_COMPONENT_TYPE_FLOAT:
                        {
                            auto const * floatWeightValues = static_cast<float const *>(rawValues);
                            for (uint32_t i = 0; i < jointItemCount; ++i)
                            {
                                weightValues[i] = floatWeightValues[i];
                            }
                        }
                        break;
                        default:
                            MFA_CRASH("Unhandled type");
                        }
                        MFA_ASSERT((accessorType > 0 || itemCount == 0));
                    }
                    // TODO Start from here, Assign weight and joint
                    float const * colors = nullptr;
                    float colorsMinValue[3]{ 0 };
                    float colorsMaxValue[3]{ 1 };
                    float colorsMinMaxDiff[3]{ 1 };
                    if (gltfPrimitive.attributes["COLOR"] >= 0)
                    {
                        MFA_REQUIRE(gltfPrimitive.attributes["COLOR"] < gltfModel.accessors.size());
                        auto const & accessor = gltfModel.accessors[gltfPrimitive.attributes["COLOR"]];
                        //MFA_ASSERT(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);
                        //TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT
                        if (accessor.minValues.size() == 3 && accessor.maxValues.size() == 3)
                        {
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
                        colors = reinterpret_cast<const float *>(                           // TODO: Variable not used! Why?
                            &buffer.data[bufferView.byteOffset + accessor.byteOffset]
                        );
                    }

                    bool hasPosition = positions != nullptr;
                    MFA_ASSERT(hasPosition == true);
                    bool hasBaseColorTexture = baseColorUVs != nullptr;
                    MFA_ASSERT(baseColorUVs != nullptr == baseColorTextureIndex >= 0);
                    bool hasNormalValue = normalValues != nullptr;
                    MFA_ASSERT(hasNormalValue == true);
                    bool hasNormalTexture = normalsUVs != nullptr;
                    MFA_ASSERT(normalsUVs != nullptr == normalTextureIndex >= 0);
                    bool hasCombinedMetallicRoughness = metallicRoughnessUvs != nullptr;
                    MFA_ASSERT(metallicRoughnessUvs != nullptr == metallicRoughnessTextureIndex >= 0);
                    bool hasEmissiveTexture = emissionUVs != nullptr;
                    MFA_ASSERT(emissionUVs != nullptr == emissiveTextureIndex >= 0);
                    bool hasOcclusionTexture = occlusionUVs != nullptr;
                    MFA_ASSERT((occlusionUVs != nullptr) == (occlusionTextureIndex >= 0));
                    bool hasTangentValue = tangentValues != nullptr;
                    bool hasSkin = jointItemCount > 0;
                    for (uint32_t i = 0; i < primitiveVertexCount; ++i)
                    {
                        primitiveVertices.emplace_back();
                        auto & vertex = primitiveVertices.back();

                        // Positions
                        if (hasPosition)
                        {
                            copyDataIntoVertexMember(
                                vertex.position,
                                3,
                                positions,
                                i,
                                hasPositionMinMax,
                                positionsMinValue,
                                positionsMaxValue
                            );
                        }

                        // Normal values
                        if (hasNormalValue)
                        {
                            copyDataIntoVertexMember(
                                vertex.normalValue,
                                3,
                                normalValues,
                                i,
                                hasNormalValueMinMax,
                                normalsValuesMin,
                                normalsValuesMax
                            );
                        }

                        // Normal uvs
                        if (hasNormalTexture)
                        {
                            copyDataIntoVertexMember(
                                vertex.normalMapUV,
                                2,
                                normalsUVs,
                                i,
                                hasNormalUvMinMax,
                                normalsUV_Min,
                                normalsUV_Max
                            );
                        }

                        if (hasTangentValue)
                        {// Tangent
                            copyDataIntoVertexMember(
                                vertex.tangentValue,
                                4,
                                tangentValues,
                                i,
                                hasTangentsValuesMinMax,
                                tangentsValuesMin,
                                tangentsValuesMax
                            );
                        }

                        if (hasEmissiveTexture)
                        {// Emissive
                            copyDataIntoVertexMember(
                                vertex.emissionUV,
                                2,
                                emissionUVs,
                                i,
                                hasEmissionUvMinMax,
                                emissionUV_Min,
                                emissionUV_Max
                            );
                        }

                        if (hasBaseColorTexture)
                        {// BaseColor
                            copyDataIntoVertexMember(
                                vertex.baseColorUV,
                                2,
                                baseColorUVs,
                                i,
                                hasEmissionUvMinMax,
                                baseColorUV_Min,
                                baseColorUV_Max
                            );
                        }

                        if (hasOcclusionTexture)
                        {// Occlusion
                            copyDataIntoVertexMember(
                                vertex.occlusionUV,
                                2,
                                occlusionUVs,
                                i,
                                hasOcclusionUV_MinMax,
                                occlusionUV_Min,
                                occlusionUV_Max
                            );
                        }

                        if (hasCombinedMetallicRoughness)
                        {// MetallicRoughness
                            vertex.roughnessUV[0] = metallicRoughnessUvs[i * 2 + 0];
                            vertex.roughnessUV[1] = metallicRoughnessUvs[i * 2 + 1];
                            ::memcpy(vertex.metallicUV, vertex.roughnessUV, sizeof(vertex.roughnessUV));
                            static_assert(sizeof(vertex.roughnessUV) == sizeof(vertex.metallicUV));
                            if (hasMetallicRoughnessUvMinMax)
                            {
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
                        if (hasSkin)
                        {
                            for (int j = 0; j < jointAccessorType; j++)
                            {
                                vertex.jointIndices[j] = jointValues[i * jointAccessorType + j];
                                vertex.jointWeights[j] = weightValues[i * jointAccessorType + j];
                            }
                            for (int j = jointAccessorType; j < 4; j++)
                            {
                                vertex.jointIndices[j] = 0;
                                vertex.jointWeights[j] = 0;
                            }
                        }
                    }

                    {// Creating new subMesh
                        Primitive primitive{};
                        primitive.uniqueId = uniqueId;
                        primitive.baseColorTextureIndex = baseColorTextureIndex;
                        primitive.metallicRoughnessTextureIndex = metallicRoughnessTextureIndex;
                        primitive.normalTextureIndex = normalTextureIndex;
                        primitive.emissiveTextureIndex = emissiveTextureIndex;
                        primitive.occlusionTextureIndex = occlusionTextureIndex;
                        Copy<4>(primitive.baseColorFactor, baseColorFactor);
                        primitive.metallicFactor = metallicFactor;
                        primitive.roughnessFactor = roughnessFactor;
                        Copy<3>(primitive.emissiveFactor, emissiveFactor);
                        //primitive.occlusionStrengthFactor = occlusion
                        primitive.hasBaseColorTexture = hasBaseColorTexture;
                        primitive.hasEmissiveTexture = hasEmissiveTexture;
                        primitive.hasMetallicRoughnessTexture = hasCombinedMetallicRoughness;
                        primitive.hasNormalBuffer = hasNormalValue;
                        primitive.hasNormalTexture = hasNormalTexture;
                        primitive.hasTangentBuffer = hasTangentValue;
                        primitive.hasSkin = hasSkin;
                        primitive.hasPositionMinMax = hasPositionMinMax;
                        Copy<3>(primitive.positionMin, positionsMinValue);
                        Copy<3>(primitive.positionMax, positionsMaxValue);

                        primitive.alphaMode = alphaMode;
                        primitive.alphaCutoff = alphaCutoff;
                        primitive.doubleSided = doubleSided;

                        mesh->insertPrimitive(
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
        return mesh;
    }

    //-------------------------------------------------------------------------------------------------

    static void GLTF_extractNodes(
        tinygltf::Model const & gltfModel,
        AS::PBR::Mesh * mesh
    )
    {
        using namespace AS::PBR;
        // Step3: Fill nodes
        if (false == gltfModel.nodes.empty())
        {
            MFA_ASSERT(mesh != nullptr);
            for (auto const & gltfNode : gltfModel.nodes)
            {
                Node node{};
                node.subMeshIndex = gltfNode.mesh;
                node.children = gltfNode.children;
                node.skin = gltfNode.skin;

                if (gltfNode.translation.empty() == false)
                {
                    MFA_ASSERT(gltfNode.translation.size() == 3);
                    node.translate[0] = static_cast<float>(gltfNode.translation[0]);
                    node.translate[1] = static_cast<float>(gltfNode.translation[1]);
                    node.translate[2] = static_cast<float>(gltfNode.translation[2]);
                }
                if (gltfNode.rotation.empty() == false)
                {
                    MFA_ASSERT(gltfNode.rotation.size() == 4);
                    node.rotation[0] = static_cast<float>(gltfNode.rotation[0]);
                    node.rotation[1] = static_cast<float>(gltfNode.rotation[1]);
                    node.rotation[2] = static_cast<float>(gltfNode.rotation[2]);
                    node.rotation[3] = static_cast<float>(gltfNode.rotation[3]);
                }
                if (gltfNode.scale.empty() == false)
                {
                    MFA_ASSERT(gltfNode.scale.size() == 3);
                    node.scale[0] = static_cast<float>(gltfNode.scale[0]);
                    node.scale[1] = static_cast<float>(gltfNode.scale[1]);
                    node.scale[2] = static_cast<float>(gltfNode.scale[2]);
                }
                if (gltfNode.matrix.empty() == false)
                {
                    for (int i = 0; i < 16; ++i)
                    {
                        node.transform[i] = static_cast<float>(gltfNode.matrix[i]);
                    }
                }
                mesh->insertNode(node);
            }
        }
    }

    //-------------------------------------------------------------------------------------------------

    static void GLTF_extractSkins(
        tinygltf::Model & gltfModel,
        AS::PBR::Mesh * mesh
    )
    {
        using namespace AS::PBR;

        MFA_ASSERT(mesh != nullptr);

        for (auto const & gltfSkin : gltfModel.skins)
        {
            Skin skin{};

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
            for (size_t i = 0; i < skin.inverseBindMatrices.size(); ++i)
            {
                auto & currentMatrix = skin.inverseBindMatrices[i];
                Copy<16>(currentMatrix, &inverseBindMatricesPtr[i * 16]);
                //Matrix::CopyCellsToGlm(&inverseBindMatricesPtr[i * 16], currentMatrix);
            }
            MFA_ASSERT(skin.inverseBindMatrices.size() == skin.joints.size());
            skin.skeletonRootNode = gltfSkin.skeleton;

            mesh->insertSkin(skin);
        }
    }

    //-------------------------------------------------------------------------------------------------

    static void GLTF_extractAnimations(
        tinygltf::Model & gltfModel,
        AS::PBR::Mesh * mesh
    )
    {
        using namespace AS::PBR;

        using Sampler = Animation::Sampler;
        using Channel = Animation::Channel;
        using Interpolation = Animation::Interpolation;
        using Path = Animation::Path;

        MFA_ASSERT(mesh != nullptr);

        auto const convertInterpolationToEnum = [](char const * value)-> Interpolation
        {
            if (strcmp("LINEAR", value) == 0)
            {
                return Interpolation::Linear;
            }
            if (strcmp("STEP", value) == 0)
            {
                return Interpolation::Step;
            }
            if (strcmp("CUBICSPLINE", value) == 0)
            {
                return Interpolation::CubicSpline;
            }
            MFA_CRASH("Unhandled test case");
            return Interpolation::Invalid;
        };

        auto const convertPathToEnum = [](char const * value)-> Path
        {
            if (strcmp("translation", value) == 0)
            {
                return Path::Translation;
            }
            if (strcmp("rotation", value) == 0)
            {
                return Path::Rotation;
            }
            if (strcmp("scale", value) == 0)
            {
                return Path::Scale;
            }
            MFA_CRASH("Unhandled test case");
            return Path::Invalid;
        };

        for (auto const & gltfAnimation : gltfModel.animations)
        {
            Animation animation{};
            animation.name = gltfAnimation.name;
            // Samplers
            for (auto const & gltfSampler : gltfAnimation.samplers)
            {
                Sampler sampler{};
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
                        if (animation.startTime == -1.0f || animation.startTime > input)
                        {
                            animation.startTime = input;
                        }
                        if (animation.endTime == -1.0f || animation.endTime < input)
                        {
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

                    struct Output3
                    {
                        float value[3];
                    };

                    struct Output4
                    {
                        float value[4];
                    };

                    switch (outputDataType)
                    {
                    case TINYGLTF_TYPE_VEC3:
                    {
                        auto const * output = static_cast<Output3 const *>(outputData);
                        for (size_t index = 0; index < outputCount; index++)
                        {
                            MFA::Copy<3>(sampler.inputAndOutput[index].output, output[index].value);
                        }
                        break;
                    }
                    case TINYGLTF_TYPE_VEC4:
                    {
                        auto const * output = static_cast<Output4 const *>(outputData);
                        for (size_t index = 0; index < outputCount; index++)
                        {
                            MFA::Copy<4>(sampler.inputAndOutput[index].output, output[index].value);
                        }
                        break;
                    }
                    default:
                    {
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
                Channel channel{};
                channel.path = convertPathToEnum(gltfChannel.target_path.c_str());
                channel.samplerIndex = gltfChannel.sampler;
                channel.nodeIndex = gltfChannel.target_node;
                animation.channels.emplace_back(channel);
            }

            mesh->insertAnimation(animation);
        }
    }

    //-------------------------------------------------------------------------------------------------

    // Based on sasha willems solution and a comment in github
    std::shared_ptr<AS::Model> ImportGLTF(std::string const & path)
    {
        using namespace AS::PBR;

        std::shared_ptr<AS::Model> result = nullptr;
        if (MFA_VERIFY(path.empty() == false))
        {
            namespace TG = tinygltf;
            TG::TinyGLTF loader{};
            std::string error;
            std::string warning;
            TG::Model gltfModel{};

            auto const extension = Path::ExtractExtensionFromPath(path);

            bool success = false;

            if (extension == ".gltf")
            {
                success = loader.LoadASCIIFromFile(
                        &gltfModel,
                        &error,
                        &warning,
                        path
                );
            }
            else if (extension == ".glb")
            {
                success = loader.LoadBinaryFromFile(
                        &gltfModel,
                        &error,
                        &warning,
                        path
                );
            }
            else
            {
                MFA_CRASH("ImportGLTF format is not support: %s", extension.c_str());
            }

            if (error.empty() == false)
            {
                MFA_LOG_ERROR("ImportGltf Error: %s", error.c_str());
            }
            if (warning.empty() == false)
            {
                MFA_LOG_WARN("ImportGltf Warning: %s", warning.c_str());
            }
            if (success)
            {
                std::shared_ptr<AS::PBR::Mesh> mesh{};
                std::vector<TextureRef> textureRefs{};
                std::vector<AS::SamplerConfig> samplerConfigs {};

                // TODO Camera
                if (false == gltfModel.meshes.empty())
                {
                    // Textures
                    GLTF_extractTextures(
                        path,
                        gltfModel,
                        textureRefs,
                        samplerConfigs
                    );
                    //{// Reading samplers values from materials
                        //auto const & sampler = model.samplers[base_color_gltf_texture.sampler];
                        //model_asset.textures[base_color_texture_index]
                    //}
                    // SubMeshes
                    mesh = GLTF_extractSubMeshes(gltfModel, textureRefs);
                    if (mesh == nullptr)
                    {
                        return result;
                    }
                }
                // Nodes
                GLTF_extractNodes(gltfModel, mesh.get());
                // Fill skin
                GLTF_extractSkins(gltfModel, mesh.get());
                // Animation
                GLTF_extractAnimations(gltfModel, mesh.get());
                mesh->finalizeData();

                std::vector<std::string> textureIds (textureRefs.size());
                for (size_t i = 0; i < textureIds.size(); ++i)
                {
                    textureIds[i] = textureRefs[i].relativePath;
                }

                result = std::make_shared<AS::Model>(mesh, textureIds, samplerConfigs);
            }
        }
        return result;
    }

    //-------------------------------------------------------------------------------------------------

    RawFile ReadRawFile(std::string const & path)
    {
        RawFile rawFile{};
        if (MFA_VERIFY(path.empty() == false))
        {
#if defined(__DESKTOP__) || defined(__IOS__)
            auto file = FS::OpenFile(path, FS::Usage::Read);
            if (MFA_VERIFY(file != nullptr))
            {
                auto const fileSize = file->size();
                // TODO Allocate using a memory pool system
                auto const memoryBlob = Memory::Alloc(fileSize);
                auto const readBytes = file->read(memoryBlob->memory);
                // Means that reading is successful
                if (readBytes == fileSize)
                {
                    rawFile.data = memoryBlob;
                }
            }
#elif defined(__ANDROID__)
            auto * file = FS::Android_OpenAsset(path);
            MFA_DEFER{ FS::Android_CloseAsset(file); };
            if (FS::Android_AssetIsUsable(file))
            {
                auto const fileSize = FS::Android_AssetSize(file);
                auto const memoryBlob = Memory::Alloc(fileSize);
                auto const readBytes = FS::Android_ReadAsset(file, memoryBlob->memory);
                // Means that reading is successful
                if (readBytes == fileSize)
                {
                    rawFile.data = memoryBlob;
                }
            }
#else
#error Os not handled
#endif
        }
        return rawFile;
        }

    //-------------------------------------------------------------------------------------------------

    }
