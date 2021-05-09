#include "./ShapeGenerator.hpp"

#include "engine/BedrockMatrix.hpp"
#include "engine/BedrockMemory.hpp"

namespace MFA::ShapeGenerator {

    namespace AS = AssetSystem;

    AssetSystem::Model Sphere(float scaleFactor) {
        AssetSystem::Model model {};

        std::vector<Vector3Float> positions {};
        std::vector<Vector2Float> uvs {};
        std::vector<Vector3Float> normals {};
        std::vector<Vector4Float> tangents {};

        std::vector<AS::MeshIndex> meshIndices;
        
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
                Matrix4X4Float::AssignRotation(rotationMatrix, 0.0f, 0.0f, 90.0f);

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
                    meshIndices.push_back(y       * (X_SEGMENTS + 1) + x);
                    meshIndices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
                }
            }
            else
            {
                for (int x = X_SEGMENTS; x >= 0; --x)
                {
                    meshIndices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
                    meshIndices.push_back(y       * (X_SEGMENTS + 1) + x);
                }
            }
            oddRow = !oddRow;
        }

        uint16_t const indicesCount = static_cast<uint16_t>(meshIndices.size());
        uint16_t const verticesCount = static_cast<uint16_t>(positions.size());

        model.mesh.initForWrite(
            verticesCount, 
            indicesCount, 
            Memory::Alloc(sizeof(AS::MeshVertex) * verticesCount), 
            Memory::Alloc(sizeof(AS::MeshIndex) * indicesCount)
        );

        std::vector<AS::MeshVertex> meshVertices {verticesCount};
        
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

        auto node = AS::MeshNode {
            .subMeshIndex = 0,
            .children {},
            .transformMatrix {},
        };

        {// Assign value to transformMat
            auto transform = Matrix4X4Float::Identity();
            Matrix4X4Float::AssignScale(transform, scaleFactor);
            ::memcpy(node.transformMatrix, transform.cells, sizeof(node.transformMatrix));
            static_assert(sizeof(node.transformMatrix) == sizeof(transform.cells));
        }

        model.mesh.insertNode(node);

        if(model.mesh.isValid() == false) {
            Blob verticesBuffer {};
            Blob indicesBuffer {};
            model.mesh.revokeBuffers(verticesBuffer, indicesBuffer);
            Memory::Free(verticesBuffer);
            Memory::Free(indicesBuffer);
        }

        return model;
    }

    AssetSystem::Model Sheet() {
        AssetSystem::Model model {};

        std::vector<Vector3Float> positions {};
        std::vector<Vector2Float> uvs {};
        
        std::vector<AS::MeshIndex> meshIndices;

        positions.emplace_back(0.0f, 0.0f, 0.0f);
        positions.emplace_back(1.0f, 0.0f, 0.0f);
        positions.emplace_back(0.0f, 1.0f, 0.0f);
        positions.emplace_back(1.0f, 1.0f, 0.0f);

        for (int i = 0; i < positions.size(); ++i) {
            positions[i].set(0, 0, positions[i].get(0, 0) - 0.5f);
            positions[i].set(1, 0, positions[i].get(1, 0) - 0.5f);
        }

        uvs.emplace_back(0.0f, 0.0f);
        uvs.emplace_back(1.0f, 0.0f);
        uvs.emplace_back(0.0f, 1.0f);
        uvs.emplace_back(1.0f, 1.0f);

        meshIndices.emplace_back(0);
        meshIndices.emplace_back(1);
        meshIndices.emplace_back(2);

        meshIndices.emplace_back(1);
        meshIndices.emplace_back(2);
        meshIndices.emplace_back(3);

        uint16_t const indicesCount = static_cast<uint16_t>(meshIndices.size());
        uint16_t const verticesCount = static_cast<uint16_t>(positions.size());

        model.mesh.initForWrite(
            verticesCount, 
            indicesCount, 
            Memory::Alloc(sizeof(AS::MeshVertex) * verticesCount), 
            Memory::Alloc(sizeof(AS::MeshIndex) * indicesCount)
        );

        std::vector<AS::MeshVertex> meshVertices {verticesCount};
        
        for (uintmax_t index = 0; index < verticesCount; ++index) {
            // Positions
            static_assert(sizeof(meshVertices[index].position) == sizeof(positions[index].cells));
            ::memcpy(meshVertices[index].position, positions[index].cells, sizeof(positions[index].cells));
            // UVs We assign uvs for all materials in case a texture get assigned to shape
            // Base color
            static_assert(sizeof(meshVertices[index].baseColorUV) == sizeof(uvs[index].cells));
            ::memcpy(meshVertices[index].baseColorUV, uvs[index].cells, sizeof(uvs[index].cells));
        }

        auto const subMeshIndex = model.mesh.insertSubMesh();
        model.mesh.insertPrimitive(
            subMeshIndex,
            AS::Mesh::Primitive {
                .hasBaseColorTexture = true
            },
            verticesCount,
            meshVertices.data(),
            indicesCount,
            meshIndices.data()
        );

        auto node = AS::MeshNode {
            .subMeshIndex = 0,
            .children {},
            .transformMatrix {},
        };
        auto const identityMatrix = Matrix4X4Float::Identity();
        ::memcpy(node.transformMatrix, identityMatrix.cells, sizeof(node.transformMatrix));
        static_assert(sizeof(node.transformMatrix) == sizeof(identityMatrix.cells));
        model.mesh.insertNode(node);

        if(model.mesh.isValid() == false) {
            Blob verticesBuffer {};
            Blob indicesBuffer {};
            model.mesh.revokeBuffers(verticesBuffer, indicesBuffer);
            Memory::Free(verticesBuffer);
            Memory::Free(indicesBuffer);
        }

        return model;
    }

}
