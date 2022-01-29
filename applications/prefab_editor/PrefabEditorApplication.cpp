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

    SceneManager::RegisterScene("PrefabEditorScene", [this](){
        return std::make_shared<PrefabEditorScene>();
    });

    SceneManager::SetActiveScene("PrefabEditorScene");
}

//-------------------------------------------------------------------------------------------------
