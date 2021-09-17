#include "PbrWithShadowPipelineV2.hpp"

#include "engine/BedrockPath.hpp"
#include "engine/render_system/render_passes/display_render_pass/DisplayRenderPass.hpp"
#include "tools/Importer.hpp"
#include "engine/BedrockMatrix.hpp"
#include "engine/render_system/drawable_essence/DrawableEssence.hpp"
#include "engine/render_system/drawable_variant/DrawableVariant.hpp"
#include "engine/render_system/pipelines/DescriptorSetSchema.hpp"
#include "engine/render_system/render_passes/shadow_render_pass_v2/ShadowRenderPassV2.hpp"

namespace MFA {

//-------------------------------------------------------------------------------------------------

PBRWithShadowPipelineV2::PBRWithShadowPipelineV2()
: mShadowRenderPass(std::make_unique<ShadowRenderPassV2>(
    static_cast<uint32_t>(SHADOW_WIDTH),
    static_cast<uint32_t>(SHADOW_HEIGHT)))
{}

//-------------------------------------------------------------------------------------------------

PBRWithShadowPipelineV2::~PBRWithShadowPipelineV2() {
    if(mIsInitialized) {
        Shutdown();
    }
}

//-------------------------------------------------------------------------------------------------

void PBRWithShadowPipelineV2::Init(
    RT::SamplerGroup * samplerGroup, 
    RT::GpuTexture * errorTexture,
    float const projectionNear,
    float const projectionFar
) {
    if (mIsInitialized == true) {
        MFA_ASSERT(false);
        return;
    }
    mIsInitialized = true;

    BasePipeline::Init();

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

    mShadowRenderPass->Init();
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

    updateShadowViewProjectionData();
        
    RT::DrawPass drawPass {};
    for (uint32_t i = 0; i < RF::GetMaxFramesPerFlight(); ++i) {
        drawPass.frameIndex = i;
        updateShadowLightBuffer(drawPass);
        updateDisplayLightBuffer(drawPass);
        updateShadowViewProjectionBuffer(drawPass);
        updateDisplayViewProjectionBuffer(drawPass);
    }
}

//-------------------------------------------------------------------------------------------------

void PBRWithShadowPipelineV2::Shutdown() {
    if (mIsInitialized == false) {
        MFA_ASSERT(false);
        return;
    }
    mIsInitialized = false;

    mSamplerGroup = nullptr;
    mErrorTexture = nullptr;

    mShadowRenderPass->Shutdown();

    destroyPipeline();
    destroyDescriptorSetLayout();
    destroyUniformBuffers();

    BasePipeline::Shutdown();
    
}

//-------------------------------------------------------------------------------------------------

void PBRWithShadowPipelineV2::PreRender(RT::DrawPass & drawPass, float const deltaTime) {

    BasePipeline::PreRender(drawPass, deltaTime);

    if (mDisplayViewProjectionNeedUpdate > 0) {
        updateDisplayViewProjectionBuffer(drawPass);
        --mDisplayViewProjectionNeedUpdate;
    }
    if (mDisplayLightNeedUpdate > 0) {
        updateDisplayLightBuffer(drawPass);
        --mDisplayLightNeedUpdate;
    }
    if (mShadowLightNeedUpdate > 0) {
        updateShadowLightBuffer(drawPass);
        --mShadowLightNeedUpdate;
    }
    if (mShadowViewProjectionNeedUpdate > 0) {
        updateShadowViewProjectionBuffer(drawPass);
        --mShadowViewProjectionNeedUpdate;
    }

    RF::BindDrawPipeline(drawPass, mShadowPassPipeline);

    mShadowRenderPass->PrepareCubeMapForTransferDestination(drawPass);
    for (int faceIndex = 0; faceIndex < 6; ++faceIndex) {
        mShadowRenderPass->SetNextPassParams(faceIndex);
        mShadowRenderPass->BeginRenderPass(drawPass);

        for (auto & essenceAndVariant : mEssenceAndVariantsMap) {
            for (auto & variant : essenceAndVariant.second->variants) {
                RF::BindDescriptorSet(
                    drawPass, 
                    variant->GetDescriptorSetGroup("PbrWithShadowV2ShadowPipeline")->descriptorSets[drawPass.frameIndex]
                );

                variant->Draw(drawPass, [&drawPass, &faceIndex](AS::MeshPrimitive const & primitive, DrawableVariant::Node const & node)-> void {

                    {// Vertex push constants
                        ShadowPassVertexStagePushConstants pushConstants {
                            .faceIndex = faceIndex,
                            .skinIndex = node.skin != nullptr ? node.skin->skinStartingIndex : -1,
                        };
                        Matrix4X4Float::ConvertGlmToCells(node.cachedModelTransform, pushConstants.modeTransform);
                        Matrix4X4Float::ConvertGlmToCells(node.cachedGlobalInverseTransform, pushConstants.inverseNodeTransform);
                        RF::PushConstants(
                            drawPass,
                            AS::ShaderStage::Vertex,
                            0,
                            CBlobAliasOf(pushConstants)
                        );
                    }
                    
                });
            }
        }
        mShadowRenderPass->EndRenderPass(drawPass);
    }
    mShadowRenderPass->PrepareCubeMapForSampling(drawPass);
}

//-------------------------------------------------------------------------------------------------

void PBRWithShadowPipelineV2::Render(RT::DrawPass & drawPass, float deltaTime) {
    RF::BindDrawPipeline(drawPass, mDisplayPassPipeline);
    
    for (auto & essenceAndVariant : mEssenceAndVariantsMap) {
        for (auto & variant : essenceAndVariant.second->variants) {
            RF::BindDescriptorSet(
                drawPass, 
                variant->GetDescriptorSetGroup("PbrWithShadowV2DisplayPipeline")->descriptorSets[drawPass.frameIndex]
            );

            variant->Draw(drawPass, [&drawPass](AS::MeshPrimitive const & primitive, DrawableVariant::Node const & node)-> void {
                // Push constants
                DisplayPassAllStagesPushConstants pushConstants {
                    .skinIndex = node.skin != nullptr ? node.skin->skinStartingIndex : -1,
                    .primitiveIndex = primitive.uniqueId
                };
                Matrix4X4Float::ConvertGlmToCells(node.cachedModelTransform, pushConstants.modeTransform);
                Matrix4X4Float::ConvertGlmToCells(node.cachedGlobalInverseTransform, pushConstants.inverseNodeTransform);
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

DrawableVariant * PBRWithShadowPipelineV2::CreateDrawableVariant(char const * essenceName) {
    auto * variant = BasePipeline::CreateDrawableVariant(essenceName);
    MFA_ASSERT(variant != nullptr);

    CreateDisplayPassDescriptorSets(variant);
    CreateShadowPassDescriptorSets(variant);
    
    return variant;
}

//-------------------------------------------------------------------------------------------------

void PBRWithShadowPipelineV2::UpdateCameraView(const float view[16]) {
    if (IsEqual<16>(mDisplayView, view) == false) {
        Copy<16>(mDisplayView, view);
        mDisplayViewProjectionNeedUpdate = RF::GetMaxFramesPerFlight();
    }
}

//-------------------------------------------------------------------------------------------------

void PBRWithShadowPipelineV2::UpdateCameraProjection(const float projection[16]) {
    if (IsEqual<16>(mDisplayProjection, projection) == false) {
        Copy<16>(mDisplayProjection, projection);
        mDisplayViewProjectionNeedUpdate = RF::GetMaxFramesPerFlight();
    }
}

//-------------------------------------------------------------------------------------------------

void PBRWithShadowPipelineV2::UpdateLightPosition(const float lightPosition[3]) {
    if (IsEqual<3>(mLightPosition, lightPosition) == false) {
        Copy<3>(mLightPosition, lightPosition);
        updateShadowViewProjectionData();
        mDisplayLightNeedUpdate = RF::GetMaxFramesPerFlight();
        mShadowLightNeedUpdate = RF::GetMaxFramesPerFlight();
        mShadowViewProjectionNeedUpdate = RF::GetMaxFramesPerFlight();
    }
}

//-------------------------------------------------------------------------------------------------

void PBRWithShadowPipelineV2::UpdateCameraPosition(const float cameraPosition[3]) {
    if (IsEqual<3>(mCameraPosition, cameraPosition) == false) {
        Copy<3>(mCameraPosition, cameraPosition);
        mDisplayLightNeedUpdate = RF::GetMaxFramesPerFlight();
    }
}

//-------------------------------------------------------------------------------------------------

void PBRWithShadowPipelineV2::UpdateLightColor(const float lightColor[3]) {
    if (IsEqual<3>(mLightColor, lightColor) == false) {
        Copy<3>(mLightColor, lightColor);
        mDisplayLightNeedUpdate = RF::GetMaxFramesPerFlight();
    }
}

//-------------------------------------------------------------------------------------------------

void PBRWithShadowPipelineV2::updateDisplayViewProjectionBuffer(RT::DrawPass const & drawPass) {

    auto const viewProjectionMatrix = Matrix4X4Float::ConvertCellsToMat4(mDisplayProjection)
        * Matrix4X4Float::ConvertCellsToMat4(mDisplayView);

    RF::UpdateUniformBuffer(
        mDisplayViewProjectionBuffer.buffers[drawPass.frameIndex],
        CBlobAliasOf(viewProjectionMatrix)
    );
}

//-------------------------------------------------------------------------------------------------

void PBRWithShadowPipelineV2::updateDisplayLightBuffer(RT::DrawPass const & drawPass) {
    DisplayLightAndCameraData displayLightViewData {
        .projectFarToNearDistance = mProjectionFarToNearDistance
    };
    Copy<3>(displayLightViewData.cameraPosition, mCameraPosition);
    Copy<3>(displayLightViewData.lightColor, mLightColor);
    Copy<3>(displayLightViewData.lightPosition, mLightPosition);
    RF::UpdateUniformBuffer(mDisplayLightAndCameraBuffer.buffers[drawPass.frameIndex], CBlobAliasOf(displayLightViewData));
}

//-------------------------------------------------------------------------------------------------

void PBRWithShadowPipelineV2::updateShadowLightBuffer(RT::DrawPass const & drawPass) {
    ShadowLightData shadowLightData {
        .projectionMatrixDistance = mProjectionFarToNearDistance
    };
    Copy<3>(shadowLightData.lightPosition, mLightPosition);
    RF::UpdateUniformBuffer(mShadowLightBuffer.buffers[drawPass.frameIndex], CBlobAliasOf(shadowLightData));
}

//-------------------------------------------------------------------------------------------------

void PBRWithShadowPipelineV2::updateShadowViewProjectionData() {
    auto const lightPositionVector = glm::vec3(mLightPosition[0], mLightPosition[1], mLightPosition[2]);

    Matrix4X4Float::ConvertGlmToCells(
        mShadowProjection * glm::lookAt(
            lightPositionVector, 
            lightPositionVector + glm::vec3( 1.0, 0.0, 0.0), 
            glm::vec3(0.0,-1.0, 0.0)
        ), 
        mShadowViewProjectionData.viewMatrices[0]
    );

    Matrix4X4Float::ConvertGlmToCells(
        mShadowProjection * glm::lookAt(
            lightPositionVector, 
            lightPositionVector + glm::vec3(-1.0, 0.0, 0.0), 
            glm::vec3(0.0,-1.0, 0.0)
        ), 
        mShadowViewProjectionData.viewMatrices[1]
    );

    Matrix4X4Float::ConvertGlmToCells(
        mShadowProjection * glm::lookAt(
            lightPositionVector, 
            lightPositionVector + glm::vec3( 0.0, 1.0, 0.0), 
            glm::vec3(0.0, 0.0, 1.0)
        ), 
        mShadowViewProjectionData.viewMatrices[2]
    );

    Matrix4X4Float::ConvertGlmToCells(
        mShadowProjection * glm::lookAt(
            lightPositionVector, 
            lightPositionVector + glm::vec3( 0.0,-1.0, 0.0), 
            glm::vec3(0.0, 0.0,-1.0)
        ), 
        mShadowViewProjectionData.viewMatrices[3]
    );

    Matrix4X4Float::ConvertGlmToCells(
        mShadowProjection * glm::lookAt(
            lightPositionVector, 
            lightPositionVector + glm::vec3( 0.0, 0.0, 1.0), 
            glm::vec3(0.0,-1.0, 0.0)
        ), 
        mShadowViewProjectionData.viewMatrices[4]
    );

    Matrix4X4Float::ConvertGlmToCells(
        mShadowProjection * glm::lookAt(
            lightPositionVector, 
            lightPositionVector + glm::vec3( 0.0, 0.0,-1.0), 
            glm::vec3(0.0,-1.0, 0.0)
        ), 
        mShadowViewProjectionData.viewMatrices[5]
    );
}

//-------------------------------------------------------------------------------------------------

void PBRWithShadowPipelineV2::updateShadowViewProjectionBuffer(RT::DrawPass const & drawPass) {
    RF::UpdateUniformBuffer(
        mShadowViewProjectionBuffer.buffers[drawPass.frameIndex],
        CBlobAliasOf(mShadowViewProjectionData)
    );
}

//-------------------------------------------------------------------------------------------------

void PBRWithShadowPipelineV2::CreateDisplayPassDescriptorSets(DrawableVariant * variant) {
    MFA_ASSERT(variant != nullptr);
    auto const & textures = variant->GetEssence()->GetGpuModel().textures;
    
    auto const descriptorSetGroup = variant->CreateDescriptorSetGroup(
        "PbrWithShadowV2DisplayPipeline",
        mDisplayPassDescriptorSetLayout,
        RF::GetMaxFramesPerFlight()
    );

    auto const & primitivesBuffer = variant->GetEssence()->GetPrimitivesBuffer();

    auto const & skinJointsBuffer = variant->GetSkinJointsBuffer();

    for (uint32_t frameIndex = 0; frameIndex < RF::GetMaxFramesPerFlight(); ++frameIndex) {

        auto descriptorSet = descriptorSetGroup.descriptorSets[frameIndex];
        MFA_VK_VALID_ASSERT(descriptorSet);

        DescriptorSetSchema descriptorSetSchema {descriptorSet};

        /////////////////////////////////////////////////////////////////
        // Vertex shader
        /////////////////////////////////////////////////////////////////
        
        {// ViewProjectionTransform
            VkDescriptorBufferInfo bufferInfo {
                .buffer = mDisplayViewProjectionBuffer.buffers[frameIndex].buffer,
                .offset = 0,
                .range = mDisplayViewProjectionBuffer.bufferSize,
            };
            descriptorSetSchema.AddUniformBuffer(bufferInfo);
        }

        {// SkinJoints
            VkBuffer buffer = mErrorBuffer.buffers[0].buffer;
            size_t bufferSize = mErrorBuffer.bufferSize;
            if (skinJointsBuffer.bufferSize > 0) {
                buffer = skinJointsBuffer.buffers[frameIndex].buffer;
                bufferSize = skinJointsBuffer.bufferSize;
            }
            VkDescriptorBufferInfo skinTransformBufferInfo {
                .buffer = buffer,
                .offset = 0,
                .range = bufferSize,
            };
            descriptorSetSchema.AddUniformBuffer(skinTransformBufferInfo);
        }

        /////////////////////////////////////////////////////////////////
        // Fragment shader
        /////////////////////////////////////////////////////////////////
        
        {// Primitives
            VkDescriptorBufferInfo primitiveBufferInfo {
                .buffer = primitivesBuffer.buffers[0].buffer,
                .offset = 0,
                .range = primitivesBuffer.bufferSize,
            };
            descriptorSetSchema.AddUniformBuffer(primitiveBufferInfo);
        }

        {// LightViewBuffer
            VkDescriptorBufferInfo lightViewBufferInfo {
                .buffer = mDisplayLightAndCameraBuffer.buffers[frameIndex].buffer,
                .offset = 0,
                .range = mDisplayLightAndCameraBuffer.bufferSize,
            };
            descriptorSetSchema.AddUniformBuffer(lightViewBufferInfo);
        }

        {// ShadowMap
            VkDescriptorImageInfo shadowMapImageInfo {
                .sampler = mSamplerGroup->sampler,
                .imageView = mShadowRenderPass->GetDepthCubeMap().imageView,
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            };
            descriptorSetSchema.AddCombinedImageSampler(shadowMapImageInfo);
        }

        {// Sampler
            VkDescriptorImageInfo samplerInfo {
                .sampler = mSamplerGroup->sampler,
                .imageView = nullptr,
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            };
            descriptorSetSchema.AddSampler(samplerInfo);
        }

        // TODO Each one need their own sampler
        // Textures
        MFA_ASSERT(textures.size() <= 64);
        // We need to keep imageInfos alive
        std::vector<VkDescriptorImageInfo> imageInfos {};
        for (auto const & texture : textures) {
            imageInfos.emplace_back(VkDescriptorImageInfo {
                .sampler = nullptr,
                .imageView = texture.imageView(),
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            });
        }
        for (auto i = static_cast<uint32_t>(textures.size()); i < 64; ++i) {
            imageInfos.emplace_back(VkDescriptorImageInfo {
                .sampler = nullptr,
                .imageView = mErrorTexture->imageView(),
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            });
        }
        MFA_ASSERT(imageInfos.size() == 64);
        descriptorSetSchema.AddImage(
            imageInfos.data(), 
            static_cast<uint32_t>(imageInfos.size())
        );
        
        descriptorSetSchema.UpdateDescriptorSets();
    }
}

void PBRWithShadowPipelineV2::CreateShadowPassDescriptorSets(DrawableVariant * variant) {

    auto const descriptorSetGroup = variant->CreateDescriptorSetGroup(
        "PbrWithShadowV2ShadowPipeline", 
        mShadowPassDescriptorSetLayout, 
        variant->GetEssence()->GetPrimitiveCount() * RF::GetMaxFramesPerFlight()
    );

    auto const & skinJointsBuffer = variant->GetSkinJointsBuffer();
    
    for (uint32_t frameIndex = 0; frameIndex < RF::GetMaxFramesPerFlight(); ++frameIndex) {

        auto descriptorSet = descriptorSetGroup.descriptorSets[frameIndex];
        MFA_VK_VALID_ASSERT(descriptorSet);
       
        DescriptorSetSchema descriptorSetSchema {descriptorSet};

        /////////////////////////////////////////////////////////////////
        // Vertex shader
        /////////////////////////////////////////////////////////////////
        
        {// ViewProjectionTransform
            VkDescriptorBufferInfo viewProjectionBufferInfo {
                .buffer = mShadowViewProjectionBuffer.buffers[frameIndex].buffer,
                .offset = 0,
                .range = mShadowViewProjectionBuffer.bufferSize
            };
            descriptorSetSchema.AddUniformBuffer(viewProjectionBufferInfo);
        }
        {// SkinJoints
            VkBuffer buffer = mErrorBuffer.buffers[0].buffer;
            size_t bufferSize = mErrorBuffer.bufferSize;
            if (skinJointsBuffer.bufferSize > 0) {
                buffer = skinJointsBuffer.buffers[frameIndex].buffer;
                bufferSize = skinJointsBuffer.bufferSize;
            }
            VkDescriptorBufferInfo skinTransformBufferInfo {
                .buffer = buffer,
                .offset = 0,
                .range = bufferSize,
            };
            descriptorSetSchema.AddUniformBuffer(skinTransformBufferInfo);
        }

        /////////////////////////////////////////////////////////////////
        // Fragment shader
        /////////////////////////////////////////////////////////////////

        {// LightBuffer
            VkDescriptorBufferInfo bufferInfo {
                .buffer = mShadowLightBuffer.buffers[frameIndex].buffer,
                .offset = 0,
                .range = mShadowLightBuffer.bufferSize,
            };
            descriptorSetSchema.AddUniformBuffer(bufferInfo);
        }

        descriptorSetSchema.UpdateDescriptorSets();
    }
}

void PBRWithShadowPipelineV2::createDisplayPassDescriptorSetLayout() {
    std::vector<VkDescriptorSetLayoutBinding> bindings {};
    
    /////////////////////////////////////////////////////////////////
    // Vertex shader
    /////////////////////////////////////////////////////////////////

    {// ViewProjectionTransform
        VkDescriptorSetLayoutBinding layoutBinding {
            .binding = static_cast<uint32_t>(bindings.size()),
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        };
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
        VkDescriptorSetLayoutBinding layoutBinding {
            .binding = static_cast<uint32_t>(bindings.size()),
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        };
        bindings.emplace_back(layoutBinding);
    }
    {// Light/View
        VkDescriptorSetLayoutBinding layoutBinding {
            .binding = static_cast<uint32_t>(bindings.size()),
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        };
        bindings.emplace_back(layoutBinding);
    }
    {// ShadowMap
        VkDescriptorSetLayoutBinding layoutBinding {
            .binding = static_cast<uint32_t>(bindings.size()),
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        };
        bindings.emplace_back(layoutBinding);
    }
    {// TextureSampler
        VkDescriptorSetLayoutBinding layoutBinding {
            .binding = static_cast<uint32_t>(bindings.size()),
            .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        };
        bindings.emplace_back(layoutBinding);
    }
    {// Textures
        VkDescriptorSetLayoutBinding layoutBinding {
            .binding = static_cast<uint32_t>(bindings.size()),
            .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
            .descriptorCount = 64,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        };
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

    {// ViewProjectionTransform
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
        Importer::FreeShader(cpuVertexShader);
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
        Importer::FreeShader(cpuFragmentShader);
    };

    std::vector<RT::GpuShader const *> shaders {&gpuVertexShader, &gpuFragmentShader};

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

    std::vector<VkPushConstantRange> mPushConstantRanges {};
    mPushConstantRanges.emplace_back(VkPushConstantRange {
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        .offset = 0,
        .size = sizeof(DisplayPassAllStagesPushConstants),
    });
    
    RT::CreateGraphicPipelineOptions options {};
    options.rasterizationSamples = RF::GetMaxSamplesCount();
    options.pushConstantRanges = mPushConstantRanges.data();
    options.pushConstantsRangeCount = static_cast<uint8_t>(mPushConstantRanges.size());
    options.cullMode = VK_CULL_MODE_BACK_BIT;
    
    mDisplayPassPipeline = RF::CreatePipeline(
        RF::GetDisplayRenderPass(),
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
        Importer::FreeShader(cpuVertexShader);
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
        Importer::FreeShader(cpuFragmentShader);
    };

    std::vector<RT::GpuShader const *> shaders {&gpuVertexShader, &gpuFragmentShader};

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
            .size = sizeof(ShadowPassVertexStagePushConstants),
        };
        pushConstantRanges.emplace_back(pushConstantRange);
    }  
    RT::CreateGraphicPipelineOptions graphicPipelineOptions {};
    graphicPipelineOptions.cullMode = VK_CULL_MODE_NONE;
    graphicPipelineOptions.pushConstantRanges = pushConstantRanges.data();
    // TODO Probably we need to make pushConstantsRangeCount uint32_t
    graphicPipelineOptions.pushConstantsRangeCount = static_cast<uint8_t>(pushConstantRanges.size());
    
    MFA_ASSERT(mShadowPassPipeline.isValid() == false);
    mShadowPassPipeline = RF::CreatePipeline(
        mShadowRenderPass.get(),
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
    RF::DestroyPipelineGroup(mDisplayPassPipeline);

    MFA_ASSERT(mShadowPassPipeline.isValid());
    RF::DestroyPipelineGroup(mShadowPassPipeline);
}

void PBRWithShadowPipelineV2::createUniformBuffers() {
    mErrorBuffer = RF::CreateUniformBuffer(sizeof(DrawableVariant::JointTransformData), 1);

    mDisplayViewProjectionBuffer = RF::CreateUniformBuffer(sizeof(DisplayViewProjectionData), RF::GetMaxFramesPerFlight());
    mDisplayLightAndCameraBuffer = RF::CreateUniformBuffer(sizeof(DisplayLightAndCameraData), RF::GetMaxFramesPerFlight());
    
    mShadowViewProjectionBuffer = RF::CreateUniformBuffer(sizeof(ShadowViewProjectionData), RF::GetMaxFramesPerFlight());
    mShadowLightBuffer = RF::CreateUniformBuffer(sizeof(ShadowLightData), RF::GetMaxFramesPerFlight());
}

void PBRWithShadowPipelineV2::destroyUniformBuffers() {
    RF::DestroyUniformBuffer(mErrorBuffer);

    RF::DestroyUniformBuffer(mDisplayViewProjectionBuffer);
    RF::DestroyUniformBuffer(mDisplayLightAndCameraBuffer);
    
    RF::DestroyUniformBuffer(mShadowViewProjectionBuffer);
    RF::DestroyUniformBuffer(mShadowLightBuffer);
}

}
