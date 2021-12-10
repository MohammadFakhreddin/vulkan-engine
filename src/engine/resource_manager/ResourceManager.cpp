#include "ResourceManager.hpp"

#include "engine/render_system/RenderTypes.hpp"
#include "engine/BedrockFileSystem.hpp"
#include "engine/BedrockAssert.hpp"
#include "engine/BedrockMemory.hpp"
#include "tools/Importer.hpp"
#include "engine/render_system/RenderFrontend.hpp"

namespace MFA
{

    //-------------------------------------------------------------------------------------------------

    struct State
    {
        std::unordered_map<std::string, std::weak_ptr<RT::GpuModel>> availableGpuModels{};
        RT::GpuModelId nextId = 0;
    };
    State * state = nullptr;

    //-------------------------------------------------------------------------------------------------

    void ResourceManager::Init()
    {
        state = new State();
    }

    //-------------------------------------------------------------------------------------------------

    void ResourceManager::Shutdown()
    {
        for (auto & pair : state->availableGpuModels)
        {
            pair.second.reset();
        }
        delete state;
    }

    //-------------------------------------------------------------------------------------------------

    static std::shared_ptr<RT::GpuModel> createGpuModel(std::shared_ptr<AS::Model> cpuModel, char const * name)
    {
        MFA_ASSERT(name != nullptr);
        MFA_ASSERT(strlen(name) > 0);

        MFA_ASSERT(cpuModel->mesh->IsValid());
        auto gpuModel = RF::CreateGpuModel(std::move(cpuModel), state->nextId++);
        state->availableGpuModels[name] = gpuModel;
        return gpuModel;
    }

    //-------------------------------------------------------------------------------------------------

    std::shared_ptr<RT::GpuModel> ResourceManager::Acquire(char const * fileAddress, bool const loadFileIfNotExists)
    {
        std::shared_ptr<RT::GpuModel> gpuModel = nullptr;

        auto const findResult = state->availableGpuModels.find(fileAddress);
        if (findResult != state->availableGpuModels.end())
        {
            gpuModel = findResult->second.lock();
        }

        // It means that file is already loaded and still exists
        if (gpuModel != nullptr || loadFileIfNotExists == false)
        {
            return gpuModel;
        }

        auto const extension = FS::ExtractExtensionFromPath(fileAddress);
        if (extension == ".gltf" || extension == ".glb")
        {
            auto cpuModel = Importer::ImportGLTF(fileAddress);
            if (cpuModel != nullptr)
            {
                return createGpuModel(cpuModel, fileAddress);
            }
        }

        MFA_NOT_IMPLEMENTED_YET("Mohammad Fakhreddin");
        return nullptr;
    }

    //-------------------------------------------------------------------------------------------------

    std::shared_ptr<RT::GpuModel> ResourceManager::Assign(std::shared_ptr<AssetSystem::Model> cpuModel, char const * name)
    {
        auto const findResult = state->availableGpuModels.find(name);
        if (
            findResult != state->availableGpuModels.end() &&
            findResult->second.expired() == false
        )
        {
            MFA_ASSERT(false);
            return nullptr;
        }

        return createGpuModel(std::move(cpuModel), name);
    }

    //-------------------------------------------------------------------------------------------------

    //bool ResourceManager::FreeModel(AS::Model & model)
    //{
    //    bool success = false;
    //    bool freeResult = false;
    //    // Mesh
    //    freeResult = FreeMesh(model.mesh);
    //    MFA_ASSERT(freeResult);
    //    // Textures
    //    if (false == model.textures.empty())
    //    {
    //        for (auto & texture : model.textures)
    //        {
    //            freeResult = FreeTexture(texture);
    //            MFA_ASSERT(freeResult);
    //        }
    //    }
    //    success = true;
    //    return success;
    //}

    ////-------------------------------------------------------------------------------------------------

    //bool ResourceManager::FreeTexture(AS::Texture & texture)
    //{
    //    bool success = false;
    //    MFA_ASSERT(texture.isValid());
    //    if (texture.isValid())
    //    {
    //        // TODO This is RCMGMT task
    //        Memory::Free(texture.revokeBuffer());
    //        success = true;
    //    }
    //    return success;
    //}

    ////-------------------------------------------------------------------------------------------------

    //bool ResourceManager::FreeShader(AS::Shader & shader)
    //{
    //    bool success = false;
    //    MFA_ASSERT(shader.isValid());
    //    if (shader.isValid())
    //    {
    //        Memory::Free(shader.revokeData());
    //        success = true;
    //    }
    //    return success;
    //}

    ////-------------------------------------------------------------------------------------------------

    //bool ResourceManager::FreeMesh(AS::Mesh & mesh)
    //{
    //    bool success = false;
    //    MFA_ASSERT(mesh.IsValid());
    //    if (mesh.IsValid())
    //    {
    //        Blob vertexBuffer{};
    //        Blob indexBuffer{};
    //        mesh.RevokeBuffers(vertexBuffer, indexBuffer);
    //        Memory::Free(vertexBuffer);
    //        Memory::Free(indexBuffer);
    //        success = true;
    //    }
    //    return success;
    //}

    //-------------------------------------------------------------------------------------------------

}
