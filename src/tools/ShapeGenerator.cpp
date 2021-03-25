#include "./ShapeGenerator.hpp"

#include "engine/BedrockMatrix.hpp"
#include "engine/BedrockMemory.hpp"

namespace MFA::ShapeGenerator {

    namespace AS = AssetSystem;
    AssetSystem::Model Sphere() {
        AssetSystem::Model model {};

        std::vector<Vector3Float> positions {};
        std::vector<Vector2Float> uvs {};
        std::vector<Vector3Float> normals {};
        std::vector<Vector4Float> tangents {};

        std::vector<U16> indices;
        
        const unsigned int X_SEGMENTS = 64;
        const unsigned int Y_SEGMENTS = 64;

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
                uvs.emplace_back(xSegment, ySegment);
                normals.emplace_back(xPos, yPos, zPos);

                // As solution to compute tangent I decided to rotate normal by 90 degree (In any direction :)))
                Matrix4X4Float rotationMatrix {};
                Matrix4X4Float::assignRotationXYZ(rotationMatrix, 0.0f, 0.0f, 90.0f);

                Matrix4X1Float tangentMatrix {};
                tangentMatrix.setX(xPos);
                tangentMatrix.setY(yPos);
                tangentMatrix.setZ(zPos);

                tangentMatrix.multiply(rotationMatrix);

                tangents.emplace_back(Vector4Float {});
                tangents.back().setX(tangentMatrix.getX());
                tangents.back().setY(tangentMatrix.getY());
                tangents.back().setZ(tangentMatrix.getZ());
            }
        }

        MFA_ASSERT(positions.empty() == false);
        MFA_ASSERT(uvs.empty() == false);
        MFA_ASSERT(normals.empty() == false);
        MFA_ASSERT(tangents.empty() == false);
        
        bool oddRow = false;
        for (unsigned int y = 0; y < Y_SEGMENTS; ++y)
        {
            if (!oddRow) // even rows: y == 0, y == 2; and so on
            {
                for (unsigned int x = 0; x <= X_SEGMENTS; ++x)
                {
                    indices.push_back(y       * (X_SEGMENTS + 1) + x);
                    indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
                }
            }
            else
            {
                for (int x = X_SEGMENTS; x >= 0; --x)
                {
                    indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
                    indices.push_back(y       * (X_SEGMENTS + 1) + x);
                }
            }
            oddRow = !oddRow;
        }

        U16 const indicesCount = static_cast<U16>(indices.size());
        U16 const verticesCount = static_cast<U16>(positions.size());

        model.mesh.initForWrite(
            verticesCount, 
            indicesCount, 
            Memory::Alloc(sizeof(AS::MeshVertex) * verticesCount), 
            Memory::Alloc(sizeof(AS::MeshIndex) * indicesCount)
        );

        MFA_DEFER {
            if(model.mesh.isValid() == false) {
                Blob verticesBuffer {};
                Blob indicesBuffer {};
                model.mesh.revokeBuffers(verticesBuffer, indicesBuffer);
                Memory::Free(verticesBuffer);
                Memory::Free(indicesBuffer);
            }
        };

        
        std::vector<AS::MeshVertex> meshVertices {verticesCount};
        std::vector<AS::MeshIndex> meshIndices {indicesCount};
        
        for(uintmax_t index = 0; index < indicesCount; ++index) {
            meshIndices[index] = indices[index];
        }

        for (uintmax_t index = 0; index < verticesCount; ++index) {
            // Positions
            static_assert(sizeof(meshVertices[index].position) == sizeof(positions[index].cells));
            ::memcpy(meshVertices[index].position, positions[index].cells, sizeof(positions[index].cells));
            // UVs We assign uvs for all materials in case a texture get assigned to shape
                // Base color
                static_assert(sizeof(meshVertices[index].baseColorUV) == sizeof(uvs[index].cells));
                ::memcpy(meshVertices[index].baseColorUV, uvs[index].cells, sizeof(uvs[index].cells));
                // Metallic
                static_assert(sizeof(meshVertices[index].metallicUV) == sizeof(uvs[index].cells));
                ::memcpy(meshVertices[index].metallicUV, uvs[index].cells, sizeof(uvs[index].cells));
                // Roughness
                static_assert(sizeof(meshVertices[index].roughnessUV) == sizeof(uvs[index].cells));
                ::memcpy(meshVertices[index].roughnessUV, uvs[index].cells, sizeof(uvs[index].cells));
                // Emission
                static_assert(sizeof(meshVertices[index].emissionUV) == sizeof(uvs[index].cells));
                ::memcpy(meshVertices[index].emissionUV, uvs[index].cells, sizeof(uvs[index].cells));
                // Normals
                static_assert(sizeof(meshVertices[index].normalMapUV) == sizeof(uvs[index].cells));
                ::memcpy(meshVertices[index].normalMapUV, uvs[index].cells, sizeof(uvs[index].cells));
            // Normal buffer
            ::memcpy(
                meshVertices[index].normalValue, 
                normals[index].cells, 
                sizeof(normals[index].cells)
            );
            // Tangent buffer
            static_assert(sizeof(meshVertices[index].tangentValue) == sizeof(tangents[index].cells));
            ::memcpy(meshVertices[index].tangentValue, tangents[index].cells, sizeof(tangents[index].cells));
        }

        auto const subMeshIndex = model.mesh.insertSubMesh();
        model.mesh.insertPrimitive(
            subMeshIndex,
            AS::Mesh::Primitive {
                .hasNormalBuffer = true
            },
            verticesCount,
            meshVertices.data(),
            indicesCount,
            meshIndices.data()
        );

        model.mesh.insertNode(AS::MeshNode {
            .subMeshIndex = 0,
            .children {},
            .transformMatrix = Matrix4X4Float::Identity(),
        });

        return model;
    }
}
