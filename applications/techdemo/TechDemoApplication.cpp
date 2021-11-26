#include "TechDemoApplication.hpp"

#include "engine/entity_system/EntitySystem.hpp"
#include "engine/scene_manager/SceneManager.hpp"
#include "scenes/demo_3rd_person_scene/Demo3rdPersonScene.hpp"
#include "scenes/gltf_mesh_viewer/GLTFMeshViewerScene.hpp"
#include "engine/ui_system/UISystem.hpp"

using namespace MFA;

//-------------------------------------------------------------------------------------------------

TechDemoApplication::TechDemoApplication()
    : Application()
    , mThirdPersonDemoScene(nullptr)
    , mGltfMeshViewerScene(nullptr)
{}

//-------------------------------------------------------------------------------------------------

TechDemoApplication::~TechDemoApplication() = default;

//-------------------------------------------------------------------------------------------------

void TechDemoApplication::internalInit()
{
    mThirdPersonDemoScene = std::make_shared<Demo3rdPersonScene>();
    mGltfMeshViewerScene = std::make_shared<GLTFMeshViewerScene>();

    SceneManager::RegisterScene(mThirdPersonDemoScene, "ThirdPersonDemoScene");
    SceneManager::RegisterScene(mGltfMeshViewerScene, "GLTFMeshViewerScene");
    
    SceneManager::SetActiveScene("ThirdPersonDemoScene");

    UI::Register([]()->void {OnUI();});
    
}

//-------------------------------------------------------------------------------------------------

void TechDemoApplication::OnUI() {
    SceneManager::OnUI();
    EntitySystem::OnUI();
}

//-------------------------------------------------------------------------------------------------
