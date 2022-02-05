#include "PrefabEditorApplication.hpp"

#include "engine/render_system/pipelines/particle/ParticlePipeline.hpp"
#include "engine/scene_manager/SceneManager.hpp"
#include "scenes/PrefabEditorScene.hpp"
#include "engine/render_system/pipelines/debug_renderer/DebugRendererPipeline.hpp"

using namespace MFA;

//-------------------------------------------------------------------------------------------------

PrefabEditorApplication::PrefabEditorApplication()
    : Application()
    , mPrefabEditorScene(nullptr)
{}

//-------------------------------------------------------------------------------------------------

PrefabEditorApplication::~PrefabEditorApplication() = default;

//-------------------------------------------------------------------------------------------------

void PrefabEditorApplication::internalInit()
{
    Application::internalInit();

    SceneManager::RegisterScene("PrefabEditorScene", [this](){
        return std::make_shared<PrefabEditorScene>();
    });

    SceneManager::SetActiveScene("PrefabEditorScene");

    SceneManager::RegisterPipeline<DebugRendererPipeline>();
    SceneManager::RegisterPipeline<PBRWithShadowPipelineV2>();
    SceneManager::RegisterPipeline<ParticlePipeline>();
}

//-------------------------------------------------------------------------------------------------
