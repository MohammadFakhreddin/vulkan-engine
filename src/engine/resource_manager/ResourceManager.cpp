#include "ResourceManager.hpp"

#include "engine/render_system/RenderTypes.hpp"
#include "engine/BedrockFileSystem.hpp"
#include "engine/BedrockAssert.hpp"
#include "tools/Importer.hpp"
#include "engine/render_system/RenderFrontend.hpp"
#include "engine/BedrockPath.hpp"
#include "engine/asset_system/AssetModel.hpp"
#include "tools/ShapeGenerator.hpp"

namespace MFA
{

    //-------------------------------------------------------------------------------------------------

    struct State
    {
        std::unordered_map<std::string, std::weak_ptr<RT::GpuModel>> availableGpuModels{};
        std::unordered_map<std::string, std::weak_ptr<AS::Model>> availableCpuModels{};

        std::unordered_map<std::string, std::weak_ptr<RT::GpuTexture>> availableGpuTextures {};
        std::unordered_map<std::string, std::weak_ptr<AS::Texture>> availableCpuTextures {};
    };
    State * state = nullptr;

    //-------------------------------------------------------------------------------------------------

    void ResourceManager::Init()
    {
        state = new State();
    }

    //-------------------------------------------------------------------------------------------------

    #define CLEAR_MAP(map)                              \
    for (auto & pair : state->availableGpuModels)       \
    {                                                   \
        pair.second.reset();                            \
    }                                                   \

    void ResourceManager::Shutdown()
    {
        CLEAR_MAP(state->availableGpuModels)
        CLEAR_MAP(state->availableCpuModels)
        CLEAR_MAP(state->availableGpuTextures)
        CLEAR_MAP(state->availableCpuTextures)
        delete state;
    }

    //-------------------------------------------------------------------------------------------------

    static std::shared_ptr<RT::GpuModel> createGpuModel(AS::Model * cpuModel, std::string const & name)
    {
        MFA_ASSERT(name.empty() == false);
        MFA_ASSERT(cpuModel != nullptr);
        MFA_ASSERT(cpuModel->mesh->isValid());

        auto gpuModel = RF::CreateGpuModel(cpuModel, name);
        state->availableGpuModels[name] = gpuModel;
        return gpuModel;
    }

    //-------------------------------------------------------------------------------------------------
    // TODO Use job system to make this process multi-threaded
    std::shared_ptr<RT::GpuModel> ResourceManager::AcquireGpuModel(std::string const & nameOrFileAddress, bool const loadFileIfNotExists)
    {
        std::shared_ptr<RT::GpuModel> gpuModel = nullptr;

        std::string relativePath{};
        Path::RelativeToAssetFolder(nameOrFileAddress, relativePath);
        
        auto const findResult = state->availableGpuModels.find(relativePath);
        if (findResult != state->availableGpuModels.end())
        {
            gpuModel = findResult->second.lock();
        }

        // It means that file is already loaded and still exists
        if (gpuModel != nullptr )
        {
            return gpuModel;
        }

        auto const cpuModel = AcquireCpuModel(relativePath, loadFileIfNotExists);
        if (cpuModel == nullptr)
        {
            return nullptr;
        }

        return createGpuModel(cpuModel.get(), relativePath);
    }

    //-------------------------------------------------------------------------------------------------
    // TODO Use job system to make this process multi-threaded
    std::shared_ptr<AS::Model> ResourceManager::AcquireCpuModel(std::string const & nameOrFileAddress, bool const loadFileIfNotExists)
    {
        std::shared_ptr<AS::Model> cpuModel = nullptr;

        std::string relativePath{};
        Path::RelativeToAssetFolder(nameOrFileAddress, relativePath);
        
        auto const findResult = state->availableCpuModels.find(relativePath);
        if (findResult != state->availableCpuModels.end())
        {
            cpuModel = findResult->second.lock();
        }

        if (cpuModel != nullptr || loadFileIfNotExists == false)
        {
            return cpuModel;
        }

        auto const extension = FS::ExtractExtensionFromPath(relativePath);
        if (extension == ".gltf" || extension == ".glb")
        {
            cpuModel = Importer::ImportGLTF(Path::ForReadWrite(relativePath));
        } else if (strcmp("Cube", relativePath.c_str()) == 0)
        {
            cpuModel = ShapeGenerator::Debug::Cube();
        } else if (strcmp("Sphere", relativePath.c_str()) == 0)
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

    std::shared_ptr<RT::GpuTexture> ResourceManager::AcquireGpuTexture(
        std::string const & nameOrFileAddress,
        bool const loadFileIfNotExists
    )
    {
        std::shared_ptr<RT::GpuTexture> gpuTexture = nullptr;

        std::string relativePath{};
        Path::RelativeToAssetFolder(nameOrFileAddress, relativePath);

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

        return createGpuTexture(cpuTexture.get(), nameOrFileAddress);
    }

    //-------------------------------------------------------------------------------------------------

    std::shared_ptr<AssetSystem::Texture> ResourceManager::AcquireCpuTexture(
        std::string const & nameOrFileAddress,
        bool const loadFileIfNotExists
    )
    {
        std::shared_ptr<AS::Texture> texture = nullptr;

        std::string relativePath{};
        Path::RelativeToAssetFolder(nameOrFileAddress, relativePath);

        auto const findResult = state->availableCpuTextures.find(relativePath);
        if (findResult != state->availableCpuTextures.end())
        {
            texture = findResult->second.lock();
        }

        if (texture != nullptr || loadFileIfNotExists == false)
        {
            return texture;
        }

        auto const extension = FS::ExtractExtensionFromPath(relativePath);
        if (extension == ".ktx")
        {
            texture = Importer::ImportKTXImage(Path::ForReadWrite(relativePath));
        } else if (extension == ".png" || extension == ".jpg")
        {
            texture = Importer::ImportUncompressedImage(Path::ForReadWrite(relativePath));
        } else if (nameOrFileAddress == "Error")
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

    //bool ResourceManager::Assign(std::shared_ptr<AssetSystem::Model> const & cpuModel, std::string const & name)
    //{
    //    MFA_ASSERT(cpuModel != nullptr);

    //    auto const findResult = state->availableCpuModels.find(name);
    //    if (
    //        findResult != state->availableCpuModels.end() &&
    //        findResult->second.expired() == false
    //    )
    //    {
    //        MFA_ASSERT(false);
    //        return false;
    //    }

    //    state->availableCpuModels[name] = cpuModel;
    //    return true;
    //}

    //-------------------------------------------------------------------------------------------------

}
