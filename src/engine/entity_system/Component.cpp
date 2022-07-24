#include "Component.hpp"

#include "Entity.hpp"
#include "engine/ui_system/UI_System.hpp"

namespace MFA
{

    //-------------------------------------------------------------------------------------------------

    Component::Component() = default;

    //-------------------------------------------------------------------------------------------------

    Component::~Component() = default;

    //-------------------------------------------------------------------------------------------------

    void Component::Init() {}

    //-------------------------------------------------------------------------------------------------

    void Component::LateInit() {}

    //-------------------------------------------------------------------------------------------------

    void Component::Shutdown() {}

    //-------------------------------------------------------------------------------------------------

    void Component::OnActivationStatusChanged(bool isActive) {}

    //-------------------------------------------------------------------------------------------------

    void Component::SetActive(bool const isActive)
    {
        if (mIsActive == isActive)
        {
            return;
        }
        mIsActive = isActive;

        // Update event
        if ((requiredEvents() & EventTypes::UpdateEvent) > 0)
        {
            if (mIsActive)
            {
                mUpdateEventId = GetEntity()->mUpdateSignal.Register([this](float const deltaTimeInSec)->void
                {
                    Update(deltaTimeInSec);
                });
            }
            else
            {
                GetEntity()->mUpdateSignal.UnRegister(mUpdateEventId);
            }
        }

        if ((requiredEvents() & EventTypes::ActivationChangeEvent) > 0)
        {
            OnActivationStatusChanged(isActive);
        }
    }

    //-------------------------------------------------------------------------------------------------

    bool Component::IsActive() const
    {
        return mIsActive && mEntity->IsActive();
    }

    //-------------------------------------------------------------------------------------------------

    void Component::Update(float deltaTimeInSec) {}

    //-------------------------------------------------------------------------------------------------

    void Component::OnUI()
    {
        if (UI::TreeNode("Component"))
        {
            bool isActive = mIsActive;
            UI::Checkbox("IsActive", &isActive);
            SetActive(isActive);

            EditorSignal.Emit(this, GetEntity());

            UI::TreePop();
        }
    }

    //-------------------------------------------------------------------------------------------------

    using RecipeMap = std::unordered_map<std::string, std::function<std::shared_ptr<Component>()>>;
    static std::unique_ptr<RecipeMap> ComponentRecipeMap = nullptr;

    //-------------------------------------------------------------------------------------------------

    void Component::RegisterComponent(
        std::string const & name,
        std::function<std::shared_ptr<Component>()> const & recipe
    )
    {
        if (ComponentRecipeMap == nullptr)
        {
            ComponentRecipeMap = std::make_unique<RecipeMap>();
        }
        MFA_ASSERT(ComponentRecipeMap->contains(name) == false);
        MFA_ASSERT(recipe != nullptr);
        ComponentRecipeMap->emplace(name, recipe);    
    }

    //-------------------------------------------------------------------------------------------------

    std::shared_ptr<Component> Component::CreateComponent(std::string const & name)
    {
        auto const findRecipeResult = ComponentRecipeMap->find(name);
        if (findRecipeResult == ComponentRecipeMap->end())
        {
            return nullptr;
        }
        MFA_ASSERT(findRecipeResult->second != nullptr);
        return findRecipeResult->second();
    }

    //-------------------------------------------------------------------------------------------------

}
