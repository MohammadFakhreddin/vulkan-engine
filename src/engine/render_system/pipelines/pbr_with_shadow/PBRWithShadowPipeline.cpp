#include "PBRWithShadowPipeline.hpp"

#include "engine/BedrockPath.hpp"
#include "engine/camera/FirstPersonCamera.hpp"
#include "engine/render_system/render_passes/DisplayRenderPass.hpp"
#include "tools/Importer.hpp"
#include "engine/BedrockMatrix.hpp"

namespace MFA {

// HLSL Geometry shader example:
// https://stackoverflow.com/questions/29124668/rendering-to-a-full-3d-render-target-in-one-pass

void PBRWithShadowPipeline::Init(
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
}

void PBRWithShadowPipeline::Shutdown() {
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

void PBRWithShadowPipeline::Update(
    RF::DrawPass & drawPass, 
    float const deltaTime, 
    uint32_t const idsCount, 
    DrawableObjectId * ids
) {
    
    if (mNeedToUpdateDisplayLightBuffer) {
        mNeedToUpdateDisplayLightBuffer = false;

        DisplayLightViewData displayLightViewData {
            .projectFarToNearDistance = mProjectionFarToNearDistance
        };
        Copy<3>(displayLightViewData.cameraPosition, mCameraPosition);
        Copy<3>(displayLightViewData.lightColor, mLightColor);
        Copy<3>(displayLightViewData.lightPosition, mLightPosition);
        RF::UpdateUniformBuffer(mDisplayLightViewBuffer.buffers[0], CBlobAliasOf(displayLightViewData));
    }

    if (mNeedToUpdateShadowLightBuffer) {
        mNeedToUpdateShadowLightBuffer = false;
    
        ShadowLightData shadowLightData {
            .projectionMatrixDistance = mProjectionFarToNearDistance
        };
        Copy<3>(shadowLightData.lightPosition, mLightPosition);
        RF::UpdateUniformBuffer(mShadowLightBuffer.buffers[0], CBlobAliasOf(shadowLightData));
    }

    if (mNeedToUpdateShadowProjectionBuffer) {
        mNeedToUpdateShadowProjectionBuffer = false;

        auto const lightPositionVector = glm::vec3(mLightPosition[0], mLightPosition[1], mLightPosition[2]);

        ShadowMatricesData shadowMatricesData {};

        Matrix4X4Float::ConvertGmToCells(
            mShadowProjection * glm::lookAt(
                lightPositionVector, 
                lightPositionVector + glm::vec3( 1.0, 0.0, 0.0), 
                glm::vec3(0.0,-1.0, 0.0)
            ), 
            shadowMatricesData.viewMatrices[0]
        );

        Matrix4X4Float::ConvertGmToCells(
            mShadowProjection * glm::lookAt(
                lightPositionVector, 
                lightPositionVector + glm::vec3(-1.0, 0.0, 0.0), 
                glm::vec3(0.0,-1.0, 0.0)
            ), 
            shadowMatricesData.viewMatrices[1]
        );

        Matrix4X4Float::ConvertGmToCells(
            mShadowProjection * glm::lookAt(
                lightPositionVector, 
                lightPositionVector + glm::vec3( 0.0, 1.0, 0.0), 
                glm::vec3(0.0, 0.0, 1.0)
            ), 
            shadowMatricesData.viewMatrices[2]
        );

        Matrix4X4Float::ConvertGmToCells(
            mShadowProjection * glm::lookAt(
                lightPositionVector, 
                lightPositionVector + glm::vec3( 0.0,-1.0, 0.0), 
                glm::vec3(0.0, 0.0,-1.0)
            ), 
            shadowMatricesData.viewMatrices[3]
        );

        Matrix4X4Float::ConvertGmToCells(
            mShadowProjection * glm::lookAt(
                lightPositionVector, 
                lightPositionVector + glm::vec3( 0.0, 0.0, 1.0), 
                glm::vec3(0.0,-1.0, 0.0)
            ), 
            shadowMatricesData.viewMatrices[4]
        );

        Matrix4X4Float::ConvertGmToCells(
            mShadowProjection * glm::lookAt(
                lightPositionVector, 
                lightPositionVector + glm::vec3( 0.0, 0.0,-1.0), 
                glm::vec3(0.0,-1.0, 0.0)
            ), 
            shadowMatricesData.viewMatrices[5]
        );


        RF::UpdateUniformBuffer(mShadowMatricesBuffer.buffers[0], CBlobAliasOf(shadowMatricesData));
    }

    mShadowRenderPass.BeginRenderPass(drawPass);

    RF::BindDrawPipeline(drawPass, mShadowPassPipeline);

    for (uint32_t i = 0; i < idsCount; ++i) {
        auto const findResult = mShadowPassDrawableObjects.find(ids[i]);
        if (findResult != mShadowPassDrawableObjects.end()) {
            auto * drawableObject = findResult->second.get();
            MFA_ASSERT(drawableObject != nullptr);
            drawableObject->update(deltaTime);
            drawableObject->draw(drawPass);
        }
    }

    mShadowRenderPass.EndRenderPass(drawPass);
}

void PBRWithShadowPipeline::Render(
    RF::DrawPass & drawPass, 
    float const deltaTime, 
    uint32_t const idsCount, 
    DrawableObjectId * ids
) {
    RF::BindDrawPipeline(drawPass, mDisplayPassPipeline);
    
    for (uint32_t i = 0; i < idsCount; ++i) {
        auto const findResult = mDisplayPassDrawableObjects.find(ids[i]);
        if (findResult != mDisplayPassDrawableObjects.end()) {
            auto * drawableObject = findResult->second.get();
            MFA_ASSERT(drawableObject != nullptr);
            drawableObject->update(deltaTime);
            drawableObject->draw(drawPass);
        }
    }
}

DrawableObjectId PBRWithShadowPipeline::AddGpuModel(RF::GpuModel & gpuModel) {
    MFA_ASSERT(gpuModel.valid == true);

    auto const drawableObjectId = mNextDrawableObjectId ++;

    CreateDisplayPassDrawableObject(gpuModel, drawableObjectId);
    CreateShadowPassDrawableObject(gpuModel, drawableObjectId);
    
    return drawableObjectId;
}

bool PBRWithShadowPipeline::RemoveGpuModel(DrawableObjectId const drawableObjectId) {
    auto deleteCount = mDisplayPassDrawableObjects.erase(drawableObjectId);  // Unique ptr should be deleted correctly
    MFA_ASSERT(deleteCount == 1);
    deleteCount = mShadowPassDrawableObjects.erase(drawableObjectId);
    MFA_ASSERT(deleteCount == 1);
    return deleteCount;
}

bool PBRWithShadowPipeline::UpdateViewProjectionBuffer(
    DrawableObjectId const drawableObjectId, 
    ModelViewProjectionData const & viewProjectionData
) {
    // We should share buffers
    {// Display
        auto const findResult = mDisplayPassDrawableObjects.find(drawableObjectId);
        if (findResult == mDisplayPassDrawableObjects.end()) {
            MFA_ASSERT(false);
            return false;
        }
        auto * drawableObject = findResult->second.get();
        MFA_ASSERT(drawableObject != nullptr);

        drawableObject->updateUniformBuffer(
            "ViewProjection", 
            CBlobAliasOf(viewProjectionData)
        );
    }
    {// Shadow
        auto const findResult = mShadowPassDrawableObjects.find(drawableObjectId);
        if (findResult == mShadowPassDrawableObjects.end()) {
            MFA_ASSERT(false);
            return false;
        }
        auto * drawableObject = findResult->second.get();
        MFA_ASSERT(drawableObject != nullptr);

        drawableObject->updateUniformBuffer(
            "ViewProjection", 
            CBlobAliasOf(viewProjectionData)
        );
    }
    return true;
}

void PBRWithShadowPipeline::UpdateLightPosition(float lightPosition[3]) {
    if (IsEqual<3>(mLightPosition, lightPosition) == false) {
        Copy<3>(mLightPosition, lightPosition);
        mNeedToUpdateDisplayLightBuffer = true;
        mNeedToUpdateShadowLightBuffer = true;
        mNeedToUpdateShadowProjectionBuffer = true;
    }
}

void PBRWithShadowPipeline::UpdateCameraPosition(float cameraPosition[3]) {
    if (IsEqual<3>(mCameraPosition, cameraPosition) == false) {
        Copy<3>(mCameraPosition, cameraPosition);
        mNeedToUpdateDisplayLightBuffer = true;
    }
}

void PBRWithShadowPipeline::UpdateLightColor(float lightColor[3]) {
    if (IsEqual<3>(mLightColor, lightColor) == false) {
        Copy<3>(mLightColor, lightColor);
        mNeedToUpdateDisplayLightBuffer = true;
    }
}

DrawableObject * PBRWithShadowPipeline::GetDrawableById(DrawableObjectId const objectId) {
    auto const findResult = mDisplayPassDrawableObjects.find(objectId);
    if (findResult != mDisplayPassDrawableObjects.end()) {
        return findResult->second.get();
    }
    return nullptr;
}

void PBRWithShadowPipeline::CreateDisplayPassDrawableObject(RF::GpuModel & gpuModel, DrawableObjectId drawableObjectId) {
    auto * drawableObject = new DrawableObject(gpuModel, mDisplayPassDescriptorSetLayout);
    MFA_ASSERT(mDisplayPassDrawableObjects.find(drawableObjectId) == mDisplayPassDrawableObjects.end());
    mDisplayPassDrawableObjects[drawableObjectId] = std::unique_ptr<DrawableObject>(drawableObject);

    const auto * primitiveInfoBuffer = drawableObject->createMultipleUniformBuffer(
        "PrimitiveInfo", 
        sizeof(PrimitiveInfo), 
        drawableObject->getDescriptorSetCount()
    );
    MFA_ASSERT(primitiveInfoBuffer != nullptr);

    const auto * modelTransformBuffer = drawableObject->createUniformBuffer(
        "ViewProjection", 
        sizeof(ModelViewProjectionData)
    );
    MFA_ASSERT(modelTransformBuffer != nullptr);

    const auto & nodeTransformBuffer = drawableObject->getNodeTransformBuffer();

    auto const & textures = drawableObject->getModel()->textures;

    auto const & mesh = drawableObject->getModel()->model.mesh;

    for (uint32_t nodeIndex = 0; nodeIndex < mesh.GetNodesCount(); ++nodeIndex) {// Updating descriptor sets
        auto const & node = mesh.GetNodeByIndex(nodeIndex);
        if (node.hasSubMesh()) {
            auto const & subMesh = mesh.GetSubMeshByIndex(node.subMeshIndex);
            if (subMesh.primitives.empty() == false) {
                for (auto const & primitive : subMesh.primitives) {
                    MFA_ASSERT(primitive.uniqueId >= 0);
                    auto descriptorSet = drawableObject->getDescriptorSetByPrimitiveUniqueId(primitive.uniqueId);
                    MFA_VK_VALID_ASSERT(descriptorSet);

                    DescriptorSetSchema descriptorSetSchema {descriptorSet};

                    {// ModelTransform
                        VkDescriptorBufferInfo modelTransformBufferInfo {};
                        modelTransformBufferInfo.buffer = modelTransformBuffer->buffers[0].buffer;
                        modelTransformBufferInfo.offset = 0;
                        modelTransformBufferInfo.range = modelTransformBuffer->bufferSize;

                        descriptorSetSchema.AddUniformBuffer(modelTransformBufferInfo);
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
                            ? drawableObject->getSkinTransformBuffer(node.skinBufferIndex)
                            : mErrorBuffer;

                        VkDescriptorBufferInfo skinTransformBufferInfo {};
                        skinTransformBufferInfo.buffer = skinBuffer.buffers[0].buffer;
                        skinTransformBufferInfo.offset = 0;
                        skinTransformBufferInfo.range = skinBuffer.bufferSize;

                        descriptorSetSchema.AddUniformBuffer(skinTransformBufferInfo);
                    }

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
                        lightViewBufferInfo.buffer = mDisplayLightViewBuffer.buffers[0].buffer;
                        lightViewBufferInfo.offset = 0;
                        lightViewBufferInfo.range = mDisplayLightViewBuffer.bufferSize;

                        descriptorSetSchema.AddUniformBuffer(lightViewBufferInfo);
                    }

                    {// ShadowMap
                        VkDescriptorImageInfo shadowMapImageInfo {};
                        shadowMapImageInfo.sampler = mSamplerGroup->sampler;
                        shadowMapImageInfo.imageView = mShadowRenderPass.GetDepthImageGroup().imageView;
                        shadowMapImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

                        descriptorSetSchema.AddCombinedImageSampler(shadowMapImageInfo);
                    }

                    descriptorSetSchema.UpdateDescriptorSets();
                }
            }
        }
    }
}

void PBRWithShadowPipeline::CreateShadowPassDrawableObject(RF::GpuModel & gpuModel, DrawableObjectId drawableObjectId) {
    auto * drawableObject = new DrawableObject(gpuModel, mShadowPassDescriptorSetLayout);
    MFA_ASSERT(mShadowPassDrawableObjects.find(drawableObjectId) == mShadowPassDrawableObjects.end());
    mShadowPassDrawableObjects[drawableObjectId] = std::unique_ptr<DrawableObject>(drawableObject);

    // TODO We need a better drawable object or maybe we can use push constants instead

    const auto * modelTransformBuffer = drawableObject->createUniformBuffer(
        "ViewProjection", 
        sizeof(ModelViewProjectionData)
    );
    MFA_ASSERT(modelTransformBuffer != nullptr);

    const auto & nodeTransformBuffer = drawableObject->getNodeTransformBuffer();

    auto const & mesh = drawableObject->getModel()->model.mesh;

    for (uint32_t nodeIndex = 0; nodeIndex < mesh.GetNodesCount(); ++nodeIndex) {// Updating descriptor sets
        auto const & node = mesh.GetNodeByIndex(nodeIndex);
        if (node.hasSubMesh()) {
            auto const & subMesh = mesh.GetSubMeshByIndex(node.subMeshIndex);
            if (subMesh.primitives.empty() == false) {
                for (auto const & primitive : subMesh.primitives) {
                    MFA_ASSERT(primitive.uniqueId >= 0);
                    // TODO Remove this shit and use push constants instead
                    auto descriptorSet = drawableObject->getDescriptorSetByPrimitiveUniqueId(primitive.uniqueId);
                    MFA_VK_VALID_ASSERT(descriptorSet);

                    DescriptorSetSchema descriptorSetSchema {descriptorSet};

                    // Vertex shader
                    {// ModelTransform
                        VkDescriptorBufferInfo modelTransformBufferInfo {};
                        modelTransformBufferInfo.buffer = modelTransformBuffer->buffers[0].buffer;
                        modelTransformBufferInfo.offset = 0;
                        modelTransformBufferInfo.range = modelTransformBuffer->bufferSize;

                        descriptorSetSchema.AddUniformBuffer(modelTransformBufferInfo);
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
                            ? drawableObject->getSkinTransformBuffer(node.skinBufferIndex)
                            : mErrorBuffer;

                        VkDescriptorBufferInfo skinTransformBufferInfo {};
                        skinTransformBufferInfo.buffer = skinBuffer.buffers[0].buffer;
                        skinTransformBufferInfo.offset = 0;
                        skinTransformBufferInfo.range = skinBuffer.bufferSize;

                        descriptorSetSchema.AddUniformBuffer(skinTransformBufferInfo);
                    }

                    // Geometry shader
                    {// ShadowMatrices
                        VkDescriptorBufferInfo bufferInfo {};
                        bufferInfo.buffer = mShadowMatricesBuffer.buffers[0].buffer;
                        bufferInfo.offset = 0;
                        bufferInfo.range = mShadowMatricesBuffer.bufferSize;

                        descriptorSetSchema.AddUniformBuffer(bufferInfo);
                    }

                    // Fragment shader
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

void PBRWithShadowPipeline::createDisplayPassDescriptorSetLayout() {
    std::vector<VkDescriptorSetLayoutBinding> bindings {};
    // Vertex shader
    {// ModelTransformation
        VkDescriptorSetLayoutBinding layoutBinding {};
        layoutBinding.binding = static_cast<uint32_t>(bindings.size());
        layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        layoutBinding.descriptorCount = 1;
        layoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
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
    // Fragment shader
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

void PBRWithShadowPipeline::createShadowPassDescriptorSetLayout() {
    std::vector<VkDescriptorSetLayoutBinding> bindings {};
    // Vertex shader
    {// ModelTransformation
        VkDescriptorSetLayoutBinding layoutBinding {};
        layoutBinding.binding = static_cast<uint32_t>(bindings.size());
        layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        layoutBinding.descriptorCount = 1;
        layoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
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
    // Geometry shader
    {// ShadowMatrices
        VkDescriptorSetLayoutBinding layoutBinding {};
        layoutBinding.binding = static_cast<uint32_t>(bindings.size());
        layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        layoutBinding.descriptorCount = 1;
        layoutBinding.stageFlags = VK_SHADER_STAGE_GEOMETRY_BIT;
        bindings.emplace_back(layoutBinding);
    }
    // Fragment shader
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

void PBRWithShadowPipeline::destroyDescriptorSetLayout() const {
    MFA_VK_VALID_ASSERT(mDisplayPassDescriptorSetLayout);
    RF::DestroyDescriptorSetLayout(mDisplayPassDescriptorSetLayout);

    MFA_VK_VALID_ASSERT(mShadowPassDescriptorSetLayout);
    RF::DestroyDescriptorSetLayout(mShadowPassDescriptorSetLayout);
}

void PBRWithShadowPipeline::createDisplayPassPipeline() {
    // Vertex shader
    auto cpuVertexShader = Importer::ImportShaderFromSPV(
        Path::Asset("shaders/pbr_with_shadow/PbrWithShadow.vert.spv").c_str(),
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
        Path::Asset("shaders/pbr_with_shadow/PbrWithShadow.frag.spv").c_str(),
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
    mDisplayPassPipeline = RF::CreateBasicDrawPipeline(
        RF::GetDisplayRenderPass()->GetVkRenderPass(),
        static_cast<uint8_t>(shaders.size()), 
        shaders.data(),
        1,
        &mDisplayPassDescriptorSetLayout,
        vertexInputBindingDescription,
        static_cast<uint8_t>(inputAttributeDescriptions.size()),
        inputAttributeDescriptions.data()
    );
}

void PBRWithShadowPipeline::createShadowPassPipeline() {
     // Vertex shader
    auto cpuVertexShader = Importer::ImportShaderFromSPV(
        Path::Asset("shaders/pbr_with_shadow/OffScreenShadow.vert.spv").c_str(),
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

    // Geometry shader
    auto cpuGeometryShader = Importer::ImportShaderFromSPV(
        Path::Asset("shaders/pbr_with_shadow/OffScreenShadow.geom.spv").c_str(),
        AssetSystem::Shader::Stage::Geometry,
        "main"
    );
    auto gpuGeometryShader = RF::CreateShader(cpuGeometryShader);
    MFA_ASSERT(cpuGeometryShader.isValid());
    MFA_ASSERT(gpuGeometryShader.valid());
    MFA_DEFER {
        RF::DestroyShader(gpuGeometryShader);
        Importer::FreeShader(&cpuGeometryShader);
    };

    // Fragment shader
    auto cpuFragmentShader = Importer::ImportShaderFromSPV(
        Path::Asset("shaders/pbr_with_shadow/OffScreenShadow.frag.spv").c_str(),
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

    std::vector<RB::GpuShader> shaders {gpuVertexShader, gpuGeometryShader, gpuFragmentShader};

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

    RB::CreateGraphicPipelineOptions graphicPipelineOptions {};
    //graphicPipelineOptions.colorBlendAttachments.blendEnable = VK_TRUE;
    //graphicPipelineOptions.colorBlendAttachments.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    //graphicPipelineOptions.colorBlendAttachments.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    //graphicPipelineOptions.colorBlendAttachments.colorBlendOp = VK_BLEND_OP_ADD;
    //graphicPipelineOptions.colorBlendAttachments.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    //graphicPipelineOptions.colorBlendAttachments.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    //graphicPipelineOptions.colorBlendAttachments.alphaBlendOp = VK_BLEND_OP_ADD;
    //graphicPipelineOptions.colorBlendAttachments.colorWriteMask = VK_COLOR_COMPONENT_R_BIT;
    //graphicPipelineOptions.useStaticViewportAndScissor = false;

    //graphicPipelineOptions.depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    //graphicPipelineOptions.depthStencil.depthTestEnable = VK_TRUE;
    //graphicPipelineOptions.depthStencil.depthWriteEnable = VK_TRUE;
    //graphicPipelineOptions.depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    //graphicPipelineOptions.depthStencil.depthBoundsTestEnable = VK_FALSE;
    //graphicPipelineOptions.depthStencil.stencilTestEnable = VK_FALSE;
    
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

void PBRWithShadowPipeline::destroyPipeline() {
    MFA_ASSERT(mDisplayPassPipeline.isValid());
    RF::DestroyDrawPipeline(mDisplayPassPipeline);

    MFA_ASSERT(mShadowPassPipeline.isValid());
    RF::DestroyDrawPipeline(mShadowPassPipeline);
}

void PBRWithShadowPipeline::createUniformBuffers() {
    mDisplayLightViewBuffer = RF::CreateUniformBuffer(sizeof(DisplayLightViewData), 1);
    mErrorBuffer = RF::CreateUniformBuffer(sizeof(DrawableObject::JointTransformBuffer), 1);

    mShadowMatricesBuffer = RF::CreateUniformBuffer(sizeof(ShadowMatricesData), 1);
    mShadowLightBuffer = RF::CreateUniformBuffer(sizeof(ShadowLightData), 1);
}

void PBRWithShadowPipeline::destroyUniformBuffers() {
    RF::DestroyUniformBuffer(mErrorBuffer);
    RF::DestroyUniformBuffer(mDisplayLightViewBuffer);
    RF::DestroyUniformBuffer(mShadowMatricesBuffer);
    RF::DestroyUniformBuffer(mShadowLightBuffer);

    for (const auto & [id, drawableObject] : mDisplayPassDrawableObjects) {
        MFA_ASSERT(drawableObject != nullptr);
        drawableObject->deleteUniformBuffers();
    }
    mDisplayPassDrawableObjects.clear();

    for (const auto & [id, drawableObject] : mShadowPassDrawableObjects) {
        MFA_ASSERT(drawableObject != nullptr);
        drawableObject->deleteUniformBuffers();
    }
    mShadowPassDrawableObjects.clear();

}

}