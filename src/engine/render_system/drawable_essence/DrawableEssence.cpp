#include "DrawableEssence.hpp"

#include "engine/BedrockAssert.hpp"
#include "engine/BedrockMemory.hpp"
#include "engine/render_system/RenderFrontend.hpp"

namespace MFA {

//-------------------------------------------------------------------------------------------------

// We need other overrides for easier use as well
DrawableEssence::DrawableEssence(char const * name, RT::GpuModel const & model_)
    : mName(name)
    , mGpuModel(model_)
{
    MFA_ASSERT(mName.empty() == false);
    MFA_ASSERT(mGpuModel.valid);
    MFA_ASSERT(mGpuModel.model.mesh.IsValid());
    
    auto & mesh = mGpuModel.model.mesh;

    {// PrimitiveCount
        mPrimitiveCount = 0;
        for (uint32_t i = 0; i < mesh.GetSubMeshCount(); ++i) {
            auto const & subMesh = mesh.GetSubMeshByIndex(i);
            mPrimitiveCount += static_cast<uint32_t>(subMesh.primitives.size());
        }
        if (mPrimitiveCount > 0) {
            size_t const bufferSize = sizeof(PrimitiveInfo) * mPrimitiveCount;
            mPrimitivesBuffer = RF::CreateUniformBuffer(bufferSize, 1);
            Blob primitiveData = Memory::Alloc(bufferSize);
            MFA_DEFER {
                Memory::Free(primitiveData);
            };

            auto * primitivesArray = primitiveData.as<PrimitiveInfo>();
            for (uint32_t i = 0; i < mesh.GetSubMeshCount(); ++i) {
                auto const & subMesh = mesh.GetSubMeshByIndex(i);
                for (auto const & primitive : subMesh.primitives) {

                    // Copy primitive into primitive info
                    PrimitiveInfo & primitiveInfo = primitivesArray[primitive.uniqueId];
                    primitiveInfo.baseColorTextureIndex = primitive.hasBaseColorTexture ? primitive.baseColorTextureIndex : -1;
                    primitiveInfo.metallicFactor = primitive.metallicFactor;
                    primitiveInfo.roughnessFactor = primitive.roughnessFactor;
                    primitiveInfo.metallicRoughnessTextureIndex = primitive.hasMetallicRoughnessTexture ? primitive.metallicRoughnessTextureIndex : -1;
                    primitiveInfo.normalTextureIndex = primitive.hasNormalTexture ? primitive.normalTextureIndex : -1;
                    primitiveInfo.emissiveTextureIndex = primitive.hasEmissiveTexture ? primitive.emissiveTextureIndex : -1;
                    primitiveInfo.hasSkin = primitive.hasSkin ? 1 : 0;
                    // TODO Occlusion

                    ::memcpy(primitiveInfo.baseColorFactor, primitive.baseColorFactor, sizeof(primitiveInfo.baseColorFactor));
                    static_assert(sizeof(primitiveInfo.baseColorFactor) == sizeof(primitive.baseColorFactor));
                    ::memcpy(primitiveInfo.emissiveFactor, primitive.emissiveFactor, sizeof(primitiveInfo.emissiveFactor));
                    static_assert(sizeof(primitiveInfo.emissiveFactor) == sizeof(primitive.emissiveFactor));
                }
            }

            RF::UpdateUniformBuffer(mPrimitivesBuffer.buffers[0], primitiveData);
        }
    }
}

//-------------------------------------------------------------------------------------------------

DrawableEssence::~DrawableEssence() {
    if (mPrimitivesBuffer.bufferSize > 0) {
        RF::DestroyUniformBuffer(mPrimitivesBuffer);
    }
}

//-------------------------------------------------------------------------------------------------

RT::GpuModel const & DrawableEssence::GetGpuModel() const {
    return mGpuModel;
}

//-------------------------------------------------------------------------------------------------

RT::UniformBufferGroup const & DrawableEssence::GetPrimitivesBuffer() const noexcept {
    return mPrimitivesBuffer;
}

//-------------------------------------------------------------------------------------------------

std::string const & DrawableEssence::GetName() const noexcept {
    return mName;
}

//-------------------------------------------------------------------------------------------------

};
