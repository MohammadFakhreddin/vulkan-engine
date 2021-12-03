#include "ResourceManager.hpp"

#include "engine/render_system/RenderTypes.hpp"
#include "engine/BedrockFileSystem.hpp"
#include "engine/BedrockAssert.hpp"
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

    static std::shared_ptr<RT::GpuModel> createGpuModel(AssetSystem::Model & cpuModel, char const * name)
    {
        MFA_ASSERT(name != nullptr);
        MFA_ASSERT(strlen(name) > 0);

        MFA_ASSERT(cpuModel.mesh.IsValid());
        auto gpuModel = RF::CreateGpuModel(cpuModel, state->nextId++);
        MFA_ASSERT(gpuModel->valid);
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
            return createGpuModel(cpuModel, fileAddress);
        }

        MFA_NOT_IMPLEMENTED_YET("Mohammad Fakhreddin");
        return nullptr;
    }

    //-------------------------------------------------------------------------------------------------

    std::shared_ptr<RT::GpuModel> ResourceManager::Assign(AssetSystem::Model & cpuModel, char const * name)
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

        return createGpuModel(cpuModel, name);
    }

    //-------------------------------------------------------------------------------------------------

}
