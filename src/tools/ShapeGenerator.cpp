#include "./ShapeGenerator.hpp"

#include "engine/BedrockMatrix.hpp"
#include "engine/BedrockMemory.hpp"

namespace MFA::ShapeGenerator {

    AssetSystem::ModelAsset Sphere() {
        AssetSystem::ModelAsset model_asset {};

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

        U16 const indices_count = static_cast<U16>(indices.size());
        U16 const vertices_count = static_cast<U16>(positions.size());

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

        model_asset.mesh = AssetSystem::MeshAsset(mesh_asset_blob);
        auto * mesh_header = mesh_asset_blob.as<AssetSystem::MeshHeader>();
        mesh_header->sub_mesh_count = 1;
        mesh_header->total_vertex_count = vertices_count;
        mesh_header->total_index_count = indices_count;
        // We only have 1 sub_mesh for sphere
        auto & sub_mesh = mesh_header->sub_meshes[0];
        sub_mesh.vertex_count = vertices_count;
        sub_mesh.index_count = indices_count;
        sub_mesh.vertices_offset = header_size;
        sub_mesh.indices_offset = header_size + vertices_count * sizeof(AssetSystem::MeshVertex);
        sub_mesh.has_metallic_roughness = false;
        sub_mesh.has_normal_buffer = true;
        sub_mesh.has_normal_texture = false;
        sub_mesh.has_emissive_texture = false;
        sub_mesh.indices_starting_index = 0;
        
        auto * mesh_vertices = model_asset.mesh.vertices_blob(0).as<AssetSystem::MeshVertex>();
        auto * mesh_indices = model_asset.mesh.indices_blob(0).as<AssetSystem::MeshIndex>();
        MFA_ASSERT(model_asset.mesh.indices_blob(0).ptr + model_asset.mesh.indices_blob(0).len == mesh_asset_blob.ptr + mesh_asset_blob.len);

        for(
            uintmax_t index = 0;
            index < sub_mesh.index_count;
            ++index
        ) {
            mesh_indices[index] = indices[index];
        }
        for (
            uintmax_t index = 0;
            index < sub_mesh.vertex_count;
            ++index
        ) {
            // Positions
            static_assert(sizeof(mesh_vertices[index].position) == sizeof(positions[index].cells));
            ::memcpy(mesh_vertices[index].position, positions[index].cells, sizeof(positions[index].cells));
            // UVs We assign uvs for all materials in case a texture get assigned to shape
                // Base color
                static_assert(sizeof(mesh_vertices[index].base_color_uv) == sizeof(uvs[index].cells));
                ::memcpy(mesh_vertices[index].base_color_uv, uvs[index].cells, sizeof(uvs[index].cells));
                // Metallic/Roughness       // TODO We might need to separate metallic_roughness
                static_assert(sizeof(mesh_vertices[index].metallic_roughness_uv) == sizeof(uvs[index].cells));
                ::memcpy(mesh_vertices[index].metallic_roughness_uv, uvs[index].cells, sizeof(uvs[index].cells));
                // Emission
                static_assert(sizeof(mesh_vertices[index].emission_uv) == sizeof(uvs[index].cells));
                ::memcpy(mesh_vertices[index].emission_uv, uvs[index].cells, sizeof(uvs[index].cells));
                // Normals
                static_assert(sizeof(mesh_vertices[index].normal_map_uv) == sizeof(uvs[index].cells));
                ::memcpy(mesh_vertices[index].normal_map_uv, uvs[index].cells, sizeof(uvs[index].cells));
            // Normal buffer
            ::memcpy(
                mesh_vertices[index].normal_value, 
                normals[index].cells, 
                sizeof(normals[index].cells)
            );
            // Tangent buffer
            static_assert(sizeof(mesh_vertices[index].tangent_value) == sizeof(tangents[index].cells));
            ::memcpy(mesh_vertices[index].tangent_value, tangents[index].cells, sizeof(tangents[index].cells));
        }

        MFA_ASSERT(model_asset.mesh.valid());
        if(true == model_asset.mesh.valid()) {
            mesh_asset_blob = {};
        }

        return model_asset;
    }
}
