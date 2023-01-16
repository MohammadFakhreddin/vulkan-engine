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
#include "engine/render_system/pipelines/BaseMaterial.hpp"
#include "engine/scene_manager/SceneManager.hpp"
#include "engine/job_system/TaskTracker.hpp"
#include "engine/physics/PhysicsTypes.hpp"
#include "engine/physics/Physics.hpp"

#include <cooking/PxTriangleMeshDesc.h>

#include <unordered_map>
#include <cooking/PxCooking.h>

namespace MFA::ResourceManager
{

    //-------------------------------------------------------------------------------------------------

    template<typename DataType, typename CallbackType>
    struct Data
    {
        std::weak_ptr<DataType> data {};
        std::vector<CallbackType> callbacks {};
        std::atomic<bool> lock = false;
    };

    //-------------------------------------------------------------------------------------------------

    struct CpuModelData : Data<AS::Model, CpuModelCallback> {};

    //-------------------------------------------------------------------------------------------------

    struct CpuTextureData : Data<AS::Texture, CpuTextureCallback> {};

    //-------------------------------------------------------------------------------------------------

    struct GpuTextureData : Data<RT::GpuTexture, GpuTextureCallback> {};

    //-------------------------------------------------------------------------------------------------

    struct EssenceData : Data<EssenceBase, EssenceCallback> {};

    //-------------------------------------------------------------------------------------------------

    struct PhysicsMeshData : Data<Physics::TriangleMeshGroup, PhysicsMeshCallback> {};

    //-------------------------------------------------------------------------------------------------

    struct State
    {
        std::unordered_map<std::string, CpuModelData> cpuModels{};

        std::unordered_map<std::string, GpuTextureData> gpuTextures {};
        std::unordered_map<std::string, CpuTextureData> cpuTextures {};

        std::unordered_map<std::string, EssenceData> essences {};

        std::unordered_map<std::string, PhysicsMeshData> physicsMeshes {};
    
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
        
//        for (auto & cpuTexture : state->cpuTextures)
//        {
//            MFA_ASSERT(cpuTexture.second.callbacks.empty());
//            MFA_ASSERT(cpuTexture.second.data.lock() == nullptr);
//        }
        state->cpuTextures.clear();

//        for (auto & essence : state->essences)
//        {
//            MFA_ASSERT(essence.second.callbacks.empty());
//            MFA_ASSERT(essence.second.data.lock() == nullptr);
//        }
        state->essences.clear();

//        for (auto & texture : state->gpuTextures)
//        {
//            MFA_ASSERT(texture.second.callbacks.empty());
//            MFA_ASSERT(texture.second.data.lock() == nullptr);
//        }
        state->gpuTextures.clear();

//        for (auto & cpuModel : state->cpuModels)
//        {
//            MFA_ASSERT(cpuModel.second.callbacks.empty());
//            MFA_ASSERT(cpuModel.second.data.lock() == nullptr);
//        }
        state->cpuModels.clear();

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

    void AcquireCpuModel(
        std::string const & modelId,
        CpuModelCallback const & callback,
        bool const loadFromFile
    )
    {
        std::string const relativePath = Path::RelativeToAssetFolder(modelId);

        auto & cpuModelData = state->cpuModels[relativePath];

        bool shouldAssignTask;
        {
            SCOPE_LOCK(cpuModelData.lock)

            auto const cpuModel = cpuModelData.data.lock();

            if (cpuModel != nullptr || loadFromFile == false)
            {
                callback(cpuModel);
                return;
            }

            cpuModelData.callbacks.emplace_back(callback);

            shouldAssignTask = cpuModelData.callbacks.size() == 1;
        }

        if (shouldAssignTask)
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

    // // User should not use importer directly. Everything should be through resource manager
    void AcquireGpuTexture(
        std::string const & textureId,
        GpuTextureCallback const & callback,
        bool const loadFromFile
    )
    {
        MFA_ASSERT(textureId.empty() == false);
        MFA_ASSERT(callback != nullptr);
        
        std::string const relativePath = Path::RelativeToAssetFolder(textureId);

        auto & gpuTextureData = state->gpuTextures[relativePath];

        bool shouldAssignTask;
        {
            SCOPE_LOCK(gpuTextureData.lock)

            auto const gpuTexture = gpuTextureData.data.lock();

            // It means that file is already loaded and still exists
            if (gpuTexture != nullptr)
            {
                callback(gpuTexture);
                return;
            }

            gpuTextureData.callbacks.emplace_back(callback);

            shouldAssignTask = gpuTextureData.callbacks.size() == 1;
        }

        if (shouldAssignTask)
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

        bool shouldAssignTask;
        {
            SCOPE_LOCK(textureData.lock)

            auto const existingTexture = textureData.data.lock();

            if (existingTexture != nullptr || loadFromFile == false)
            {
                callback(existingTexture);
                return;
            }

            textureData.callbacks.emplace_back(callback);

            shouldAssignTask = textureData.callbacks.size() == 1;
        }

        if (shouldAssignTask)
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

    static void CreateEssence(
        BaseMaterial * pipeline,
        std::string const & nameId,
        std::shared_ptr<AS::Model> const & cpuModel,
        std::vector<std::shared_ptr<RT::GpuTexture>> const & gpuTextures
    )
    {
        SceneManager::AssignMainThreadTask([pipeline, nameId, cpuModel, gpuTextures]()->void{

            auto const essence = pipeline->CreateEssence(nameId, cpuModel, gpuTextures);

            auto & essenceData = state->essences[nameId];

            SCOPE_LOCK(essenceData.lock)

            essenceData.data = essence;

            bool const success = essence != nullptr;

            for (auto & callback : essenceData.callbacks)
            {
                callback(success);
            }

            essenceData.callbacks.clear();
        });
    }

    //-------------------------------------------------------------------------------------------------

    void AcquireEssence(
        std::string const & path,
        BaseMaterial * pipeline,
        EssenceCallback const & callback,
        bool loadFromFile
    )
    {
        MFA_ASSERT(path.length() > 0);
        MFA_ASSERT(pipeline != nullptr);
        MFA_ASSERT(callback != nullptr);

        std::shared_ptr<EssenceBase> essence = nullptr;

        auto const nameId = Path::RelativeToAssetFolder(path);

        auto & essenceData = state->essences[nameId];

        bool shouldAssignTask;
        {
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

            shouldAssignTask = essenceData.callbacks.size() == 1;
        }

        if (shouldAssignTask)
        {
            RC::AcquireCpuModel(
                nameId,
                [nameId, pipeline, loadFromFile]
                (std::shared_ptr<AS::Model> const & cpuModel)->void {

                    MFA_ASSERT(cpuModel != nullptr);
                    
                    if (cpuModel->textureIds.empty() == false)
                    {
                        struct Data
                        {
                            std::vector<std::shared_ptr<RT::GpuTexture>> gpuTextures {};
                        };

                        auto taskTracker = std::make_shared<JS::TaskTracker1<Data>>(
                            std::make_shared<Data>(),
                            static_cast<int>(cpuModel->textureIds.size()),
                            [pipeline, nameId, cpuModel](Data const * userData)->void {
                                CreateEssence(pipeline, nameId, cpuModel, userData->gpuTextures);
                            }
                        );
                        
                        taskTracker->userData->gpuTextures.resize (cpuModel->textureIds.size());

                        for (size_t i = 0; i < taskTracker->userData->gpuTextures.size(); ++i)
                        {
                            RC::AcquireGpuTexture(
                                cpuModel->textureIds[i],
                                [taskTracker, i](std::shared_ptr<RT::GpuTexture> const & gpuTexture)->void {
                                    taskTracker->userData->gpuTextures[i] = gpuTexture;
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

    void AcquirePhysicsMesh(
        std::string const & path,
        bool const isConvex,            // Is not used for now
        PhysicsMeshCallback const & callback,
        bool const loadFromFile
    )
    {
        MFA_ASSERT(path.empty() == false);
        MFA_ASSERT(callback != nullptr);

        std::string const nameId = Path::RelativeToAssetFolder(path);

        auto & meshMap = state->physicsMeshes;
        auto & meshData = meshMap[nameId];

        bool shouldAssignTask;
        {
            SCOPE_LOCK(meshData.lock)
            auto const physicsMesh = meshData.data.lock();
            if (physicsMesh != nullptr)
            {
                callback(physicsMesh);
                return;
            }
            meshData.callbacks.emplace_back(callback);
            shouldAssignTask = meshData.callbacks.size() == 1;
        }

        if (shouldAssignTask)
        {
            AcquireCpuModel(
                nameId,
                [isConvex, &meshData, nameId](std::shared_ptr<AS::Model> const & cpuModel)->void{

                    auto const & mesh = cpuModel->mesh;

                    mesh->PreparePhysicsPoints([mesh, isConvex, &meshData, nameId](std::vector<Physics::TriangleMeshDesc> const & meshDescList)->void{

                        auto const meshGroup = std::make_shared<Physics::TriangleMeshGroup>();

                        for (auto const & meshDesc : meshDescList)
                        {
                            physx::PxTriangleMeshDesc pxMeshDesc;
                            pxMeshDesc.setToDefault();
                            pxMeshDesc.triangles.count = meshDesc.trianglesCount;
                            pxMeshDesc.triangles.stride = meshDesc.trianglesStride;
                            pxMeshDesc.triangles.data = meshDesc.triangleBuffer->memory.ptr;

                            pxMeshDesc.points.count = meshDesc.pointsCount;
                            pxMeshDesc.points.stride = meshDesc.pointsStride;
                            pxMeshDesc.points.data = meshDesc.pointsBuffer->memory.ptr;

                            //if (isConvex)
                            //{
                            //    meshDesc.flags = physx::PxConvexFlag::eCOMPUTE_CONVEX;
                            //}

                            // https://docs.nvidia.com/gameworks/content/gameworkslibrary/physx/guide/Manual/Startup.html#startup
                            // mesh should be validated before cooking without the mesh cleaning
                            //MFA_ASSERT(Physics::ValidateTriangleMesh(meshDesc));
                            if (Physics::ValidateTriangleMesh(pxMeshDesc))
                            {
                                MFA_LOG_WARN("Validation of mesh with name %s failed", nameId.c_str());
                            }
                            
                            auto const triangleMesh = Physics::CreateTriangleMesh(pxMeshDesc);

                            if (triangleMesh != nullptr)
                            {
                                meshGroup->triangleMeshes.emplace_back(triangleMesh);
                            } else
                            {
                                MFA_LOG_WARN("Failed to create triangle mesh for one of the descriptions");
                            }
                        }

                        SCOPE_LOCK(meshData.lock)
                        meshData.data = meshGroup;
                        for (auto const & callback : meshData.callbacks)
                        {
                            callback(meshGroup);
                        }
                        meshData.callbacks.clear();
                    });
                },
                loadFromFile
            );
        }
    }

    //-------------------------------------------------------------------------------------------------

}
