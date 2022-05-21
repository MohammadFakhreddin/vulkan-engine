#include "./ShapeGenerator.hpp"

#include "engine/BedrockAssert.hpp"
#include "engine/BedrockMath.hpp"
#include "engine/BedrockMatrix.hpp"
#include "engine/BedrockMemory.hpp"
#include "engine/asset_system/AssetDebugMesh.hpp"

namespace MFA::ShapeGenerator
{
    namespace Debug
    {
        std::shared_ptr<AS::Model> Sphere()
        {

            using namespace AS::Debug;

            std::vector<glm::vec3> positions{};
            //std::vector<glm::vec2> uvs{};
            //std::vector<glm::vec3> normals{};
            //std::vector<glm::vec4> tangents{};

            std::vector<AS::Index> meshIndices{};

            static constexpr unsigned int X_SEGMENTS = 64;
            static constexpr unsigned int Y_SEGMENTS = 64;

            for (unsigned int y = 0; y <= Y_SEGMENTS; ++y)
            {
                for (unsigned int x = 0; x <= X_SEGMENTS; ++x)
                {
                    float xSegment = static_cast<float>(x) / static_cast<float>(X_SEGMENTS);
                    float ySegment = static_cast<float>(y) / static_cast<float>(Y_SEGMENTS);
                    float xPos = std::cos(xSegment * 2.0f * Math::PiFloat) * std::sin(ySegment * Math::PiFloat);
                    float yPos = std::cos(ySegment * Math::PiFloat);
                    float zPos = std::sin(xSegment * 2.0f * Math::PiFloat) * std::sin(ySegment * Math::PiFloat);
                    //https://computergraphics.stackexchange.com/questions/5498/compute-sphere-tangent-for-normal-mapping?newreg=93cd3b167a714b24ba01001a16545482

                    positions.emplace_back(xPos, yPos, zPos);
                    //uvs.emplace_back(xSegment, ySegment);
                    //normals.emplace_back(xPos, yPos, zPos);

                    // As solution to compute tangent I decided to rotate normal by 90 degree (In any direction :)))
                    auto rotationMatrix = glm::identity<glm::mat4>();
                    float angle[3]{ 0.0f, 0.0f, 90.0f };
                    Matrix::RotateWithEulerAngle(rotationMatrix, angle);

                    glm::vec4 tangentMatrix{ xPos, yPos, zPos, 0.0f };

                    //tangents.emplace_back(rotationMatrix * tangentMatrix);
                }
            }

            MFA_ASSERT(positions.empty() == false);
            //MFA_ASSERT(uvs.empty() == false);
            //MFA_ASSERT(normals.empty() == false);
            //MFA_ASSERT(tangents.empty() == false);

            bool oddRow = false;
            for (unsigned int y = 0; y < Y_SEGMENTS; ++y)
            {
                if (!oddRow) // even rows: y == 0, y == 2; and so on
                {
                    for (unsigned int x = 0; x <= X_SEGMENTS; ++x)
                    {
                        meshIndices.push_back(y * (X_SEGMENTS + 1) + x);
                        meshIndices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
                    }
                }
                else
                {
                    for (int x = X_SEGMENTS; x >= 0; --x)
                    {
                        meshIndices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
                        meshIndices.push_back(y * (X_SEGMENTS + 1) + x);
                    }
                }
                oddRow = !oddRow;
            }

            auto const indicesCount = static_cast<uint32_t>(meshIndices.size());
            auto const verticesCount = static_cast<uint32_t>(positions.size());

            auto mesh = std::make_shared<Mesh>();

            auto const vertexBuffer = Memory::Alloc(sizeof(Vertex) * verticesCount);
            auto const indexBuffer = Memory::Alloc(sizeof(AS::Index) * indicesCount);

            mesh->initForWrite(
                verticesCount,
                indicesCount,
                vertexBuffer,
                indexBuffer
            );

            ::memcpy(
                indexBuffer->memory.ptr,
                meshIndices.data(),
                indexBuffer->memory.len
            );

            auto * meshVertices = vertexBuffer->memory.as<Vertex>();

            for (uint32_t index = 0; index < verticesCount; ++index)
            {
                auto & vertex = meshVertices[index];

                // Positions
                Matrix::CopyGlmToCells(positions[index], vertex.position);

                //// UVs We assign uvs for all materials in case a texture get assigned to shape
                //// Base color
                //Matrix::CopyGlmToCells(uvs[index], vertex.baseColorUV);

                //// Metallic
                //Matrix::CopyGlmToCells(uvs[index], vertex.metallicUV);

                //// Roughness
                //Matrix::CopyGlmToCells(uvs[index], vertex.roughnessUV);

                //// Emission
                //Matrix::CopyGlmToCells(uvs[index], vertex.emissionUV);

                //// Normals
                //Matrix::CopyGlmToCells(uvs[index], vertex.normalMapUV);

                //// Normal buffer
                //Matrix::CopyGlmToCells(normals[index], vertex.normalValue);

                //// Tangent buffer
                //Matrix::CopyGlmToCells(tangents[index], vertex.tangentValue);

            }

            //auto const subMeshIndex = mesh->insertSubMesh();

            //Primitive primitive{};
            //primitive.hasNormalBuffer = true;

            //mesh->insertPrimitive(
            //    subMeshIndex,
            //    primitive,
            //    verticesCount,
            //    meshVertices.data(),
            //    indicesCount,
            //    meshIndices.data()
            //);

            //Node node{
            //    .subMeshIndex = 0,
            //    .scale = {1, 1, 1}
            //};

            //mesh->insertNode(node);

            mesh->finalizeData();

            return std::make_shared<AS::Model>(
                mesh,
                std::vector<std::string>{},
                std::vector<AS::SamplerConfig>{}
            );
        }

        std::shared_ptr<AS::Model> Sheet()
        {
            using namespace AS::Debug;

            std::vector<glm::vec3> positions{};
            //std::vector<glm::vec2> uvs{};

            std::vector<AS::Index> meshIndices{};

            positions.emplace_back(0.0f, 0.0f, 0.0f);
            positions.emplace_back(1.0f, 0.0f, 0.0f);
            positions.emplace_back(0.0f, 1.0f, 0.0f);
            positions.emplace_back(1.0f, 1.0f, 0.0f);

            for (auto & position : positions)
            {
                position[0] = position[0] - 0.5f;
                position[1] = position[1] - 0.5f;
            }

            //uvs.emplace_back(0.0f, 0.0f);
            //uvs.emplace_back(1.0f, 0.0f);
            //uvs.emplace_back(0.0f, 1.0f);
            //uvs.emplace_back(1.0f, 1.0f);

            meshIndices.emplace_back(0);
            meshIndices.emplace_back(1);
            meshIndices.emplace_back(2);

            meshIndices.emplace_back(1);
            meshIndices.emplace_back(2);
            meshIndices.emplace_back(3);

            auto const indicesCount = static_cast<uint16_t>(meshIndices.size());
            auto const verticesCount = static_cast<uint16_t>(positions.size());

            auto const verticesBuffer = Memory::Alloc(sizeof(Vertex) * verticesCount);
            auto const indicesBuffer = Memory::Alloc(sizeof(AS::Index) * indicesCount);

            auto mesh = std::make_shared<Mesh>();
            mesh->initForWrite(
                verticesCount,
                indicesCount,
                verticesBuffer,
                indicesBuffer
            );

            auto * meshVertices = verticesBuffer->memory.as<Vertex>();

            ::memcpy(
                indicesBuffer->memory.ptr,
                meshIndices.data(),
                indicesBuffer->memory.len
            );

            for (uint32_t index = 0; index < verticesCount; ++index)
            {
                auto & vertex = meshVertices[index];
                // Positions
                Matrix::CopyGlmToCells(positions[index], vertex.position);
                // UVs We assign uvs for all materials in case a texture get assigned to shape
                // Base color
                //Matrix::CopyGlmToCells(uvs[index], vertex.baseColorUV);
            }

            //auto const subMeshIndex = mesh->insertSubMesh();

            //Primitive primitive{};
            //primitive.hasBaseColorTexture = true;
            //mesh->insertPrimitive(
            //subMeshIndex,
            //primitive,
            //verticesCount,
            //meshVertices.data(),
            //indicesCount,
            //meshIndices.data()
            //);

            //Node node{};
            //Matrix::CopyGlmToCells(glm::identity<glm::mat4>(), node.transform);
            //mesh->insertNode(node);

            mesh->finalizeData();

            return std::make_shared<AS::Model>(
                mesh,
                std::vector<std::string>{},
                std::vector<AS::SamplerConfig> {}
            );
        }

        std::shared_ptr<AS::Model> Cube()
        {
            using namespace AS::Debug;

            std::vector<glm::vec3> const positions{
                // front
                glm::vec3(-1.0, -1.0,  1.0),
                glm::vec3(1.0, -1.0,  1.0),
                glm::vec3(1.0,  1.0,  1.0),
                glm::vec3(-1.0,  1.0,  1.0),
                // back
                glm::vec3(-1.0, -1.0, -1.0),
                glm::vec3(1.0, -1.0, -1.0),
                glm::vec3(1.0,  1.0, -1.0),
                glm::vec3(-1.0,  1.0, -1.0)
            };

            std::vector<AS::Index> const meshIndices{
                0, 1, 2, 3,
                0, 4, 5, 6,
                7, 4, 7, 3,
                2, 6, 5, 1
            };

            auto const verticesCount = static_cast<uint16_t>(positions.size());
            auto const indicesCount = static_cast<uint16_t>(meshIndices.size());

            auto const verticesBuffer = Memory::Alloc(sizeof(Vertex) * verticesCount);
            auto const indicesBuffer = Memory::Alloc(sizeof(AS::Index) * indicesCount);

            auto const mesh = std::make_shared<Mesh>();
            MFA_ASSERT(mesh != nullptr);
            mesh->initForWrite(
                verticesCount,
                indicesCount,
                verticesBuffer,
                indicesBuffer
            );

            auto * meshVertices = verticesBuffer->memory.as<Vertex>();

            ::memcpy(
                indicesBuffer->memory.ptr,
                meshIndices.data(),
                indicesBuffer->memory.len
            );

            for (uint32_t index = 0; index < verticesCount; ++index)
            {
                auto & vertex = meshVertices[index];
                // Positions
                Matrix::CopyGlmToCells(positions[index], vertex.position);
            }

            //auto const subMeshIndex = mesh->insertSubMesh();

            //Primitive primitive{};
            //primitive.hasBaseColorTexture = true;
            //mesh->insertPrimitive(
            //    subMeshIndex,
            //    primitive,
            //    verticesCount,
            //    meshVertices.data(),
            //    indicesCount,
            //    meshIndices.data()
            //);

            //Node node{};
            //node.subMeshIndex = 0;
            //Matrix::CopyGlmToCells(glm::identity<glm::mat4>(), node.transform);
            //mesh->insertNode(node);

            mesh->finalizeData();

            return std::make_shared<AS::Model>(
                mesh,
                std::vector<std::string>{},
                std::vector<AS::SamplerConfig>{}
            );

        }
    }
}
