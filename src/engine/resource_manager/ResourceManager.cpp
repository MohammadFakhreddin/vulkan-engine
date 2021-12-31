#include "ResourceManager.hpp"

#include "engine/render_system/RenderTypes.hpp"
#include "engine/BedrockFileSystem.hpp"
#include "engine/BedrockAssert.hpp"
#include "tools/Importer.hpp"
#include "engine/render_system/RenderFrontend.hpp"
#include "engine/BedrockPath.hpp"

namespace MFA
{

    //-------------------------------------------------------------------------------------------------

    struct State
    {
        std::unordered_map<std::string, std::weak_ptr<RT::GpuModel>> availableGpuModels{};
        std::unordered_map<std::string, std::weak_ptr<AS::Model>> availableCpuModels{};
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
        for (auto & pair : state->availableCpuModels)
        {
            pair.second.reset();
        }
        delete state;
    }

    //-------------------------------------------------------------------------------------------------

    static std::shared_ptr<RT::GpuModel> createGpuModel(AS::Model * cpuModel, char const * name)
    {
        MFA_ASSERT(name != nullptr);
        MFA_ASSERT(strlen(name) > 0);
        MFA_ASSERT(state->availableGpuModels.contains(name) == false);
        MFA_ASSERT(cpuModel != nullptr);
        MFA_ASSERT(cpuModel->mesh->IsValid());

        auto gpuModel = RF::CreateGpuModel(cpuModel, state->nextId++, name);
        state->availableGpuModels[name] = gpuModel;
        return gpuModel;
    }

    //-------------------------------------------------------------------------------------------------

    std::shared_ptr<RT::GpuModel> ResourceManager::AcquireForGpu(char const * nameOrFileAddress, bool const loadFileIfNotExists)
    {
        std::shared_ptr<RT::GpuModel> gpuModel = nullptr;

        auto const findResult = state->availableGpuModels.find(nameOrFileAddress);
        if (findResult != state->availableGpuModels.end())
        {
            gpuModel = findResult->second.lock();
        }

        // It means that file is already loaded and still exists
        if (gpuModel != nullptr )
        {
            return gpuModel;
        }

        auto const cpuModel = AcquireForCpu(nameOrFileAddress, loadFileIfNotExists);
        if (cpuModel == nullptr)
        {
            return nullptr;
        }

        std::string relativePath{};
        if (Path::RelativeToAssetFolder(nameOrFileAddress, relativePath) == false)
        {
            relativePath = nameOrFileAddress;
        }
        return createGpuModel(cpuModel.get(), relativePath.c_str());
    }

    //-------------------------------------------------------------------------------------------------

    std::shared_ptr<AS::Model> ResourceManager::AcquireForCpu(char const * nameOrFileAddress, bool const loadFileIfNotExists)
    {
        std::shared_ptr<AS::Model> cpuModel = nullptr;

        auto const findResult = state->availableCpuModels.find(nameOrFileAddress);
        if (findResult != state->availableCpuModels.end())
        {
            cpuModel = findResult->second.lock();
        }

        if (cpuModel != nullptr || loadFileIfNotExists == false)
        {
            return cpuModel;
        }

        auto const extension = FS::ExtractExtensionFromPath(nameOrFileAddress);
        if (extension == ".gltf" || extension == ".glb")
        {
            cpuModel = Importer::ImportGLTF(Path::ForReadWrite(nameOrFileAddress).c_str());
        } else
        {
            MFA_NOT_IMPLEMENTED_YET("Mohammad Fakhreddin");
        }

        state->availableCpuModels[nameOrFileAddress] = cpuModel;

        return cpuModel;
    }

    //-------------------------------------------------------------------------------------------------

    bool ResourceManager::Assign(std::shared_ptr<AssetSystem::Model> const & cpuModel, char const * name)
    {
        MFA_ASSERT(cpuModel != nullptr);

        auto const findResult = state->availableCpuModels.find(name);
        if (
            findResult != state->availableCpuModels.end() &&
            findResult->second.expired() == false
        )
        {
            MFA_ASSERT(false);
            return false;
        }

        state->availableCpuModels[name] = cpuModel;
        return true;
    }

    //-------------------------------------------------------------------------------------------------

}
