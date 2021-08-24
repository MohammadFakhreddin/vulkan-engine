#include "PBRWithShadowPipelineV2.hpp"

#include "engine/BedrockPath.hpp"
#include "engine/camera/FirstPersonCamera.hpp"
#include "engine/render_system/render_passes/DisplayRenderPass.hpp"
#include "tools/Importer.hpp"
#include "engine/BedrockMatrix.hpp"


//https://stackoverflow.com/questions/54182706/why-do-i-need-resources-per-swapchain-image
namespace MFA {

void PBRWithShadowPipelineV2::Init(
    RF::SamplerGroup * samplerGroup, 
    RB::GpuTexture * errorTexture,
    float const projectionNear,
    float const projectionFar
) {
    if (mIsInitialized == true) {
        MFA_ASSERT(false);
        return;
    }
    mIsInitialized = true;
    MFA_ASSERT(samplerGroup != nullptr);
    mSamplerGroup = samplerGroup;
    MFA_ASSERT(errorTexture != nullptr);
    mErrorTexture = errorTexture;

    mProjectionFar = projectionFar;
    mProjectionNear = projectionNear;
    mProjectionFarToNearDistance = projectionFar - projectionNear;
    MFA_ASSERT(mProjectionFarToNearDistance > 0.0f);
    
    createUniformBuffers();

    createDisplayPassDescriptorSetLayout();
    createDisplayPassPipeline();

    mShadowRenderPass.Init();
    createShadowPassDescriptorSetLayout();
    createShadowPassPipeline();

    static float FOV = 90.0f;          // TODO Maybe it should be the same with our camera's FOV.
    
    const float ratio = SHADOW_WIDTH / SHADOW_HEIGHT;

    Matrix4X4Float projectionMatrix4X4 {};
    Matrix4X4Float::PreparePerspectiveProjectionMatrix(
        projectionMatrix4X4,
        ratio,
        FOV,
        mProjectionNear,
        mProjectionFar
    );

    mShadowProjection = Matrix4X4Float::ConvertCellsToMat4(projectionMatrix4X4.cells);

    updateShadowLightBuffer();
    updateDisplayLightBuffer();
    updateViewProjectionBuffer();
}

void PBRWithShadowPipelineV2::Shutdown() {
    if (mIsInitialized == false) {
        MFA_ASSERT(false);
        return;
    }
    mIsInitialized = false;
    mSamplerGroup = nullptr;
    mErrorTexture = nullptr;

    mShadowRenderPass.Shutdown();

    destroyPipeline();
    destroyDescriptorSetLayout();
    destroyUniformBuffers();
}

// TODO Updating buffer inside commandBuffer might not be a good idea, Research about it!
void PBRWithShadowPipelineV2::PreRender(
    RF::DrawPass & drawPass, 
    float const deltaTime, 
    uint32_t const idsCount, 
    DrawableObjectId * ids
) {

    for (uint32_t i = 0; i < idsCount; ++i) {
        auto const findResult = mDrawableObjects.find(ids[i]);
        if (findResult != mDrawableObjects.end()) {
            auto * drawableObject = findResult->second.get();
            MFA_ASSERT(drawableObject != nullptr);
            drawableObject->Update(deltaTime);
        }
    }

    mShadowRenderPass.PrepareCubemapForTransferDestination(drawPass);
    for (int i = 0; i < 6; ++i) {
        mShadowRenderPass.SetNextPassParams(i);
        mShadowRenderPass.BeginRenderPass(drawPass);

        RF::BindDrawPipeline(drawPass, mShadowPassPipeline);

        {// Shadow constants
            ShadowPushConstants shadowConstants {
                .faceIndex = i
            };
            RF::PushConstants(
                drawPass,
                AS::ShaderStage::Vertex,
                0,
                CBlobAliasOf(shadowConstants)
            );
        }

        // TODO We can query drawable objects once
        for (uint32_t i = 0; i < idsCount; ++i) {
            auto const findResult = mDrawableObjects.find(ids[i]);
            if (findResult != mDrawableObjects.end()) {
                auto * drawableObject = findResult->second.get();
                MFA_ASSERT(drawableObject != nullptr);
                drawableObject->Draw(drawPass, [&drawPass, &drawableObject](AS::MeshPrimitive const & primitive)-> void {
                    RF::BindDescriptorSet(
                        drawPass, 
                        drawableObject->GetDescriptorSetGroup("ShadowPipeline")->descriptorSets[primitive.uniqueId]
                    );
                });
            }
        }

        mShadowRenderPass.EndRenderPass(drawPass);
    }
    mShadowRenderPass.PrepareCubemapForSampling(drawPass);
}

void PBRWithShadowPipelineV2::Render(
    RF::DrawPass & drawPass, 
    float const deltaTime, 
    uint32_t const idsCount, 
    DrawableObjectId * ids
) {
    RF::BindDrawPipeline(drawPass, mDisplayPassPipeline);
    
    for (uint32_t i = 0; i < idsCount; ++i) {
        auto const findResult = mDrawableObjects.find(ids[i]);
        if (findResult != mDrawableObjects.end()) {
            auto * drawableObject = findResult->second.get();
            MFA_ASSERT(drawableObject != nullptr);
            drawableObject->Draw(drawPass, [&drawPass, &drawableObject](AS::MeshPrimitive const & primitive)-> void {
                RF::BindDescriptorSet(
                    drawPass, 
                    drawableObject->GetDescriptorSetGroup("DisplayPipeline")->descriptorSets[primitive.uniqueId]
                );
            });
        }
    }
}

void PBRWithShadowPipelineV2::PostRender(
    RF::DrawPass & drawPass, 
    float const deltaTime, 
    uint32_t const idsCount, 
    DrawableObjectId * ids
) {

}

DrawableObjectId PBRWithShadowPipelineV2::AddGpuModel(RF::GpuModel & gpuModel) {
    MFA_ASSERT(gpuModel.valid == true);

    auto const drawableObjectId = mNextDrawableObjectId ++;
    MFA_ASSERT(mDrawableObjects.find(drawableObjectId) == mDrawableObjects.end());
    mDrawableObjects[drawableObjectId] = std::make_unique<DrawableObject>(gpuModel);

    auto * drawableObject = mDrawableObjects[drawableObjectId].get();
    MFA_ASSERT(drawableObject != nullptr);

    drawableObject->CreateUniformBuffer(
        "Model", 
        sizeof(ModelData)
    );
    
    CreateDisplayPassDescriptorSets(drawableObject);
    CreateShadowPassDescriptorSets(drawableObject);
    
    return drawableObjectId;
}

bool PBRWithShadowPipelineV2::RemoveGpuModel(DrawableObjectId const drawableObjectId) {
    auto deleteCount = mDrawableObjects.erase(drawableObjectId);
    MFA_ASSERT(deleteCount == 1);
    return deleteCount;
}

bool PBRWithShadowPipelineV2::UpdateModel(
    DrawableObjectId const drawableObjectId, 
    ModelData const & modelData
) {
    // We should share buffers
    auto const findResult = mDrawableObjects.find(drawableObjectId);
    if (findResult == mDrawableObjects.end()) {
        MFA_ASSERT(false);
        return false;
    }
    auto * drawableObject = findResult->second.get();
    MFA_ASSERT(drawableObject != nullptr);

    drawableObject->UpdateUniformBuffer(
        "Model", 
        CBlobAliasOf(modelData)
    );
    return true;
}

void PBRWithShadowPipelineV2::UpdateCameraView(DisplayViewData const & viewData) {
    RF::UpdateUniformBuffer(
        mDisplayViewBuffer.buffers[0],
        CBlobAliasOf(viewData)
    );
}

void PBRWithShadowPipelineV2::UpdateCameraProjection(DisplayProjectionData const & projectionData) {
    RF::UpdateUniformBuffer(
        mDisplayProjectionBuffer.buffers[0], 
        CBlobAliasOf(projectionData)
    );
}

// TODO Make 2 shader data similar to each other
void PBRWithShadowPipelineV2::UpdateLightPosition(float lightPosition[3]) {
    if (IsEqual<3>(mLightPosition, lightPosition) == false) {
        Copy<3>(mLightPosition, lightPosition);
        updateDisplayLightBuffer();
        updateShadowLightBuffer();
        updateViewProjectionBuffer();
    }
}

void PBRWithShadowPipelineV2::UpdateCameraPosition(float cameraPosition[3]) {
    if (IsEqual<3>(mCameraPosition, cameraPosition) == false) {
        Copy<3>(mCameraPosition, cameraPosition);
        updateDisplayLightBuffer();
    }
}

void PBRWithShadowPipelineV2::UpdateLightColor(float lightColor[3]) {
    if (IsEqual<3>(mLightColor, lightColor) == false) {
        Copy<3>(mLightColor, lightColor);
        updateDisplayLightBuffer();
    }
}

void PBRWithShadowPipelineV2::updateDisplayLightBuffer() {
    DisplayLightAndCameraData displayLightViewData {
        .projectFarToNearDistance = mProjectionFarToNearDistance
    };
    Copy<3>(displayLightViewData.cameraPosition, mCameraPosition);
    Copy<3>(displayLightViewData.lightColor, mLightColor);
    Copy<3>(displayLightViewData.lightPosition, mLightPosition);
    RF::UpdateUniformBuffer(mDisplayLightAndCameraBuffer.buffers[0], CBlobAliasOf(displayLightViewData));
}

void PBRWithShadowPipelineV2::updateShadowLightBuffer() {
    ShadowLightData shadowLightData {
        .projectionMatrixDistance = mProjectionFarToNearDistance
    };
    Copy<3>(shadowLightData.lightPosition, mLightPosition);
    RF::UpdateUniformBuffer(mShadowLightBuffer.buffers[0], CBlobAliasOf(shadowLightData));
}

void PBRWithShadowPipelineV2::updateViewProjectionBuffer() {
    auto const lightPositionVector = glm::vec3(mLightPosition[0], mLightPosition[1], mLightPosition[2]);

    Matrix4X4Float::ConvertGmToCells(
        mShadowProjection * glm::lookAt(
            lightPositionVector, 
            lightPositionVector + glm::vec3( 1.0, 0.0, 0.0), 
            glm::vec3(0.0,-1.0, 0.0)
        ), 
        mShadowViewProjectionData.viewMatrices[0]
    );

    Matrix4X4Float::ConvertGmToCells(
        mShadowProjection * glm::lookAt(
            lightPositionVector, 
            lightPositionVector + glm::vec3(-1.0, 0.0, 0.0), 
            glm::vec3(0.0,-1.0, 0.0)
        ), 
        mShadowViewProjectionData.viewMatrices[1]
    );

    Matrix4X4Float::ConvertGmToCells(
        mShadowProjection * glm::lookAt(
            lightPositionVector, 
            lightPositionVector + glm::vec3( 0.0, 1.0, 0.0), 
            glm::vec3(0.0, 0.0, 1.0)
        ), 
        mShadowViewProjectionData.viewMatrices[2]
    );

    Matrix4X4Float::ConvertGmToCells(
        mShadowProjection * glm::lookAt(
            lightPositionVector, 
            lightPositionVector + glm::vec3( 0.0,-1.0, 0.0), 
            glm::vec3(0.0, 0.0,-1.0)
        ), 
        mShadowViewProjectionData.viewMatrices[3]
    );

    Matrix4X4Float::ConvertGmToCells(
        mShadowProjection * glm::lookAt(
            lightPositionVector, 
            lightPositionVector + glm::vec3( 0.0, 0.0, 1.0), 
            glm::vec3(0.0,-1.0, 0.0)
        ), 
        mShadowViewProjectionData.viewMatrices[4]
    );

    Matrix4X4Float::ConvertGmToCells(
        mShadowProjection * glm::lookAt(
            lightPositionVector, 
            lightPositionVector + glm::vec3( 0.0, 0.0,-1.0), 
            glm::vec3(0.0,-1.0, 0.0)
        ), 
        mShadowViewProjectionData.viewMatrices[5]
    );

    RF::UpdateUniformBuffer(mShadowViewProjectionBuffer.buffers[0], CBlobAliasOf(mShadowViewProjectionData));
}

DrawableObject * PBRWithShadowPipelineV2::GetDrawableById(DrawableObjectId const objectId) {
    auto const findResult = mDrawableObjects.find(objectId);
    if (findResult != mDrawableObjects.end()) {
        return findResult->second.get();
    }
    return nullptr;
}

void PBRWithShadowPipelineV2::CreateDisplayPassDescriptorSets(DrawableObject * drawableObject) {
    const auto * modelTransformBuffer = drawableObject->GetUniformBuffer("Model");
    MFA_ASSERT(modelTransformBuffer != nullptr);

    // TODO Maybe "mesh" should track number of primitives instead
    const auto * primitiveInfoBuffer = drawableObject->CreateMultipleUniformBuffer(
        "PrimitiveInfo", 
        sizeof(PrimitiveInfo), 
        drawableObject->GetPrimitiveCount()
    );
    MFA_ASSERT(primitiveInfoBuffer != nullptr);

    const auto & nodeTransformBuffer = drawableObject->GetNodeTransformBuffer();

    auto const & textures = drawableObject->GetModel()->textures;

    auto const & mesh = drawableObject->GetModel()->model.mesh;

    auto const descriptorSetGroup = drawableObject->CreateDescriptorSetGroup("DisplayPipeline", mDisplayPassDescriptorSetLayout, drawableObject->GetPrimitiveCount());

    for (uint32_t nodeIndex = 0; nodeIndex < mesh.GetNodesCount(); ++nodeIndex) {// Updating descriptor sets
        auto const & node = mesh.GetNodeByIndex(nodeIndex);
        if (node.hasSubMesh()) {
            auto const & subMesh = mesh.GetSubMeshByIndex(node.subMeshIndex);
            if (subMesh.primitives.empty() == false) {
                for (auto const & primitive : subMesh.primitives) {

                    MFA_ASSERT(primitive.uniqueId >= 0);
                    auto descriptorSet = descriptorSetGroup.descriptorSets[primitive.uniqueId];
                    MFA_VK_VALID_ASSERT(descriptorSet);

                    DescriptorSetSchema descriptorSetSchema {descriptorSet};

                    /////////////////////////////////////////////////////////////////
                    // Vertex shader
                    /////////////////////////////////////////////////////////////////
                    
                    {// ModelTransform
                        VkDescriptorBufferInfo bufferInfo {
                            .buffer = modelTransformBuffer->buffers[0].buffer,
                            .offset = 0,
                            .range = modelTransformBuffer->bufferSize,
                        };
                        descriptorSetSchema.AddUniformBuffer(bufferInfo);
                    }

                    {// ViewTransform
                        VkDescriptorBufferInfo bufferInfo {
                            .buffer = mDisplayViewBuffer.buffers[0].buffer,
                            .offset = 0,
                            .range = mDisplayViewBuffer.bufferSize,
                        };
                        descriptorSetSchema.AddUniformBuffer(bufferInfo);
                    }

                    {// ProjectionTransform
                        VkDescriptorBufferInfo bufferInfo {
                            .buffer = mDisplayProjectionBuffer.buffers[0].buffer,
                            .offset = 0,
                            .range = mDisplayProjectionBuffer.bufferSize,
                        };
                        descriptorSetSchema.AddUniformBuffer(bufferInfo);
                    }

                    {//NodeTransform
                        VkDescriptorBufferInfo nodeTransformBufferInfo {
                            .buffer = nodeTransformBuffer.buffers[node.subMeshIndex].buffer,
                            .offset = 0,
                            .range = nodeTransformBuffer.bufferSize
                        };

                        descriptorSetSchema.AddUniformBuffer(nodeTransformBufferInfo);
                    }

                    {// SkinJoints
                        auto const & skinBuffer = node.skinBufferIndex >= 0
                            ? drawableObject->GetSkinTransformBuffer(node.skinBufferIndex)
                            : mErrorBuffer;

                        VkDescriptorBufferInfo skinTransformBufferInfo {};
                        skinTransformBufferInfo.buffer = skinBuffer.buffers[0].buffer;
                        skinTransformBufferInfo.offset = 0;
                        skinTransformBufferInfo.range = skinBuffer.bufferSize;

                        descriptorSetSchema.AddUniformBuffer(skinTransformBufferInfo);
                    }

                    /////////////////////////////////////////////////////////////////
                    // Fragment shader
                    /////////////////////////////////////////////////////////////////
                    
                    {// Primitive
                        VkDescriptorBufferInfo primitiveBufferInfo {};
                        primitiveBufferInfo.buffer = primitiveInfoBuffer->buffers[primitive.uniqueId].buffer;
                        primitiveBufferInfo.offset = 0;
                        primitiveBufferInfo.range = primitiveInfoBuffer->bufferSize;

                        descriptorSetSchema.AddUniformBuffer(primitiveBufferInfo);
                    }

                    {// Update primitive buffer information
                        PrimitiveInfo primitiveInfo {};
                        primitiveInfo.hasBaseColorTexture = primitive.hasBaseColorTexture ? 1 : 0;
                        primitiveInfo.metallicFactor = primitive.metallicFactor;
                        primitiveInfo.roughnessFactor = primitive.roughnessFactor;
                        primitiveInfo.hasMixedMetallicRoughnessOcclusionTexture = primitive.hasMixedMetallicRoughnessOcclusionTexture ? 1 : 0;
                        primitiveInfo.hasNormalTexture = primitive.hasNormalTexture ? 1 : 0;
                        primitiveInfo.hasEmissiveTexture = primitive.hasEmissiveTexture ? 1 : 0;
                        primitiveInfo.hasSkin = primitive.hasSkin ? 1 : 0;

                        ::memcpy(primitiveInfo.baseColorFactor, primitive.baseColorFactor, sizeof(primitiveInfo.baseColorFactor));
                        static_assert(sizeof(primitiveInfo.baseColorFactor) == sizeof(primitive.baseColorFactor));
                        ::memcpy(primitiveInfo.emissiveFactor, primitive.emissiveFactor, sizeof(primitiveInfo.emissiveFactor));
                        static_assert(sizeof(primitiveInfo.emissiveFactor) == sizeof(primitive.emissiveFactor));
                        RF::UpdateUniformBuffer(
                            primitiveInfoBuffer->buffers[primitive.uniqueId], 
                            CBlobAliasOf(primitiveInfo)
                        );
                    }

                    {// BaseColorTexture
                        VkDescriptorImageInfo baseColorImageInfo {
                            .sampler = mSamplerGroup->sampler,          // TODO Each texture has it's own properties that may need it's own sampler (Not sure yet)
                            .imageView = primitive.hasBaseColorTexture
                                    ? textures[primitive.baseColorTextureIndex].image_view()
                                    : mErrorTexture->image_view(),
                            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                        };

                        descriptorSetSchema.AddCombinedImageSampler(baseColorImageInfo);
                    }

                    {// Metallic/RoughnessTexture
                        VkDescriptorImageInfo metallicImageInfo {};
                        metallicImageInfo.sampler = mSamplerGroup->sampler;          // TODO Each texture has it's own properties that may need it's own sampler (Not sure yet)
                        metallicImageInfo.imageView = primitive.hasMixedMetallicRoughnessOcclusionTexture
                                ? textures[primitive.mixedMetallicRoughnessOcclusionTextureIndex].image_view()
                                : mErrorTexture->image_view();
                        metallicImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

                        descriptorSetSchema.AddCombinedImageSampler(metallicImageInfo);
                    }

                    {// NormalTexture  
                        VkDescriptorImageInfo normalImageInfo {};
                        normalImageInfo.sampler = mSamplerGroup->sampler,
                        normalImageInfo.imageView = primitive.hasNormalTexture
                                ? textures[primitive.normalTextureIndex].image_view()
                                : mErrorTexture->image_view();
                        normalImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

                        descriptorSetSchema.AddCombinedImageSampler(normalImageInfo);
                    }

                    {// EmissiveTexture
                        VkDescriptorImageInfo emissiveImageInfo {};
                        emissiveImageInfo.sampler = mSamplerGroup->sampler;
                        emissiveImageInfo.imageView = primitive.hasEmissiveTexture
                                ? textures[primitive.emissiveTextureIndex].image_view()
                                : mErrorTexture->image_view();
                        emissiveImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

                        descriptorSetSchema.AddCombinedImageSampler(emissiveImageInfo);
                    }

                    {// LightViewBuffer
                        VkDescriptorBufferInfo lightViewBufferInfo {};
                        lightViewBufferInfo.buffer = mDisplayLightAndCameraBuffer.buffers[0].buffer;
                        lightViewBufferInfo.offset = 0;
                        lightViewBufferInfo.range = mDisplayLightAndCameraBuffer.bufferSize;

                        descriptorSetSchema.AddUniformBuffer(lightViewBufferInfo);
                    }

                    {// ShadowMap
                        VkDescriptorImageInfo shadowMapImageInfo {};
                        shadowMapImageInfo.sampler = mSamplerGroup->sampler;
                        shadowMapImageInfo.imageView = mShadowRenderPass.GetDepthCubeMap().imageView;
                        shadowMapImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

                        descriptorSetSchema.AddCombinedImageSampler(shadowMapImageInfo);
                    }

                    descriptorSetSchema.UpdateDescriptorSets();
                }
            }
        }
    }
}

void PBRWithShadowPipelineV2::CreateShadowPassDescriptorSets(DrawableObject * drawableObject) {
    const auto * modelBuffer = drawableObject->GetUniformBuffer("Model");
    MFA_ASSERT(modelBuffer != nullptr);

    const auto & nodeTransformBuffer = drawableObject->GetNodeTransformBuffer();

    auto const & mesh = drawableObject->GetModel()->model.mesh;
    // TODO Each subMesh should know its node index as well
    auto const descriptorSetGroup = drawableObject->CreateDescriptorSetGroup(
        "ShadowPipeline", 
        mShadowPassDescriptorSetLayout, 
        drawableObject->GetPrimitiveCount()
    );
    
    for (uint32_t nodeIndex = 0; nodeIndex < mesh.GetNodesCount(); ++nodeIndex) {// Updating descriptor sets
        auto const & node = mesh.GetNodeByIndex(nodeIndex);
        if (node.hasSubMesh()) {
            auto const & subMesh = mesh.GetSubMeshByIndex(node.subMeshIndex);
            if (subMesh.primitives.empty() == false) {
                for (auto const & primitive : subMesh.primitives) {
                    MFA_ASSERT(primitive.uniqueId >= 0);

                    auto descriptorSet = descriptorSetGroup.descriptorSets[primitive.uniqueId];
                    MFA_VK_VALID_ASSERT(descriptorSet);

                    DescriptorSetSchema descriptorSetSchema {descriptorSet};

                    /////////////////////////////////////////////////////////////////
                    // Vertex shader
                    /////////////////////////////////////////////////////////////////
                    
                    {// ModelTransform
                        VkDescriptorBufferInfo modelBufferInfo {
                            .buffer = modelBuffer->buffers[0].buffer,
                            .offset = 0,
                            .range = modelBuffer->bufferSize,
                        };
                        descriptorSetSchema.AddUniformBuffer(modelBufferInfo);
                    }
                    {// ViewProjectionTransform
                        VkDescriptorBufferInfo viewProjectionBufferInfo {
                            .buffer = mShadowViewProjectionBuffer.buffers[0].buffer,
                            .offset = 0,
                            .range = mShadowViewProjectionBuffer.bufferSize
                        };
                        descriptorSetSchema.AddUniformBuffer(viewProjectionBufferInfo);
                    }
                    {//NodeTransform
                        VkDescriptorBufferInfo nodeTransformBufferInfo {
                            .buffer = nodeTransformBuffer.buffers[node.subMeshIndex].buffer,
                            .offset = 0,
                            .range = nodeTransformBuffer.bufferSize
                        };

                        descriptorSetSchema.AddUniformBuffer(nodeTransformBufferInfo);
                    }
                    {// SkinJoints
                        auto const & skinBuffer = node.skinBufferIndex >= 0
                            ? drawableObject->GetSkinTransformBuffer(node.skinBufferIndex)
                            : mErrorBuffer;

                        VkDescriptorBufferInfo skinTransformBufferInfo {};
                        skinTransformBufferInfo.buffer = skinBuffer.buffers[0].buffer;
                        skinTransformBufferInfo.offset = 0;
                        skinTransformBufferInfo.range = skinBuffer.bufferSize;

                        descriptorSetSchema.AddUniformBuffer(skinTransformBufferInfo);
                    }

                    /////////////////////////////////////////////////////////////////
                    // Fragment shader
                    /////////////////////////////////////////////////////////////////

                    {// LightBuffer
                        VkDescriptorBufferInfo bufferInfo {};
                        bufferInfo.buffer = mShadowLightBuffer.buffers[0].buffer;
                        bufferInfo.offset = 0;
                        bufferInfo.range = mShadowLightBuffer.bufferSize;

                        descriptorSetSchema.AddUniformBuffer(bufferInfo);
                    }

                    descriptorSetSchema.UpdateDescriptorSets();
                }
            }
        }
    }
}

void PBRWithShadowPipelineV2::createDisplayPassDescriptorSetLayout() {
    std::vector<VkDescriptorSetLayoutBinding> bindings {};
    
    /////////////////////////////////////////////////////////////////
    // Vertex shader
    /////////////////////////////////////////////////////////////////
    
    {// ModelTransformation
        VkDescriptorSetLayoutBinding layoutBinding {
            .binding = static_cast<uint32_t>(bindings.size()),
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        };
        bindings.emplace_back(layoutBinding);
    }
    {// ViewTransformation
        VkDescriptorSetLayoutBinding layoutBinding {
            .binding = static_cast<uint32_t>(bindings.size()),
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        };
        bindings.emplace_back(layoutBinding);
    }
    {// ProjectionTransform
        VkDescriptorSetLayoutBinding layoutBinding {
            .binding = static_cast<uint32_t>(bindings.size()),
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        };
        bindings.emplace_back(layoutBinding);   
    }
    {// NodeTransformation
        VkDescriptorSetLayoutBinding layoutBinding {};
        layoutBinding.binding = static_cast<uint32_t>(bindings.size());
        layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        layoutBinding.descriptorCount = 1;
        layoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        bindings.emplace_back(layoutBinding);
    }
    {// SkinJoints
        VkDescriptorSetLayoutBinding layoutBinding {};
        layoutBinding.binding = static_cast<uint32_t>(bindings.size());
        layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        layoutBinding.descriptorCount = 1;
        layoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        bindings.emplace_back(layoutBinding);
    }
   
    /////////////////////////////////////////////////////////////////
    // Fragment shader
    /////////////////////////////////////////////////////////////////

    {// Primitive
        VkDescriptorSetLayoutBinding layoutBinding {};
        layoutBinding.binding = static_cast<uint32_t>(bindings.size());
        layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        layoutBinding.descriptorCount = 1;
        layoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        bindings.emplace_back(layoutBinding);
    }
    {// BaseColor
        VkDescriptorSetLayoutBinding layoutBinding {};
        layoutBinding.binding = static_cast<uint32_t>(bindings.size());
        layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        layoutBinding.descriptorCount = 1;
        layoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        bindings.emplace_back(layoutBinding);
    }
    {// Metallic/Roughness
        VkDescriptorSetLayoutBinding layoutBinding {};
        layoutBinding.binding = static_cast<uint32_t>(bindings.size());
        layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        layoutBinding.descriptorCount = 1;
        layoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        bindings.emplace_back(layoutBinding);
    }
    {// Normal
        VkDescriptorSetLayoutBinding layoutBinding {};
        layoutBinding.binding = static_cast<uint32_t>(bindings.size());
        layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        layoutBinding.descriptorCount = 1,
        layoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        bindings.emplace_back(layoutBinding);
    }
    {// Emissive
        VkDescriptorSetLayoutBinding layoutBinding {};
        layoutBinding.binding = static_cast<uint32_t>(bindings.size());;
        layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        layoutBinding.descriptorCount = 1;
        layoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        
        bindings.emplace_back(layoutBinding);
    }
    {// Light/View
        VkDescriptorSetLayoutBinding layoutBinding {};
        layoutBinding.binding = static_cast<uint32_t>(bindings.size());;
        layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        layoutBinding.descriptorCount = 1;
        layoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        
        bindings.emplace_back(layoutBinding);
    }
    {// ShadowMap
        VkDescriptorSetLayoutBinding layoutBinding {};
        layoutBinding.binding = static_cast<uint32_t>(bindings.size());;
        layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        layoutBinding.descriptorCount = 1;
        layoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        
        bindings.emplace_back(layoutBinding);
    }
    mDisplayPassDescriptorSetLayout = RF::CreateDescriptorSetLayout(
        static_cast<uint8_t>(bindings.size()),
        bindings.data()
    );  
}

void PBRWithShadowPipelineV2::createShadowPassDescriptorSetLayout() {
    std::vector<VkDescriptorSetLayoutBinding> bindings {};
    
    /////////////////////////////////////////////////////////////////
    // Vertex shader
    /////////////////////////////////////////////////////////////////

    {// ModelTransformation
        VkDescriptorSetLayoutBinding layoutBinding {
            .binding = static_cast<uint32_t>(bindings.size()),
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        };
        bindings.emplace_back(layoutBinding);
    }
    {// ViewProjectionTransform
        VkDescriptorSetLayoutBinding layoutBinding {
            .binding = static_cast<uint32_t>(bindings.size()),
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        };
        bindings.emplace_back(layoutBinding);
    }
    {// NodeTransformation
        VkDescriptorSetLayoutBinding layoutBinding {
            .binding = static_cast<uint32_t>(bindings.size()),
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        };
        bindings.emplace_back(layoutBinding);
    }
    {// SkinJoints
        VkDescriptorSetLayoutBinding layoutBinding {
            .binding = static_cast<uint32_t>(bindings.size()),
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        };
        bindings.emplace_back(layoutBinding);
    }
    
    /////////////////////////////////////////////////////////////////
    // Fragment shader
    /////////////////////////////////////////////////////////////////

    {// Light
        VkDescriptorSetLayoutBinding layoutBinding {};
        layoutBinding.binding = static_cast<uint32_t>(bindings.size());
        layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        layoutBinding.descriptorCount = 1;
        layoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        
        bindings.emplace_back(layoutBinding);
    }
    mShadowPassDescriptorSetLayout = RF::CreateDescriptorSetLayout(
        static_cast<uint8_t>(bindings.size()),
        bindings.data()
    );  
}

void PBRWithShadowPipelineV2::destroyDescriptorSetLayout() const {
    MFA_VK_VALID_ASSERT(mDisplayPassDescriptorSetLayout);
    RF::DestroyDescriptorSetLayout(mDisplayPassDescriptorSetLayout);

    MFA_VK_VALID_ASSERT(mShadowPassDescriptorSetLayout);
    RF::DestroyDescriptorSetLayout(mShadowPassDescriptorSetLayout);
}

void PBRWithShadowPipelineV2::createDisplayPassPipeline() {
    // Vertex shader
    auto cpuVertexShader = Importer::ImportShaderFromSPV(
        Path::Asset("shaders/pbr_with_shadow_v2/PbrWithShadow.vert.spv").c_str(),
        AssetSystem::Shader::Stage::Vertex, 
        "main"
    );
    MFA_ASSERT(cpuVertexShader.isValid());
    auto gpuVertexShader = RF::CreateShader(cpuVertexShader);
    MFA_ASSERT(gpuVertexShader.valid());
    MFA_DEFER {
        RF::DestroyShader(gpuVertexShader);
        Importer::FreeShader(&cpuVertexShader);
    };
    
    // Fragment shader
    auto cpuFragmentShader = Importer::ImportShaderFromSPV(
        Path::Asset("shaders/pbr_with_shadow_v2/PbrWithShadow.frag.spv").c_str(),
        AssetSystem::Shader::Stage::Fragment,
        "main"
    );
    auto gpuFragmentShader = RF::CreateShader(cpuFragmentShader);
    MFA_ASSERT(cpuFragmentShader.isValid());
    MFA_ASSERT(gpuFragmentShader.valid());
    MFA_DEFER {
        RF::DestroyShader(gpuFragmentShader);
        Importer::FreeShader(&cpuFragmentShader);
    };

    std::vector<RB::GpuShader> shaders {gpuVertexShader, gpuFragmentShader};

    VkVertexInputBindingDescription vertexInputBindingDescription {};
    vertexInputBindingDescription.binding = 0;
    vertexInputBindingDescription.stride = sizeof(AS::MeshVertex);
    vertexInputBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    
    std::vector<VkVertexInputAttributeDescription> inputAttributeDescriptions {};

    {// Position
        VkVertexInputAttributeDescription attributeDescription {};
        attributeDescription.location = static_cast<uint32_t>(inputAttributeDescriptions.size()),
        attributeDescription.binding = 0,
        attributeDescription.format = VK_FORMAT_R32G32B32_SFLOAT,
        attributeDescription.offset = offsetof(AS::MeshVertex, position),   
        
        inputAttributeDescriptions.emplace_back(attributeDescription);
    }
    {// BaseColorUV
        VkVertexInputAttributeDescription attributeDescription {};
        attributeDescription.location = static_cast<uint32_t>(inputAttributeDescriptions.size());
        attributeDescription.binding = 0;
        attributeDescription.format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescription.offset = offsetof(AS::MeshVertex, baseColorUV);

        inputAttributeDescriptions.emplace_back(attributeDescription);
    }
    {// Metallic/Roughness
        VkVertexInputAttributeDescription attributeDescription {};
        attributeDescription.location = static_cast<uint32_t>(inputAttributeDescriptions.size());
        attributeDescription.binding = 0;
        attributeDescription.format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescription.offset = offsetof(AS::MeshVertex, metallicUV); // Metallic and roughness have same uv for gltf files  

        inputAttributeDescriptions.emplace_back(attributeDescription);
    }
    {// NormalMapUV
        VkVertexInputAttributeDescription attributeDescription {};
        attributeDescription.location = static_cast<uint32_t>(inputAttributeDescriptions.size());
        attributeDescription.binding = 0;
        attributeDescription.format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescription.offset = offsetof(AS::MeshVertex, normalMapUV);

        inputAttributeDescriptions.emplace_back(attributeDescription);
    }
    {// Tangent
        VkVertexInputAttributeDescription attributeDescription {};
        attributeDescription.location = static_cast<uint32_t>(inputAttributeDescriptions.size());
        attributeDescription.binding = 0;
        attributeDescription.format = VK_FORMAT_R32G32B32A32_SFLOAT;
        attributeDescription.offset = offsetof(AS::MeshVertex, tangentValue);
        
        inputAttributeDescriptions.emplace_back(attributeDescription);
    }
    {// Normal
        VkVertexInputAttributeDescription attributeDescription {};
        attributeDescription.location = static_cast<uint32_t>(inputAttributeDescriptions.size());
        attributeDescription.binding = 0;
        attributeDescription.format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescription.offset = offsetof(AS::MeshVertex, normalValue);   

        inputAttributeDescriptions.emplace_back(attributeDescription);
    }
    {// EmissionUV
        VkVertexInputAttributeDescription attributeDescription {};
        attributeDescription.location = static_cast<uint32_t>(inputAttributeDescriptions.size());
        attributeDescription.binding = 0;
        attributeDescription.format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescription.offset = offsetof(AS::MeshVertex, emissionUV);
        inputAttributeDescriptions.emplace_back(attributeDescription);
    }
    {// HasSkin
        VkVertexInputAttributeDescription attributeDescription {};
        attributeDescription.location = static_cast<uint32_t>(inputAttributeDescriptions.size());
        attributeDescription.binding = 0;
        attributeDescription.format = VK_FORMAT_R32_SINT;
        attributeDescription.offset = offsetof(AS::MeshVertex, hasSkin); // TODO We should use a primitiveInfo instead
        inputAttributeDescriptions.emplace_back(attributeDescription);
    }
    {// JointIndices
        VkVertexInputAttributeDescription attributeDescription {};
        attributeDescription.location = static_cast<uint32_t>(inputAttributeDescriptions.size());
        attributeDescription.binding = 0;
        attributeDescription.format = VK_FORMAT_R32G32B32A32_SINT;
        attributeDescription.offset = offsetof(AS::MeshVertex, jointIndices);
        inputAttributeDescriptions.emplace_back(attributeDescription);
    }
    {// JointWeights
        VkVertexInputAttributeDescription attributeDescription {};
        attributeDescription.location = static_cast<uint32_t>(inputAttributeDescriptions.size());
        attributeDescription.binding = 0;
        attributeDescription.format = VK_FORMAT_R32G32B32A32_SFLOAT;
        attributeDescription.offset = offsetof(AS::MeshVertex, jointWeights);
        inputAttributeDescriptions.emplace_back(attributeDescription);
    }
    MFA_ASSERT(mDisplayPassPipeline.isValid() == false);

    RB::CreateGraphicPipelineOptions options {};
    options.rasterizationSamples = RF::GetMaxSamplesCount();

    mDisplayPassPipeline = RF::CreateDrawPipeline(
        RF::GetDisplayRenderPass()->GetVkRenderPass(),
        static_cast<uint8_t>(shaders.size()), 
        shaders.data(),
        1,
        &mDisplayPassDescriptorSetLayout,
        vertexInputBindingDescription,
        static_cast<uint8_t>(inputAttributeDescriptions.size()),
        inputAttributeDescriptions.data(),
        options
    );
}

void PBRWithShadowPipelineV2::createShadowPassPipeline() {
     // Vertex shader
    auto cpuVertexShader = Importer::ImportShaderFromSPV(
        Path::Asset("shaders/pbr_with_shadow_v2/OffScreenShadow.vert.spv").c_str(),
        AssetSystem::Shader::Stage::Vertex, 
        "main"
    );
    MFA_ASSERT(cpuVertexShader.isValid());
    auto gpuVertexShader = RF::CreateShader(cpuVertexShader);
    MFA_ASSERT(gpuVertexShader.valid());
    MFA_DEFER {
        RF::DestroyShader(gpuVertexShader);
        Importer::FreeShader(&cpuVertexShader);
    };

    // Fragment shader
    auto cpuFragmentShader = Importer::ImportShaderFromSPV(
        Path::Asset("shaders/pbr_with_shadow_v2/OffScreenShadow.frag.spv").c_str(),
        AssetSystem::Shader::Stage::Fragment,
        "main"
    );
    auto gpuFragmentShader = RF::CreateShader(cpuFragmentShader);
    MFA_ASSERT(cpuFragmentShader.isValid());
    MFA_ASSERT(gpuFragmentShader.valid());
    MFA_DEFER {
        RF::DestroyShader(gpuFragmentShader);
        Importer::FreeShader(&cpuFragmentShader);
    };

    std::vector<RB::GpuShader> shaders {gpuVertexShader, gpuFragmentShader};

    VkVertexInputBindingDescription vertexInputBindingDescription {};
    vertexInputBindingDescription.binding = 0;
    vertexInputBindingDescription.stride = sizeof(AS::MeshVertex);
    vertexInputBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    
    std::vector<VkVertexInputAttributeDescription> inputAttributeDescriptions {};
    {// Position
        VkVertexInputAttributeDescription attributeDescription {};
        attributeDescription.location = static_cast<uint32_t>(inputAttributeDescriptions.size());
        attributeDescription.binding = 0;
        attributeDescription.format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescription.offset = offsetof(AS::MeshVertex, position);   
        
        inputAttributeDescriptions.emplace_back(attributeDescription);
    }
    {// HasSkin
        VkVertexInputAttributeDescription attributeDescription {};
        attributeDescription.location = static_cast<uint32_t>(inputAttributeDescriptions.size());
        attributeDescription.binding = 0;
        attributeDescription.format = VK_FORMAT_R32_SINT;
        attributeDescription.offset = offsetof(AS::MeshVertex, hasSkin); // TODO We should use a primitiveInfo instead
        inputAttributeDescriptions.emplace_back(attributeDescription);
    }
    {// JointIndices
        VkVertexInputAttributeDescription attributeDescription {};
        attributeDescription.location = static_cast<uint32_t>(inputAttributeDescriptions.size());
        attributeDescription.binding = 0;
        attributeDescription.format = VK_FORMAT_R32G32B32A32_SINT;
        attributeDescription.offset = offsetof(AS::MeshVertex, jointIndices);
        inputAttributeDescriptions.emplace_back(attributeDescription);
    }
    {// JointWeights
        VkVertexInputAttributeDescription attributeDescription {};
        attributeDescription.location = static_cast<uint32_t>(inputAttributeDescriptions.size());
        attributeDescription.binding = 0;
        attributeDescription.format = VK_FORMAT_R32G32B32A32_SFLOAT;
        attributeDescription.offset = offsetof(AS::MeshVertex, jointWeights);
        inputAttributeDescriptions.emplace_back(attributeDescription);
    }

    std::vector<VkPushConstantRange> pushConstantRanges {};
    {// FaceIndex  
        VkPushConstantRange pushConstantRange {
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
            .offset = 0,
            .size = sizeof(ShadowPushConstants),
        };
        pushConstantRanges.emplace_back(pushConstantRange);
    }  
    RB::CreateGraphicPipelineOptions graphicPipelineOptions {};
    graphicPipelineOptions.cullMode = VK_CULL_MODE_NONE;
    graphicPipelineOptions.pushConstantRanges = pushConstantRanges.data();
    // TODO Probably we need to make pushConstantsRangeCount uint32_t
    graphicPipelineOptions.pushConstantsRangeCount = static_cast<uint8_t>(pushConstantRanges.size());
    
    MFA_ASSERT(mShadowPassPipeline.isValid() == false);
    mShadowPassPipeline = RF::CreateDrawPipeline(
        mShadowRenderPass.GetVkRenderPass(),
        static_cast<uint8_t>(shaders.size()), 
        shaders.data(),
        1,
        &mShadowPassDescriptorSetLayout,
        vertexInputBindingDescription,
        static_cast<uint8_t>(inputAttributeDescriptions.size()),
        inputAttributeDescriptions.data(),
        graphicPipelineOptions
    );
}

void PBRWithShadowPipelineV2::destroyPipeline() {
    MFA_ASSERT(mDisplayPassPipeline.isValid());
    RF::DestroyDrawPipeline(mDisplayPassPipeline);

    MFA_ASSERT(mShadowPassPipeline.isValid());
    RF::DestroyDrawPipeline(mShadowPassPipeline);
}

void PBRWithShadowPipelineV2::createUniformBuffers() {
    mErrorBuffer = RF::CreateUniformBuffer(sizeof(DrawableObject::JointTransformBuffer), 1);

    mDisplayViewBuffer = RF::CreateUniformBuffer(sizeof(DisplayViewData), 1);
    mDisplayProjectionBuffer = RF::CreateUniformBuffer(sizeof(DisplayProjectionData), 1);
    mDisplayLightAndCameraBuffer = RF::CreateUniformBuffer(sizeof(DisplayLightAndCameraData), 1);
    
    mShadowViewProjectionBuffer = RF::CreateUniformBuffer(sizeof(ShadowViewProjectionData), 1);
    mShadowLightBuffer = RF::CreateUniformBuffer(sizeof(ShadowLightData), 1);
}

void PBRWithShadowPipelineV2::destroyUniformBuffers() {
    RF::DestroyUniformBuffer(mErrorBuffer);

    RF::DestroyUniformBuffer(mDisplayViewBuffer);
    RF::DestroyUniformBuffer(mDisplayLightAndCameraBuffer);
    RF::DestroyUniformBuffer(mDisplayProjectionBuffer);
    
    RF::DestroyUniformBuffer(mShadowViewProjectionBuffer);
    RF::DestroyUniformBuffer(mShadowLightBuffer);

    for (const auto & [id, drawableObject] : mDrawableObjects) {
        MFA_ASSERT(drawableObject != nullptr);
        drawableObject->DeleteUniformBuffers();
    }
    mDrawableObjects.clear();
}

}
