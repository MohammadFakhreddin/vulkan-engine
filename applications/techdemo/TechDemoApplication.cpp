#include "TechDemoApplication.hpp"

#include "engine/entity_system/EntitySystem.hpp"
#include "engine/scene_manager/SceneManager.hpp"
#include "scenes/demo_3rd_person_scene/Demo3rdPersonScene.hpp"
#include "scenes/gltf_mesh_viewer/GLTFMeshViewerScene.hpp"
#include "engine/ui_system/UISystem.hpp"

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
    
    SceneManager::SetActiveScene("ThirdPersonDemoScene");

    UI::Register([]()->void {OnUI();});
    
}

//-------------------------------------------------------------------------------------------------

void TechDemoApplication::OnUI() {
    SceneManager::OnUI();
    EntitySystem::OnUI();
}

//-------------------------------------------------------------------------------------------------
