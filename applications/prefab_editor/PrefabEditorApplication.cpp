#include "PrefabEditorApplication.hpp"

#include "engine/scene_manager/SceneManager.hpp"
#include "scenes/PrefabEditorScene.hpp"

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

    mPrefabEditorScene = std::make_shared<PrefabEditorScene>();
    SceneManager::RegisterScene(mPrefabEditorScene, "PrefabEditorScene");
    SceneManager::SetActiveScene("PrefabEditorScene");
}

//-------------------------------------------------------------------------------------------------
