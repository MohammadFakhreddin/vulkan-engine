#include "PointLightPipeline.hpp"

#include "tools/Importer.hpp"
#include "engine/BedrockPath.hpp"
#include "engine/render_system/render_passes/DisplayRenderPass.hpp"

namespace MFA {

namespace AS = AssetSystem;

//-------------------------------------------------------------------------------------------------

PointLightPipeline::~PointLightPipeline() {
    if (mIsInitialized) {
        Shutdown();
    }
}

//-------------------------------------------------------------------------------------------------

void PointLightPipeline::Init() {
    if (mIsInitialized == true) {
        MFA_ASSERT(false);
        return;
    }
    mIsInitialized = true;

    createUniformBuffers();
    createDescriptorSetLayout();
    createPipeline();
    createDescriptorSets();
}

//-------------------------------------------------------------------------------------------------

void PointLightPipeline::Shutdown() {
    if (mIsInitialized == false) {
        MFA_ASSERT(false);
        return;
    }
    mIsInitialized = false;

    destroyPipeline();
    destroyDescriptorSetLayout();
    destroyDrawableObjects();
    destroyUniformBuffers();
}

//-------------------------------------------------------------------------------------------------

DrawableObjectId PointLightPipeline::AddGpuModel(RF::GpuModel & gpuModel) {
    MFA_ASSERT(gpuModel.valid == true);

    auto const drawableId = mNextDrawableId++;

    auto drawableObject = std::make_unique<DrawableObject>(gpuModel);
    MFA_ASSERT(mDrawableObjects.find(drawableId) == mDrawableObjects.end());
    drawableObject->AllocStorage("PointLight", sizeof(DrawableStorageData));
    mDrawableObjects[drawableId] = std::move(drawableObject);
    
    return drawableId;
}

//-------------------------------------------------------------------------------------------------

bool PointLightPipeline::RemoveGpuModel(DrawableObjectId const drawableObjectId) {
    auto const deleteCount = mDrawableObjects.erase(drawableObjectId);  // Unique ptr should be deleted correctly
    MFA_ASSERT(deleteCount == 1);
    return deleteCount;
}

//-------------------------------------------------------------------------------------------------

void PointLightPipeline::PreRender(
    RF::DrawPass & drawPass, 
    float const deltaTimeInSec, 
    uint32_t const idsCount, 
    DrawableObjectId * ids
) {
    for (uint32_t i = 0; i < idsCount; ++i) {
        auto const findResult = mDrawableObjects.find(ids[i]);
        if (findResult != mDrawableObjects.end()) {
            auto * drawableObject = findResult->second.get();
            MFA_ASSERT(drawableObject != nullptr);
            drawableObject->Update(deltaTimeInSec, drawPass);
        }
    }
    updateViewProjectionBuffer(drawPass);
}

//-------------------------------------------------------------------------------------------------

void PointLightPipeline::Render(
    RF::DrawPass & drawPass,
    float const deltaTimeInSec,
    uint32_t const idsCount, 
    DrawableObjectId * ids
) {
    RF::BindDrawPipeline(drawPass, mDrawPipeline);
    RF::BindDescriptorSet(drawPass, mDescriptorSetGroup.descriptorSets[drawPass.frameIndex]);

    for (uint32_t i = 0; i < idsCount; ++i) {
        auto const findResult = mDrawableObjects.find(ids[i]);
        if (findResult != mDrawableObjects.end()) {
            auto * drawableObject = findResult->second.get();
            MFA_ASSERT(drawableObject != nullptr);
            drawableObject->Draw(drawPass, [&drawPass, &drawableObject](AS::MeshPrimitive const & primitive, DrawableObject::Node const & node)-> void {
                auto const & storageData = drawableObject->GetStorage("PointLight").as<DrawableStorageData>()[0];

                PushConstants pushConstants {};
                Copy<3>(pushConstants.baseColorFactor, storageData.color);
                Matrix4X4Float::ConvertGmToCells(node.cachedModelTransform, pushConstants.model);
                        
                RF::PushConstants(
                    drawPass,
                    VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                    0,
                    CBlobAliasOf(pushConstants)   
                );
            });
        }
    }
}

//-------------------------------------------------------------------------------------------------

void PointLightPipeline::UpdateLightTransform(uint32_t const id, float lightTransform[16]) {
    auto findResult = mDrawableObjects.find(id);   
    MFA_ASSERT(findResult != mDrawableObjects.end());
    findResult->second->UpdateModelTransform(lightTransform);
}

//-------------------------------------------------------------------------------------------------

void PointLightPipeline::UpdateLightColor(uint32_t const id, float lightColor[3]) {
    auto findResult = mDrawableObjects.find(id);
    auto & storageData = findResult->second->GetStorage("PointLight").as<DrawableStorageData>()[0];
    Copy<3>(storageData.color, lightColor);
}

//-------------------------------------------------------------------------------------------------

void PointLightPipeline::UpdateCameraView(float cameraTransform[16]) {
    if (Matrix4X4Float::IsEqual(mCameraTransform, cameraTransform) == false) {
        Matrix4X4Float::ConvertCellsToMat4(cameraTransform, mCameraTransform);
        mViewProjectionBufferDirtyCounter = RF::GetMaxFramesPerFlight();
    }
}

//-------------------------------------------------------------------------------------------------

void PointLightPipeline::UpdateCameraProjection(float cameraProjection[16]) {
    if (Matrix4X4Float::IsEqual(mCameraProjection, cameraProjection) == false) {
        Matrix4X4Float::ConvertCellsToMat4(cameraProjection, mCameraProjection);
        mViewProjectionBufferDirtyCounter = RF::GetMaxFramesPerFlight();
    }
}

//-------------------------------------------------------------------------------------------------

void PointLightPipeline::updateViewProjectionBuffer(RF::DrawPass const & drawPass) {
    if (mViewProjectionBufferDirtyCounter <= 0) {
        return;
    }
    if (mViewProjectionBufferDirtyCounter == RF::GetMaxFramesPerFlight()) {
        glm::mat4 const viewProjection = mCameraProjection * mCameraTransform;
        Matrix4X4Float::ConvertGmToCells(viewProjection, mViewProjectionData.viewProjection);
    }
    --mViewProjectionBufferDirtyCounter;
    MFA_ASSERT(mViewProjectionBufferDirtyCounter >= 0);
    
    RF::UpdateUniformBuffer(mViewProjectionBuffers.buffers[drawPass.frameIndex], CBlobAliasOf(mViewProjectionData));
}

//-------------------------------------------------------------------------------------------------

void PointLightPipeline::createUniformBuffers() {
    mViewProjectionBuffers = RF::CreateUniformBuffer(sizeof(ViewProjectionData), RF::GetMaxFramesPerFlight());
    for (uint32_t i = 0; i < RF::GetMaxFramesPerFlight(); ++i) {
        RF::UpdateUniformBuffer(mViewProjectionBuffers.buffers[i], CBlobAliasOf(mViewProjectionData));
    }
}

//-------------------------------------------------------------------------------------------------

void PointLightPipeline::destroyUniformBuffers() {
    RF::DestroyUniformBuffer(mViewProjectionBuffers);
}

//-------------------------------------------------------------------------------------------------

void PointLightPipeline::createDescriptorSetLayout() {
    std::vector<VkDescriptorSetLayoutBinding> bindings {};
    {// ModelViewProjection
        VkDescriptorSetLayoutBinding layoutBinding {
            .binding = static_cast<uint32_t>(bindings.size()),
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT
        };
        bindings.emplace_back(layoutBinding);
    }
    MFA_VK_INVALID_ASSERT(mDescriptorSetLayout);
    mDescriptorSetLayout = RF::CreateDescriptorSetLayout(
        static_cast<uint8_t>(bindings.size()),
        bindings.data()
    );
}

//-------------------------------------------------------------------------------------------------

void PointLightPipeline::destroyDescriptorSetLayout() {
    MFA_VK_VALID_ASSERT(mDescriptorSetLayout);
    RF::DestroyDescriptorSetLayout(mDescriptorSetLayout);
    MFA_VK_MAKE_NULL(mDescriptorSetLayout);
}

//-------------------------------------------------------------------------------------------------

void PointLightPipeline::createPipeline() {
    // Vertex shader
    auto cpuVertexShader = Importer::ImportShaderFromSPV(
        Path::Asset("shaders/point_light/PointLight.vert.spv").c_str(), 
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
        Path::Asset("shaders/point_light/PointLight.frag.spv").c_str(),
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

    VkVertexInputBindingDescription bindingDescription {};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(AS::MeshVertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    
    std::vector<VkVertexInputAttributeDescription> inputAttributeDescriptions {};
    {// Position
        VkVertexInputAttributeDescription attributeDescription {};
        attributeDescription.location = static_cast<uint32_t>(inputAttributeDescriptions.size());
        attributeDescription.binding = 0;
        attributeDescription.format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescription.offset = offsetof(AS::MeshVertex, position);

        inputAttributeDescriptions.emplace_back(attributeDescription);
    }
    MFA_ASSERT(mDrawPipeline.isValid() == false);

    RB::CreateGraphicPipelineOptions pipelineOptions {};
    pipelineOptions.useStaticViewportAndScissor = false;
    pipelineOptions.primitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
    pipelineOptions.rasterizationSamples = RF::GetMaxSamplesCount();
    pipelineOptions.cullMode = VK_CULL_MODE_BACK_BIT;

    std::vector<VkPushConstantRange> pushConstantRanges {
        VkPushConstantRange {
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT,
            .offset = 0,
            .size = sizeof(PushConstants),
        }
    };
    pipelineOptions.pushConstantsRangeCount = static_cast<uint32_t>(pushConstantRanges.size());
    pipelineOptions.pushConstantRanges = pushConstantRanges.data();

    mDrawPipeline = RF::CreateDrawPipeline(
        RF::GetDisplayRenderPass()->GetVkRenderPass(),
        static_cast<uint8_t>(shaders.size()), 
        shaders.data(),
        1,
        &mDescriptorSetLayout,
        bindingDescription,
        static_cast<uint8_t>(inputAttributeDescriptions.size()),
        inputAttributeDescriptions.data(),
        pipelineOptions
    );
}

//-------------------------------------------------------------------------------------------------

void PointLightPipeline::destroyPipeline() {
    MFA_ASSERT(mDrawPipeline.isValid());
    RF::DestroyDrawPipeline(mDrawPipeline);
}

//-------------------------------------------------------------------------------------------------

void PointLightPipeline::destroyDrawableObjects() {
    mDrawableObjects.clear();
}

//-------------------------------------------------------------------------------------------------

void PointLightPipeline::createDescriptorSets() {
    mDescriptorSetGroup = RF::CreateDescriptorSets(2, mDescriptorSetLayout);

    for (uint32_t frameIndex = 0; frameIndex < RF::GetMaxFramesPerFlight(); ++frameIndex) {

        auto descriptorSet = mDescriptorSetGroup.descriptorSets[frameIndex];
        MFA_VK_VALID_ASSERT(descriptorSet);

        DescriptorSetSchema descriptorSetSchema {descriptorSet};

        /////////////////////////////////////////////////////////////////
        // Vertex shader
        /////////////////////////////////////////////////////////////////
        
        {// ViewProjectionTransform
            VkDescriptorBufferInfo bufferInfo {
                .buffer = mViewProjectionBuffers.buffers[frameIndex].buffer,
                .offset = 0,
                .range = mViewProjectionBuffers.bufferSize,
            };
            descriptorSetSchema.AddUniformBuffer(bufferInfo);
        }
        
        descriptorSetSchema.UpdateDescriptorSets();
    }
}

//-------------------------------------------------------------------------------------------------


}
