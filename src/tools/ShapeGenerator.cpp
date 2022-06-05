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
            
            std::vector<AS::Index> meshIndices{};

            static constexpr unsigned int X_SEGMENTS = 64;
            static constexpr unsigned int Y_SEGMENTS = 64;

            for (unsigned int y = 0; y <= Y_SEGMENTS; ++y)
            {
                for (unsigned int x = 0; x <= X_SEGMENTS; ++x)
                {
                    float const xSegment = static_cast<float>(x) / static_cast<float>(X_SEGMENTS);
                    float const ySegment = static_cast<float>(y) / static_cast<float>(Y_SEGMENTS);
                    float xPos = std::cos(xSegment * 2.0f * Math::PiFloat) * std::sin(ySegment * Math::PiFloat);
                    float yPos = std::cos(ySegment * Math::PiFloat);
                    float zPos = std::sin(xSegment * 2.0f * Math::PiFloat) * std::sin(ySegment * Math::PiFloat);
                    //https://computergraphics.stackexchange.com/questions/5498/compute-sphere-tangent-for-normal-mapping?newreg=93cd3b167a714b24ba01001a16545482

                    positions.emplace_back(xPos, yPos, zPos);
                }
            }

            MFA_ASSERT(positions.empty() == false);
            
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
            }

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
            }

            mesh->finalizeData();

            return std::make_shared<AS::Model>(
                mesh,
                std::vector<std::string>{},
                std::vector<AS::SamplerConfig> {}
            );
        }

        std::shared_ptr<AS::Model> CubeStrip()
        {
            using namespace AS::Debug;

            std::vector<glm::vec3> const vertices{
                // front
                glm::vec3(-0.5f, -0.5f, 0.5f),
                glm::vec3(0.5f, -0.5f, 0.5f),
                glm::vec3(0.5f, 0.5f, 0.5f),
                glm::vec3(-0.5f, 0.5f, 0.5f),
                // back
                glm::vec3(-0.5f, -0.5f, -0.5f),
                glm::vec3(0.5f, -0.5f, -0.5f),
                glm::vec3(0.5f, 0.5f, -0.5f),
                glm::vec3(-0.5f, 0.5f, -0.5f)
            };

            std::vector<AS::Index> const indices{
                0, 1, 2, 3,
                0, 4, 5, 6,
                7, 4, 7, 3,
                2, 6, 5, 1
            };

            auto const verticesCount = static_cast<uint16_t>(vertices.size());
            auto const indicesCount = static_cast<uint16_t>(indices.size());

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
                indices.data(),
                indicesBuffer->memory.len
            );

            for (uint32_t index = 0; index < verticesCount; ++index)
            {
                auto & vertex = meshVertices[index];
                // Positions
                Matrix::CopyGlmToCells(vertices[index], vertex.position);
            }

            mesh->finalizeData();

            return std::make_shared<AS::Model>(
                mesh,
                std::vector<std::string>{},
                std::vector<AS::SamplerConfig>{}
            );

        }

        std::shared_ptr<AS::Model> CubeFill()
        {
            using namespace AS::Debug;

            std::vector<AS::Index> const indices {
                //Top
                2, 6, 7,
                2, 3, 7,

                //Bottom
                0, 4, 5,
                0, 1, 5,

                //Left
                0, 2, 6,
                0, 4, 6,

                //Right
                1, 3, 7,
                1, 5, 7,

                //Front
                0, 2, 3,
                0, 1, 3,

                //Back
                4, 6, 7,
                4, 5, 7
            };

            std::vector<glm::vec3> const vertices {
                glm::vec3(-0.5f,  -0.5f, 0.5), //0
                glm::vec3( 0.5f,  -0.5f, 0.5), //1
                glm::vec3(-0.5f,  0.5f,  0.5), //2
                glm::vec3( 0.5f,  0.5f,  0.5), //3
                glm::vec3(-0.5f,  -0.5f, -0.5), //4
                glm::vec3( 0.5f,  -0.5f, -0.5), //5
                glm::vec3(-0.5f,  0.5f,  -0.5), //6
                glm::vec3( 0.5f,  0.5f,  -0.5)  //7
            };

            auto const verticesCount = static_cast<uint16_t>(vertices.size());
            auto const indicesCount = static_cast<uint16_t>(indices.size());

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
                indices.data(),
                indicesBuffer->memory.len
            );

            for (uint32_t index = 0; index < verticesCount; ++index)
            {
                auto & vertex = meshVertices[index];
                // Positions
                Matrix::CopyGlmToCells(vertices[index], vertex.position);
            }

            mesh->finalizeData();

            return std::make_shared<AS::Model>(
                mesh,
                std::vector<std::string>{},
                std::vector<AS::SamplerConfig>{}
            );

        }

    }
}
