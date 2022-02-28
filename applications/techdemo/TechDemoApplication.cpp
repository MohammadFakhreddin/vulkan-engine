#include "TechDemoApplication.hpp"

#include "engine/entity_system/EntitySystem.hpp"
#include "engine/render_system/pipelines/pbr_with_shadow_v2/PbrWithShadowPipelineV2.hpp"
#include "scenes/demo_3rd_person_scene/Demo3rdPersonScene.hpp"
#include "scenes/gltf_mesh_viewer/GLTFMeshViewerScene.hpp"
#include "engine/ui_system/UI_System.hpp"
#include "engine/render_system/pipelines/debug_renderer/DebugRendererPipeline.hpp"
#include "engine/render_system/pipelines/particle/ParticlePipeline.hpp"
#include "scenes/particle_fire_scene/ParticleFireScene.hpp"
#include "engine/scene_manager/SceneManager.hpp"
#include "engine/BedrockPlatforms.hpp"

using namespace MFA;

//-------------------------------------------------------------------------------------------------

TechDemoApplication::TechDemoApplication() = default;

//-------------------------------------------------------------------------------------------------

TechDemoApplication::~TechDemoApplication() = default;

//-------------------------------------------------------------------------------------------------

void TechDemoApplication::internalInit()
{
    SceneManager::RegisterScene("ThirdPersonDemoScene", []()->std::shared_ptr<Demo3rdPersonScene>{
        return std::make_shared<Demo3rdPersonScene>();
    });

    SceneManager::RegisterScene("GLTFMeshViewerScene", []()->std::shared_ptr<GLTFMeshViewerScene>{
        return std::make_shared<GLTFMeshViewerScene>();
    });

    SceneManager::RegisterScene("ParticleFireScene", []()->std::shared_ptr<ParticleFireScene>{
        return std::make_shared<ParticleFireScene>();
    });
    
    SceneManager::SetActiveScene("ThirdPersonDemoScene");

    UI::Register([]()->void {OnUI();});

    SceneManager::RegisterPipeline<DebugRendererPipeline>();
#ifndef __PLATFORM_MAC__ // Currently we don't have replacement for geometry shader. Going to use compute shader as replacement
    SceneManager::RegisterPipeline<PBRWithShadowPipelineV2>();
#endif
    SceneManager::RegisterPipeline<ParticlePipeline>();
}

//-------------------------------------------------------------------------------------------------

void TechDemoApplication::OnUI() {
    SceneManager::OnUI();
    EntitySystem::OnUI();
}

//-------------------------------------------------------------------------------------------------
