#include "ResourceManager.hpp"

#include "engine/render_system/RenderTypes.hpp"
#include "engine/BedrockAssert.hpp"
#include "tools/Importer.hpp"
#include "engine/render_system/RenderFrontend.hpp"
#include "engine/BedrockPath.hpp"
#include "engine/asset_system/AssetModel.hpp"
#include "tools/ShapeGenerator.hpp"

#include <unordered_map>

namespace MFA::ResourceManager
{

    //-------------------------------------------------------------------------------------------------

    struct State
    {
        std::unordered_map<std::string, std::weak_ptr<AS::Model>> availableCpuModels{};

        std::unordered_map<std::string, std::weak_ptr<RT::GpuTexture>> availableGpuTextures {};
        std::unordered_map<std::string, std::weak_ptr<AS::Texture>> availableCpuTextures {};

        // std::list<ActiveTasksAndCallbacks>
        // If a task is running we add its callback so it can be called too
    };
    State * state = nullptr;

    //-------------------------------------------------------------------------------------------------

    void Init()
    {
        state = new State();
    }

    //-------------------------------------------------------------------------------------------------

    #define CLEAR_MAP(map)                              \
    for (auto & pair : map)                             \
    {                                                   \
        pair.second.reset();                            \
    }                                                   \

    void Shutdown()
    {
        //CLEAR_MAP(state->availableGpuModels)
        CLEAR_MAP(state->availableCpuModels)
        CLEAR_MAP(state->availableGpuTextures)
        CLEAR_MAP(state->availableCpuTextures)
        delete state;
    }

    //-------------------------------------------------------------------------------------------------

    /*static std::shared_ptr<RT::GpuModel> createGpuModel(AS::Model const & cpuModel, std::string const & name)
    {
        MFA_ASSERT(name.empty() == false);
        MFA_ASSERT(cpuModel.mesh->isValid());

        auto const vertexSize = cpuModel.mesh->getVertexData()->memory.len;
        auto const indexSize = cpuModel.mesh->getIndexBuffer()->memory.len;

        if (state->vertexStageBuffer == nullptr || state->vertexStageBuffer->bufferSize < vertexSize)
        {
            state->vertexStageBuffer = RF::CreateStageBuffer(vertexSize, 1);
        }
        if (state->indexStageBuffer == nullptr || state->indexStageBuffer->bufferSize < indexSize)
        {
            state->indexStageBuffer = RF::CreateStageBuffer(indexSize, 1);
        }

        auto const commandBuffer = RF::BeginSingleTimeGraphicCommand();

        auto gpuModel = RF::CreateGpuModel(
            commandBuffer,
            cpuModel,
            name,
        *state->vertexStageBuffer->buffers[0],
            *state->indexStageBuffer->buffers[0]
        );

        RF::EndAndSubmitGraphicSingleTimeCommand(commandBuffer);

        state->availableGpuModels[name] = gpuModel;
        return gpuModel;
    }*/

    //-------------------------------------------------------------------------------------------------
    // TODO Use job system to make this process multi-threaded
    //std::shared_ptr<RT::GpuModel> AcquireGpuModel(std::string const & nameOrFileAddress, bool const loadFileIfNotExists)
    //{
    //    std::shared_ptr<RT::GpuModel> gpuModel = nullptr;

    //    std::string relativePath{};
    //    Path::RelativeToAssetFolder(nameOrFileAddress, relativePath);
    //    
    //    auto const findResult = state->availableGpuModels.find(relativePath);
    //    if (findResult != state->availableGpuModels.end())
    //    {
    //        gpuModel = findResult->second.lock();
    //    }

    //    // It means that file is already loaded and still exists
    //    if (gpuModel != nullptr )
    //    {
    //        return gpuModel;
    //    }

    //    auto const cpuModel = AcquireCpuModel(relativePath, loadFileIfNotExists);
    //    if (cpuModel == nullptr)
    //    {
    //        return nullptr;
    //    }

    //    return createGpuModel(*cpuModel, relativePath);
    //}

    //-------------------------------------------------------------------------------------------------
    // TODO Use job system to make this process multi-threaded
    std::shared_ptr<AS::Model> AcquireCpuModel(
        std::string const & modelId,
        bool const loadFileIfNotExists
    )
    {
        std::shared_ptr<AS::Model> cpuModel = nullptr;

        std::string const relativePath = Path::RelativeToAssetFolder(modelId);
        
        auto const findResult = state->availableCpuModels.find(relativePath);
        if (findResult != state->availableCpuModels.end())
        {
            cpuModel = findResult->second.lock();
        }

        if (cpuModel != nullptr || loadFileIfNotExists == false)
        {
            return cpuModel;
        }

        // TODO: Find a cleaner and more scalable way
        auto const extension = Path::ExtractExtensionFromPath(relativePath);
        if (extension == ".gltf" || extension == ".glb")
        {
            cpuModel = Importer::ImportGLTF(Path::ForReadWrite(relativePath));
        } else if ("CubeStrip" == relativePath)
        {
            cpuModel = ShapeGenerator::Debug::CubeStrip();
        } else if ("CubeFill" == relativePath)
        {
            cpuModel = ShapeGenerator::Debug::CubeFill();
        } else if ("Sphere" == relativePath)
        {
            cpuModel = ShapeGenerator::Debug::Sphere();
        } else
        {
            MFA_NOT_IMPLEMENTED_YET("Mohammad Fakhreddin");
        }

        state->availableCpuModels[relativePath] = cpuModel;

        return cpuModel;
    }

    //-------------------------------------------------------------------------------------------------

    std::vector<std::shared_ptr<RT::GpuTexture>> AcquireGpuTextures(std::vector<std::string> const & textureIds)
    {
        std::vector<std::shared_ptr<RT::GpuTexture>> gpuTextures {};

        for (auto const & textureId : textureIds)
        {
            gpuTextures.emplace_back(AcquireGpuTexture(textureId));
        }

        return gpuTextures;
}

    //-------------------------------------------------------------------------------------------------

    static std::shared_ptr<RT::GpuTexture> createGpuTexture(AS::Texture * texture, std::string const & name)
    {
        MFA_ASSERT(name.empty() == false);
        MFA_ASSERT(texture != nullptr);
        MFA_ASSERT(texture->isValid());

        auto gpuTexture = RF::CreateTexture(*texture);
        state->availableGpuTextures[name] = gpuTexture;
        return gpuTexture;
    }

    //-------------------------------------------------------------------------------------------------
    // TODO: Idea: Acquire Gltf mesh separately with list of textures. Then acquire textures when needed.
    // // User should not use importer directly. Everything should be through resource manager
    std::shared_ptr<RT::GpuTexture> AcquireGpuTexture(
        std::string const & textureId,
        bool const loadFileIfNotExists
    )
    {
        std::shared_ptr<RT::GpuTexture> gpuTexture = nullptr;

        std::string const relativePath = Path::RelativeToAssetFolder(textureId);

        auto const findResult = state->availableGpuTextures.find(relativePath);
        if (findResult != state->availableGpuTextures.end())
        {
            gpuTexture = findResult->second.lock();
        }

        // It means that file is already loaded and still exists
        if (gpuTexture != nullptr )
        {
            return gpuTexture;
        }

        auto const cpuTexture = AcquireCpuTexture(relativePath, loadFileIfNotExists);
        if (cpuTexture == nullptr)
        {
            return nullptr;
        }

        return createGpuTexture(cpuTexture.get(), textureId);
    }

    //-------------------------------------------------------------------------------------------------

    std::shared_ptr<AssetSystem::Texture> AcquireCpuTexture(
        std::string const & textureId,
        bool const loadFileIfNotExists
    )
    {
        std::shared_ptr<AS::Texture> texture = nullptr;

        std::string const relativePath = Path::RelativeToAssetFolder(textureId);

        auto const findResult = state->availableCpuTextures.find(relativePath);
        if (findResult != state->availableCpuTextures.end())
        {
            texture = findResult->second.lock();
        }

        if (texture != nullptr || loadFileIfNotExists == false)
        {
            return texture;
        }

        auto const extension = Path::ExtractExtensionFromPath(relativePath);
        if (extension == ".ktx")
        {
            texture = Importer::ImportKTXImage(Path::ForReadWrite(relativePath));
        } else if (extension == ".png" || extension == ".jpg" || extension == ".jpeg")
        {
            texture = Importer::ImportUncompressedImage(Path::ForReadWrite(relativePath));
        } else if (textureId == "Error")
        {
            texture = Importer::CreateErrorTexture();
        } else
        {
            MFA_NOT_IMPLEMENTED_YET("Mohammad Fakhreddin");
        }

        state->availableCpuTextures[relativePath] = texture;

        return texture;
    }

    //-------------------------------------------------------------------------------------------------

}
