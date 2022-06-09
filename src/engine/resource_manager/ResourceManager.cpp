#include "ResourceManager.hpp"

#include "engine/render_system/RenderTypes.hpp"
#include "engine/BedrockAssert.hpp"
#include "tools/Importer.hpp"
#include "engine/render_system/RenderFrontend.hpp"
#include "engine/BedrockPath.hpp"
#include "engine/asset_system/AssetModel.hpp"
#include "tools/ShapeGenerator.hpp"
#include "engine/job_system/ScopeLock.hpp"
#include "engine/job_system/JobSystem.hpp"
#include "engine/render_system/pipelines/EssenceBase.hpp"
#include "engine/render_system/pipelines/pbr_with_shadow_v2/PBR_Essence.hpp"
#include "engine/render_system/pipelines/BasePipeline.hpp"
#include "engine/render_system/pipelines/debug_renderer/DebugEssence.hpp"
#include "engine/render_system/pipelines/debug_renderer/DebugRendererPipeline.hpp"
#include "engine/render_system/pipelines/particle/ParticlePipeline.hpp"
#include "engine/render_system/pipelines/pbr_with_shadow_v2/PbrWithShadowPipelineV2.hpp"
#include "engine/scene_manager/SceneManager.hpp"

#include <unordered_map>

namespace MFA::ResourceManager
{

    //-------------------------------------------------------------------------------------------------

    template<typename DataType, typename CallbackType>
    struct Data
    {
        std::weak_ptr<DataType> data {};
        std::list<CallbackType> callbacks {};
        std::atomic<bool> lock = false;
    };

    //-------------------------------------------------------------------------------------------------

    struct CpuModelData : Data<AS::Model, CpuModelCallback> {};

    //-------------------------------------------------------------------------------------------------

    struct CpuTextureData : Data<AS::Texture, CpuTextureCallback> {};

    //-------------------------------------------------------------------------------------------------

    struct GpuTextureData : Data<RT::GpuTexture, GpuTextureCallback> {};

    //-------------------------------------------------------------------------------------------------

    struct EssenceData : Data<EssenceBase, std::function<void(bool)>> {};

    //-------------------------------------------------------------------------------------------------

    struct State
    {
        std::unordered_map<std::string, CpuModelData> cpuModels{};

        std::unordered_map<std::string, GpuTextureData> gpuTextures {};
        std::unordered_map<std::string, CpuTextureData> cpuTextures {};

        std::unordered_map<std::string, EssenceData> essences {};

    };
    State * state = nullptr;

    //-------------------------------------------------------------------------------------------------

    void Init()
    {
        state = new State();
    }

    //-------------------------------------------------------------------------------------------------

    void Shutdown()
    {
        state->cpuModels.clear();
        state->gpuTextures.clear();
        state->cpuTextures.clear();
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
    //std::shared_ptr<RT::GpuModel> ResourceManager::AcquireGpuModel(std::string const & nameOrFileAddress, bool const loadFileIfNotExists)
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

    void AcquireCpuModel(
        std::string const & modelId,
        CpuModelCallback const & callback,
        bool const loadFileIfNotExists
    )
    {
        std::shared_ptr<AS::Model> cpuModel = nullptr;

        std::string const relativePath = Path::RelativeToAssetFolder(modelId);

        auto & cpuModelData = state->cpuModels[relativePath];

        SCOPE_LOCK(cpuModelData.lock)

        cpuModel = cpuModelData.data.lock();

        if (cpuModel != nullptr || loadFileIfNotExists == false)
        {
            callback(cpuModel);
            return;
        }

        cpuModelData.callbacks.emplace_back(callback);

        if (cpuModelData.callbacks.size() == 1)
        {
            JS::AssignTask([relativePath, &cpuModelData](JS::ThreadNumber threadNumber, JS::ThreadNumber totalThreadCount)->void {

                std::shared_ptr<AS::Model> cpuModel = nullptr;

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

                SCOPE_LOCK(cpuModelData.lock)

                cpuModelData.data = cpuModel;

                for (auto & callback : cpuModelData.callbacks)
                {
                    callback(cpuModel);
                }

                cpuModelData.callbacks.clear();

            });
        }
    }

    //-------------------------------------------------------------------------------------------------

    //std::vector<std::shared_ptr<RT::GpuTexture>> AcquireGpuTextures(std::vector<std::string> const & textureIds)
    //{
    //    std::vector<std::shared_ptr<RT::GpuTexture>> gpuTextures {};

    //    for (auto const & textureId : textureIds)
    //    {
    //        gpuTextures.emplace_back(AcquireGpuTexture(textureId));
    //    }

    //    return gpuTextures;
    //}

    //-------------------------------------------------------------------------------------------------

    // User should not use importer directly. Everything should be through resource manager
    void AcquireGpuTexture(
        std::string const & textureId,
        GpuTextureCallback const & callback,
        bool const loadFromFile
    )
    {
        MFA_ASSERT(textureId.empty() == false);
        MFA_ASSERT(callback != nullptr);
        
        std::string const relativePath = Path::RelativeToAssetFolder(textureId);

        //auto const findResult = state->gpuTextures.find(relativePath);

        auto & gpuTextureData = state->gpuTextures[relativePath];

        SCOPE_LOCK(gpuTextureData.lock)

        auto const gpuTexture = gpuTextureData.data.lock();

        /*if (findResult != state->gpuTextures.end())
        {
            gpuTexture = findResult->second.lock();
        }*/

        // It means that file is already loaded and still exists
        if (gpuTexture != nullptr )
        {
            callback(gpuTexture);
            return;
        }

        gpuTextureData.callbacks.emplace_back(callback);

        if (gpuTextureData.callbacks.size() == 1)
        {
            AcquireCpuTexture(
                relativePath,
                [relativePath, &gpuTextureData]
                (std::shared_ptr<AS::Texture> const & texture)->void{
                    SceneManager::AssignMainThreadTask([relativePath, &gpuTextureData, texture]()->void {
                        MFA_ASSERT(texture != nullptr);
                        
                        SCOPE_LOCK(gpuTextureData.lock)

                        auto const gpuTexture = RF::CreateTexture(*texture);
                        gpuTextureData.data = gpuTexture;

                        for (auto & callback : gpuTextureData.callbacks)
                        {
                            // TODO: What if the object holding this callback is dead ?
                            callback(gpuTexture);
                        }
                        gpuTextureData.callbacks.clear();
                    });
                },
                loadFromFile
            );
        }
    }

    //-------------------------------------------------------------------------------------------------

    void AcquireCpuTexture(
        std::string const & textureId,
        CpuTextureCallback const & callback,
        bool const loadFromFile
    )
    {
        std::string const relativePath = Path::RelativeToAssetFolder(textureId);

        auto & textureData = state->cpuTextures[relativePath];

        SCOPE_LOCK(textureData.lock)

        auto const texture = textureData.data.lock();

        if (texture != nullptr || loadFromFile == false)
        {
            callback(texture);
            return;
        }

        textureData.callbacks.emplace_back(callback);

        if (textureData.callbacks.size() == 1)
        {
            JS::AssignTask([relativePath, &textureData](JS::ThreadNumber threadNumber, JS::ThreadNumber totalThreadCount)->void {

                std::shared_ptr<AssetSystem::Texture> texture = nullptr;

                auto const extension = Path::ExtractExtensionFromPath(relativePath);

                if (extension == ".ktx")
                {
                    texture = Importer::ImportKTXImage(Path::ForReadWrite(relativePath));
                } else if (extension == ".png" || extension == ".jpg" || extension == ".jpeg")
                {
                    texture = Importer::ImportUncompressedImage(Path::ForReadWrite(relativePath));
                } else if (relativePath == "Error")
                {
                    texture = Importer::CreateErrorTexture();
                } else
                {
                    MFA_NOT_IMPLEMENTED_YET("Mohammad Fakhreddin");
                }

                SCOPE_LOCK(textureData.lock)

                textureData.data = texture;

                for (auto & callback : textureData.callbacks)
                {
                    callback(texture);
                }

                textureData.callbacks.clear();

            });
        }
    }

    //-------------------------------------------------------------------------------------------------

    bool createPbrEssence(
        BasePipeline * pipeline,
        std::string const & nameId,
        std::shared_ptr<AS::Model> const & cpuModel,
        std::vector<std::shared_ptr<RT::GpuTexture>> const & gpuTextures,
        std::shared_ptr<EssenceBase> & essence
    )
    {
        MFA_ASSERT(JS::IsMainThread());

        auto const * pbrMesh = static_cast<AS::PBR::Mesh *>(cpuModel->mesh.get());

        essence = std::make_shared<PBR_Essence>(
            nameId,
            *pbrMesh,
            gpuTextures
        );

        auto const addResult = pipeline->addEssence(essence);

        return addResult;
    }
    
    //-------------------------------------------------------------------------------------------------

    bool createDebugEssence(
        BasePipeline * pipeline,
        std::string const & nameId,
        std::shared_ptr<AssetSystem::Model> const & cpuModel,
        std::shared_ptr<EssenceBase> & essence
    )
    {
        MFA_ASSERT(JS::IsMainThread());

        auto const * debugMesh = static_cast<AS::Debug::Mesh *>(cpuModel->mesh.get());

        essence = std::make_shared<DebugEssence>(
            nameId,
            *debugMesh
        );

        auto const addResult = pipeline->addEssence(essence);
        MFA_ASSERT(addResult == true);

        return addResult;
    }
    
    //-------------------------------------------------------------------------------------------------

    static void CreateEssence(
        BasePipeline * pipeline,
        std::string const & nameId,
        std::shared_ptr<AS::Model> const & cpuModel,
        std::vector<std::shared_ptr<RT::GpuTexture>> const & gpuTextures
    )
    {
        SceneManager::AssignMainThreadTask([pipeline, nameId, cpuModel, gpuTextures]()->void{

            auto const pipelineName = pipeline->GetName();

            std::shared_ptr<EssenceBase> essence = nullptr;
            bool addResult = false;

            if (pipelineName == PBRWithShadowPipelineV2::Name)
            {
                addResult = createPbrEssence(pipeline, nameId, cpuModel, gpuTextures, essence);
            } else if (pipelineName == ParticlePipeline::Name)
            {
                MFA_NOT_IMPLEMENTED_YET("Mohammad Fakhreddin");
            }
            else if (pipelineName == DebugRendererPipeline::Name)
            {
                addResult = createDebugEssence(pipeline, nameId, cpuModel, essence);
            }
            else
            {
                MFA_CRASH("Unhandled pipeline type detected");
            }

            auto & essenceData = state->essences[nameId];

            SCOPE_LOCK(essenceData.lock)

            essenceData.data = essence;

            bool const success = addResult && essence != nullptr;

            for (auto & callback : essenceData.callbacks)
            {
                callback(success);
            }

            essenceData.callbacks.clear();
        });
    }

    //-------------------------------------------------------------------------------------------------

    void AcquireEssence(
        std::string const & nameId,
        BasePipeline * pipeline,
        EssenceCallback const & callback,
        bool loadFromFile
    )
    {
        MFA_ASSERT(nameId.length() > 0);
        MFA_ASSERT(pipeline != nullptr);
        MFA_ASSERT(callback != nullptr);

        std::shared_ptr<EssenceBase> essence = nullptr;

        std::string const relativePath = Path::RelativeToAssetFolder(nameId);

        auto & essenceData = state->essences[relativePath];

        SCOPE_LOCK(essenceData.lock)

        essence = essenceData.data.lock();

        if (essence != nullptr)
        {
            callback(true);
            return;
        }

        essence = pipeline->GetEssence(nameId);

        if (essence != nullptr)
        {
            essenceData.data = essence;
            callback(true);
            return;
        }

        essenceData.callbacks.emplace_back(callback);

        if (essenceData.callbacks.size() == 1)
        {
            RC::AcquireCpuModel(
                nameId,
                [nameId, pipeline, loadFromFile]
                (std::shared_ptr<AS::Model> const & cpuModel)->void {

                    MFA_ASSERT(cpuModel != nullptr);
                    
                    if (cpuModel->textureIds.empty() == false)
                    {
                        class Tracker : public JS::TaskTracker
                        {
                            using TaskTracker::TaskTracker;
                        public:
                            std::vector<std::shared_ptr<RT::GpuTexture>> gpuTextures {};
                        };

                        std::shared_ptr<Tracker> taskTracker = std::make_shared<Tracker>(cpuModel->textureIds.size());

                        taskTracker->setFinishCallback([pipeline, nameId, taskTracker, cpuModel]()->void {
                            CreateEssence(pipeline, nameId, cpuModel, taskTracker->gpuTextures);
                        });

                        taskTracker->gpuTextures.resize (cpuModel->textureIds.size());

                        for (size_t i = 0; i < taskTracker->gpuTextures.size(); ++i)
                        {
                            RC::AcquireGpuTexture(
                                cpuModel->textureIds[i],
                                [taskTracker, i](std::shared_ptr<RT::GpuTexture> const & gpuTexture)->void {
                                    taskTracker->gpuTextures[i] = gpuTexture;
                                    taskTracker->onComplete();
                                }, loadFromFile
                            );
                        }
                    }
                    else
                    {
                        CreateEssence(pipeline, nameId, cpuModel, {});
                    }
                }, loadFromFile
            );
        }
    }

    //-------------------------------------------------------------------------------------------------

}
