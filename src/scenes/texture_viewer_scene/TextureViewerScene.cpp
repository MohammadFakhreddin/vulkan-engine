#include "TextureViewerScene.hpp"

#include "engine/BedrockMemory.hpp"
#include "libs/tiny_ktx/tinyktx.h"
#include "tools/Importer.hpp"
#include "engine/BedrockFileSystem.hpp"
#include "libs/imgui/imgui.h"
#include "tools/ShapeGenerator.hpp"

namespace RF = MFA::RenderFrontend;
namespace RB = MFA::RenderBackend;
namespace Importer = MFA::Importer;
namespace FS = MFA::FileSystem;
namespace AS = MFA::AssetSystem;
namespace SG = MFA::ShapeGenerator;

static void tinyktxCallbackError(void *user, char const *msg) {
    MFA_LOG_ERROR("Tiny_Ktx ERROR: %s", msg);
}
static void *tinyktxCallbackAlloc(void *user, const size_t size) {
    return MFA::Memory::Alloc(size).ptr;
}
static void tinyktxCallbackFree(void *user, void *data) {
    MFA::Memory::PtrFree(data);
}
// TODO Start from here, Complete all callbacks
static size_t tinyktxCallbackRead(void *user, void* data, const size_t size) {
    auto * handle = static_cast<FS::FileHandle *>(user);
    return FS::Read(handle, MFA::Blob {data, size});
}
static bool tinyktxCallbackSeek(void *user, const int64_t offset) {
    auto * handle = static_cast<FS::FileHandle *>(user);
    return FS::Seek(handle, static_cast<int>(offset), FS::Origin::Start);

}
static int64_t tinyktxCallbackTell(void *user) {
    auto * handle = static_cast<FS::FileHandle *>(user);
    int64_t location = 0;
    bool success = FS::Tell(handle, location);
    MFA_ASSERT(success);
    return location;
}

AS::Texture loadKTX(FS::FileHandle * handle) {

    using namespace MFA;
    using TextureFormat = AssetSystem::Texture::Format;

    AS::Texture result {};

    TinyKtx_Callbacks callbacks {
            &tinyktxCallbackError,
            &tinyktxCallbackAlloc,
            &tinyktxCallbackFree,
            tinyktxCallbackRead,
            &tinyktxCallbackSeek,
            &tinyktxCallbackTell
    };

    auto * ctx =  TinyKtx_CreateContext( &callbacks, handle);
    MFA_DEFER {
        TinyKtx_DestroyContext(ctx);
    };
    
    auto const readHeaderResult = TinyKtx_ReadHeader(ctx);
    MFA_ASSERT(readHeaderResult);

    U16 width = static_cast<U16>(TinyKtx_Width(ctx));
    U16 height = static_cast<U16>(TinyKtx_Height(ctx));
    U16 depth = Math::Max<U16>(static_cast<U16>(TinyKtx_Depth(ctx)), 1);
    const U8 sliceCount = Math::Max<U8>(static_cast<U8>(TinyKtx_ArraySlices(ctx)), 1);
    // TODO
    //TinyKtx_Is1D()
    //TinyKtx_Is2D()
    //TinyKtx_Is3D()
    //TinyKtx_IsCubemap()
    
    TinyKtx_Format const tinyKtxFormat = TinyKtx_GetFormat(ctx);
    TextureFormat const format = [tinyKtxFormat]() -> TextureFormat
    {
        TextureFormat conversionResult = TextureFormat::INVALID;
        switch (tinyKtxFormat) 
        {
            case TKTX_BC7_SRGB_BLOCK:
                conversionResult = TextureFormat::BC7_UNorm_sRGB_RGBA;
            break;
            case TKTX_BC7_UNORM_BLOCK:
                conversionResult = TextureFormat::BC7_UNorm_Linear_RGBA;
            break;
            case TKTX_BC5_UNORM_BLOCK:
                conversionResult = TextureFormat::BC5_UNorm_Linear_RG;
            break;
            case TKTX_BC5_SNORM_BLOCK:
                conversionResult = TextureFormat::BC5_SNorm_Linear_RG;
            break;
            case TKTX_BC4_UNORM_BLOCK:
                conversionResult = TextureFormat::BC4_UNorm_Linear_R;
            break;
            case TKTX_BC4_SNORM_BLOCK:
                conversionResult = AssetSystem::Texture::Format::BC4_SNorm_Linear_R;
            break;
            case TKTX_R8G8B8A8_UNORM:
                conversionResult = AssetSystem::Texture::Format::UNCOMPRESSED_UNORM_R8G8B8A8_LINEAR;
            break;
            case TKTX_R8G8B8A8_SRGB:
                conversionResult = TextureFormat::UNCOMPRESSED_UNORM_R8G8B8A8_SRGB;
            break;
            case TKTX_R8G8_UNORM:
                conversionResult = TextureFormat::UNCOMPRESSED_UNORM_R8G8_LINEAR;
            break;
            case TKTX_R8_UNORM:
                conversionResult = TextureFormat::UNCOMPRESSED_UNORM_R8_LINEAR;
            break;
            case TKTX_R8_SNORM:
                conversionResult = TextureFormat::UNCOMPRESSED_UNORM_R8_SRGB;
            break;
            default:
                break;
        }
        return conversionResult;
    }();

    if(tinyKtxFormat != TKTX_UNDEFINED) {
        // TODO Maybe I have to reevaluate my values
        const U8 mipmapCount = static_cast<U8>(TinyKtx_NumberOfMipmaps(ctx));
        uint64_t totalImageSize = 0;

        for(auto i = 0u; i < mipmapCount; ++i) {
            totalImageSize += TinyKtx_ImageSize(ctx, i);
        }
        
        result.initForWrite(
            format, 
            sliceCount, 
            mipmapCount, 
            depth, 
            nullptr, 
            Memory::Alloc(totalImageSize)
        );

        for(auto i = 0u; i < mipmapCount; ++i) {
            MFA_ASSERT(width >= 1);
            MFA_ASSERT(height >= 1);
            MFA_ASSERT(depth >= 1);

            auto const imageSize = TinyKtx_ImageSize(ctx, i);
            auto const * imageDataPtr = TinyKtx_ImageRawData(ctx, i);
            
            result.addMipmap(
                AS::Texture::Dimensions {
                    .width = width,
                    .height = height,
                    .depth = depth
                }, 
                CBlob {imageDataPtr, imageSize}
            );

            width = Math::Max<U16>(width / 2, 1);
            height = Math::Max<U16>(height / 2, 1);
            depth = Math::Max<U16>(depth / 2, 1);
        }
    }

    return result;
}

void TextureViewerScene::Init() {

    // Vertex shader
    auto cpu_vertex_shader = Importer::ImportShaderFromSPV(
        "../assets/shaders/texture_viewer/TextureViewer.vert.spv", 
        MFA::AssetSystem::Shader::Stage::Vertex, 
        "main"
    );
    MFA_ASSERT(cpu_vertex_shader.isValid());
    auto gpu_vertex_shader = RF::CreateShader(cpu_vertex_shader);
    MFA_ASSERT(gpu_vertex_shader.valid());
    MFA_DEFER {
        RF::DestroyShader(gpu_vertex_shader);
        Importer::FreeShader(&cpu_vertex_shader);
    };
    
    // Fragment shader
    auto cpu_fragment_shader = Importer::ImportShaderFromSPV(
        "../assets/shaders/texture_viewer/TextureViewer.frag.spv",
        MFA::AssetSystem::Shader::Stage::Fragment,
        "main"
    );
    auto gpu_fragment_shader = RF::CreateShader(cpu_fragment_shader);
    MFA_ASSERT(cpu_fragment_shader.isValid());
    MFA_ASSERT(gpu_fragment_shader.valid());
    MFA_DEFER {
        RF::DestroyShader(gpu_fragment_shader);
        Importer::FreeShader(&cpu_fragment_shader);
    };

    std::vector<RB::GpuShader> shaders {gpu_vertex_shader, gpu_fragment_shader};

    // TODO We need nearest and linear filters
    mSamplerGroup = RF::CreateSampler();

    createDescriptorSetLayout();

    createModel();

    createDrawPipeline(static_cast<MFA::U8>(shaders.size()), shaders.data());

    // Updating perspective mat once for entire application
    // Perspective
    MFA::I32 width; MFA::I32 height;
    RF::GetWindowSize(width, height);
    float const ratio = static_cast<float>(width) / static_cast<float>(height);
    MFA::Matrix4X4Float perspectiveMat {};
    MFA::Matrix4X4Float::PreparePerspectiveProjectionMatrix(
        perspectiveMat,
        ratio,
        40,
        Z_NEAR,
        Z_FAR
    );
    static_assert(sizeof(mViewProjectionBuffer.perspective) == sizeof(perspectiveMat.cells));
    ::memcpy(mViewProjectionBuffer.perspective, perspectiveMat.cells, sizeof(perspectiveMat.cells));

}

void TextureViewerScene::Shutdown() {
    RF::DestroyDrawPipeline(mDrawPipeline);
    RF::DestroyDescriptorSetLayout(mDescriptorSetLayout);
    RF::DestroySampler(mSamplerGroup);
    RF::DestroyGpuModel(mGpuModel);
    Importer::FreeModel(&mGpuModel.model);
    mDrawableObject.deleteUniformBuffers();
}

void TextureViewerScene::OnDraw(
    MFA::U32 deltaTime, 
    RF::DrawPass & drawPass
) {
    RF::BindDrawPipeline(drawPass, mDrawPipeline);

    // ProjectionViewBuffer
    {
        // Rotation
        MFA::Matrix4X4Float rotationMat {};
        MFA::Matrix4X4Float::AssignRotation(
            rotationMat,
            MFA::Math::Deg2Rad(mModelRotation[0]),
            MFA::Math::Deg2Rad(mModelRotation[1]),
            MFA::Math::Deg2Rad(mModelRotation[2])
        );

        // Scale
        MFA::Matrix4X4Float scaleMat {};
        MFA::Matrix4X4Float::AssignScale(scaleMat, mModelScale);

        // Position
        MFA::Matrix4X4Float translationMat {};
        MFA::Matrix4X4Float::AssignTranslation(
            translationMat,
            mModelPosition[0],
            mModelPosition[1],
            mModelPosition[2]
        );

        MFA::Matrix4X4Float transformMat {};
        MFA::Matrix4X4Float::identity(transformMat);
        transformMat.multiply(translationMat);
        transformMat.multiply(rotationMat);
        transformMat.multiply(scaleMat);

        ::memcpy(mViewProjectionBuffer.view, transformMat.cells, sizeof(transformMat.cells));
        static_assert(sizeof(transformMat.cells) == sizeof(mViewProjectionBuffer.view));

        mDrawableObject.updateUniformBuffer(
            "viewProjection",
            MFA::CBlobAliasOf(mViewProjectionBuffer)
        );
    }
    
    // Texture options buffer
    {
        mImageOptionsBuffer.mipLevel = mMipLevel;

        mDrawableObject.updateUniformBuffer(
            "imageOptions",
            MFA::CBlobAliasOf(mImageOptionsBuffer)
        );
    }

    mDrawableObject.draw(drawPass);
}

void TextureViewerScene::OnUI(
    MFA::U32 delta_time, 
    RF::DrawPass & draw_pass
) {
    // TODO Slider for mip-level, We need to store total mipmap count

    static constexpr float ItemWidth = 500;
    ImGui::Begin("Object viewer");
    ImGui::SetNextItemWidth(ItemWidth);
    MFA_ASSERT(mGpuModel.textures.size() == 1);
    ImGui::SliderInt("MipLevel", &mMipLevel, 0, mGpuModel.textures[0].cpu_texture()->GetMipCount());
    ImGui::SetNextItemWidth(ItemWidth);
    ImGui::SliderFloat("XDegree", &mModelRotation[0], -360.0f, 360.0f);
    ImGui::SetNextItemWidth(ItemWidth);
    ImGui::SliderFloat("YDegree", &mModelRotation[1], -360.0f, 360.0f);
    ImGui::SetNextItemWidth(ItemWidth);
    ImGui::SliderFloat("ZDegree", &mModelRotation[2], -360.0f, 360.0f);
    ImGui::SetNextItemWidth(ItemWidth);
    ImGui::SliderFloat("Scale", &mModelScale, 0.0f, 1.0f);
    ImGui::SetNextItemWidth(ItemWidth);
    ImGui::SliderFloat("XDistance", &mModelPosition[0], -500.0f, 500.0f);
    ImGui::SetNextItemWidth(ItemWidth);
    ImGui::SliderFloat("YDistance", &mModelPosition[1], -500.0f, 500.0f);
    ImGui::SetNextItemWidth(ItemWidth);
    ImGui::SliderFloat("ZDistance", &mModelPosition[2], -500.0f, 100.0f);
    ImGui::End();
}

void TextureViewerScene::createDescriptorSetLayout() {
        std::vector<VkDescriptorSetLayoutBinding> bindings {};
    // ViewProjectionBuffer 
    bindings.emplace_back(VkDescriptorSetLayoutBinding {
        .binding = static_cast<MFA::U32>(bindings.size()),
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        .pImmutableSamplers = nullptr, // Optional
    });
    // Texture
    bindings.emplace_back(VkDescriptorSetLayoutBinding {
        .binding = static_cast<MFA::U32>(bindings.size()),
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        .pImmutableSamplers = nullptr,
    });
    // ImageOptions
    bindings.emplace_back(VkDescriptorSetLayoutBinding {
        .binding = static_cast<MFA::U32>(bindings.size()),
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        .pImmutableSamplers = nullptr, // Optional
    });
    mDescriptorSetLayout = RF::CreateDescriptorSetLayout(
        static_cast<MFA::U8>(bindings.size()),
        bindings.data()
    );
}

void TextureViewerScene::createDrawPipeline(
    MFA::U8 const gpuShaderCount, 
    MFA::RenderBackend::GpuShader * gpuShaders
) {
    VkVertexInputBindingDescription const vertexInputBindingDescription {
        .binding = 0,
        .stride = sizeof(AS::MeshVertex),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
    };

    std::vector<VkVertexInputAttributeDescription> vkVertexInputAttributeDescriptions {};
    // Position
    vkVertexInputAttributeDescriptions.emplace_back(VkVertexInputAttributeDescription {
        .location = static_cast<MFA::U32>(vkVertexInputAttributeDescriptions.size()),
        .binding = 0,
        .format = VK_FORMAT_R32G32B32_SFLOAT,
        .offset = offsetof(AS::MeshVertex, position),   
    });
    // BaseColor
    vkVertexInputAttributeDescriptions.emplace_back(VkVertexInputAttributeDescription {
        .location = static_cast<MFA::U32>(vkVertexInputAttributeDescriptions.size()),
        .binding = 0,
        .format = VK_FORMAT_R32G32_SFLOAT,
        .offset = offsetof(AS::MeshVertex, baseColorUV),   
    });

    mDrawPipeline = RF::CreateBasicDrawPipeline(
        gpuShaderCount, 
        gpuShaders,
        mDescriptorSetLayout,
        vertexInputBindingDescription,
        static_cast<MFA::U8>(vkVertexInputAttributeDescriptions.size()),
        vkVertexInputAttributeDescriptions.data()
    );
}

void TextureViewerScene::createModel() {
    auto cpuModel = SG::Sheet();

    auto * fileHandle = MFA::FileSystem::OpenFile(
        "../assets/models/sponza/2185409758123873465.ktx", 
        MFA::FileSystem::Usage::Read
    );
    MFA_ASSERT(fileHandle != nullptr);

    auto cpuTexture = loadKTX(fileHandle);
    MFA_ASSERT(cpuTexture.isValid());

    cpuModel.textures.emplace_back(cpuTexture);
    MFA_ASSERT(cpuModel.mesh.getSubMeshCount() == 1);
    MFA_ASSERT(cpuModel.mesh.getSubMeshByIndex(0).primitives.size() == 1);
    cpuModel.mesh.getSubMeshByIndex(0).primitives[0].baseColorTextureIndex = 0;

    mGpuModel = MFA::RenderFrontend::CreateGpuModel(cpuModel);

    mDrawableObject = MFA::DrawableObject {
        mGpuModel,
        mDescriptorSetLayout
    };

    auto const * viewProjectionBuffer = mDrawableObject.createUniformBuffer(
        "viewProjection", 
        sizeof(ViewProjectionBuffer)
    );
    MFA_ASSERT(viewProjectionBuffer != nullptr);

    auto const * imageOptionsBuffer = mDrawableObject.createUniformBuffer(
        "imageOptions", 
        sizeof(ImageOptionsBuffer)
    );
    MFA_ASSERT(imageOptionsBuffer != nullptr);

    MFA_ASSERT(cpuModel.mesh.getSubMeshCount() == 1);
    MFA_ASSERT(cpuModel.mesh.getSubMeshByIndex(0).primitives.size() == 1);
    auto const & primitive = cpuModel.mesh.getSubMeshByIndex(0).primitives[0];
    auto * descriptorSet = mDrawableObject.getDescriptorSetByPrimitiveUniqueId(primitive.uniqueId);

    std::vector<VkWriteDescriptorSet> writeInfo {};

    // ViewProjection
    VkDescriptorBufferInfo viewProjectionBufferInfo {
        .buffer = viewProjectionBuffer->buffers[0].buffer,
        .offset = 0,
        .range = viewProjectionBuffer->bufferSize
    };
    writeInfo.emplace_back(VkWriteDescriptorSet {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = descriptorSet,
        .dstBinding = static_cast<uint32_t>(writeInfo.size()),
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .pBufferInfo = &viewProjectionBufferInfo,
    });

    // Texture
    VkDescriptorImageInfo baseColorImageInfo {
        .sampler = mSamplerGroup.sampler,          // TODO Each texture has it's own properties that may need it's own sampler (Not sure yet)
        .imageView = mGpuModel.textures[primitive.baseColorTextureIndex].image_view(),
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };
    writeInfo.emplace_back(VkWriteDescriptorSet {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = descriptorSet,
        .dstBinding = static_cast<uint32_t>(writeInfo.size()),
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .pImageInfo = &baseColorImageInfo,
    });

    // ImageOptions
    VkDescriptorBufferInfo light_view_buffer_info {
        .buffer = imageOptionsBuffer->buffers[0].buffer,
        .offset = 0,
        .range = imageOptionsBuffer->bufferSize
    };
    writeInfo.emplace_back(VkWriteDescriptorSet {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = descriptorSet,
        .dstBinding = static_cast<uint32_t>(writeInfo.size()),
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .pBufferInfo = &light_view_buffer_info,
    });

    RF::UpdateDescriptorSets(
        static_cast<MFA::U8>(writeInfo.size()),
        writeInfo.data()
    );
}
