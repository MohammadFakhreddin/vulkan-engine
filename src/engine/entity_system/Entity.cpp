#include "Entity.hpp"

#include "EntitySystem.hpp"
#include "components/AxisAlignedBoundingBoxComponent.hpp"
#include "components/BoundingVolumeRendererComponent.hpp"
#include "components/ColorComponent.hpp"
#include "components/MeshRendererComponent.hpp"
#include "components/PointLightComponent.hpp"
#include "components/SphereBoundingVolumeComponent.hpp"
#include "components/TransformComponent.hpp"
#include "engine/ui_system/UISystem.hpp"

#include "libs/nlohmann/json.hpp"

namespace MFA
{

    //-------------------------------------------------------------------------------------------------

    Entity::Entity(
        char const * name,
        Entity * parent
    )
        : mName(name)
        , mParent(parent)
    {}

    //-------------------------------------------------------------------------------------------------

    Entity::~Entity() = default;

    //-------------------------------------------------------------------------------------------------

    void Entity::Init()
    {
        if (mIsInitialized)
        {
            return;
        }
        mIsInitialized = true;

        if (mParent != nullptr)
        {
            mParent->notifyANewChildAdded(this);
        }
        mInitSignal.Emit();
    }

    //-------------------------------------------------------------------------------------------------
    // TODO Support for priority between components
    void Entity::Update(float const deltaTimeInSec, RT::CommandRecordState const & recordState) const
    {
        if (mIsActive == false)
        {
            return;
        }
        mUpdateSignal.Emit(deltaTimeInSec, recordState);
    }

    //-------------------------------------------------------------------------------------------------

    void Entity::Shutdown(bool const shouldNotifyParent)
    {
        if (mIsInitialized == false)
        {
            return;
        }
        mIsInitialized = false;

        mShutdownSignal.Emit();

        if (shouldNotifyParent && mParent != nullptr)
        {
            mParent->notifyAChildRemoved(this);
        }
    }

    //-------------------------------------------------------------------------------------------------

    std::string const & Entity::GetName() const noexcept
    {
        return mName;
    }

    //-------------------------------------------------------------------------------------------------

    void Entity::SetName(char const * name)
    {
        MFA_ASSERT(name != nullptr && strlen(name) > 0);
        mName = name;
    }

    //-------------------------------------------------------------------------------------------------

    bool Entity::IsActive() const noexcept
    {
        return mIsActive && mIsParentActive;
    }

    //-------------------------------------------------------------------------------------------------

    void Entity::SetActive(bool const isActive)
    {
        if (isActive == mIsActive)
        {
            return;
        }
        mIsActive = isActive;
        mActivationStatusChangeSignal.Emit(mIsActive);
        onActivationStatusChanged();
    }

    //-------------------------------------------------------------------------------------------------

    std::vector<Entity *> const & Entity::GetChildEntities() const
    {
        return mChildEntities;
    }

    //-------------------------------------------------------------------------------------------------

    void Entity::OnUI()
    {
        if (UI::TreeNode(mName.c_str()))
        {
            bool isActive = mIsActive;
            UI::Checkbox("IsActive", &isActive);
            SetActive(isActive);
            if (UI::TreeNode("Components"))
            {
                for (auto const & component : mComponents)
                {
                    if (component != nullptr)
                    {
                        component->OnUI();
                    }
                }
                UI::TreePop();
            }
            if (UI::TreeNode("Children"))
            {
                for (auto * childEntity : mChildEntities)
                {
                    childEntity->OnUI();
                }
                UI::TreePop();
            }
            EditorSignal.Emit(this);
            UI::TreePop();
        }
    }

    //-------------------------------------------------------------------------------------------------

    bool Entity::HasParent() const
    {
        return mParent != nullptr;
    }

    //-------------------------------------------------------------------------------------------------

    void Entity::OnParentActivationStatusChanged(bool const isActive)
    {
        if (mIsParentActive == isActive)
        {
            return;
        }
        mIsParentActive = isActive;
        onActivationStatusChanged();
    }

    //-------------------------------------------------------------------------------------------------

    Entity * Entity::Clone(char const * name, Entity * parent) const
    {
        MFA_ASSERT(name != nullptr);
        MFA_ASSERT(strlen(name) > 0);
        auto * entity = EntitySystem::CreateEntity(name, parent);
        for (auto & component : mComponents)
        {
            if (component != nullptr)
            {
                component->Clone(entity);
            }
        }
        for (auto & child : mChildEntities)
        {
            child->Clone(child->GetName().c_str(), entity);
        }

        EntitySystem::InitEntity(entity);

        return entity;
    }

    //-------------------------------------------------------------------------------------------------

    void Entity::Serialize(nlohmann::json & jsonObject)
    {
        jsonObject["name"] = mName.c_str();
        for (auto const & component : mComponents)
        {
            if (component != nullptr)
            {
                nlohmann::json componentJson{};
                componentJson["name"] = component->GetName();
                componentJson["familyType"] = component->GetFamilyType();

                nlohmann::json componentData{};
                component->Serialize(componentData);
                componentJson["data"] = componentData;

                jsonObject["components"].emplace_back(componentJson);

            }
        }
        for (auto const & child : mChildEntities)
        {
            nlohmann::json childEntityJson{};
            child->Serialize(childEntityJson);
            jsonObject["children"].emplace_back(childEntityJson);
        }
    }

    //-------------------------------------------------------------------------------------------------

    void Entity::Deserialize(nlohmann::json const & jsonObject)
    {
        mName = jsonObject["name"];
        auto const & rawComponents = jsonObject["components"];
        for (auto const & rawComponent : rawComponents)
        {
            std::string const name = rawComponent["name"];
            int const familyType = rawComponent["familyType"];

            auto rawComponentData = rawComponent["data"];

            std::shared_ptr<Component> component = nullptr;

            if (strcmp(name.c_str(), TransformComponent::Name) == 0)
            {
                component = AddComponent<TransformComponent>().lock();
            }
            else if (strcmp(name.c_str(), MeshRendererComponent::Name) == 0)
            {
                component = AddComponent<MeshRendererComponent>().lock();
            }
            else if (strcmp(name.c_str(), BoundingVolumeRendererComponent::Name) == 0)
            {
                component = AddComponent<BoundingVolumeRendererComponent>().lock();
            }
            else if (strcmp(name.c_str(), SphereBoundingVolumeComponent::Name) == 0)
            {
                component = AddComponent<SphereBoundingVolumeComponent>().lock();
            }
            else if (strcmp(name.c_str(), AxisAlignedBoundingBoxComponent::Name) == 0)
            {
                component = AddComponent<AxisAlignedBoundingBoxComponent>().lock();
            }
            else if (strcmp(name.c_str(), ColorComponent::Name) == 0)
            {
                component = AddComponent<ColorComponent>().lock();
            }
            else if (strcmp(name.c_str(), PointLightComponent::Name) == 0)
            {
                component = AddComponent<PointLightComponent>().lock();
            }

            if (MFA_VERIFY(component != nullptr))
            {
                component->Deserialize(rawComponentData);
            }
        }

        EntitySystem::InitEntity(this);
    }

    //-------------------------------------------------------------------------------------------------

    std::vector<Component *> Entity::GetComponents()
    {
        std::vector<Component *> result {};
        for (auto & component : mComponents)
        {
            if (component != nullptr)
            {
                result.emplace_back(component.get());
            }
        }
        return result;
    }

    //-------------------------------------------------------------------------------------------------

    void Entity::notifyANewChildAdded(Entity * entity)
    {
        MFA_ASSERT(entity != nullptr);
        MFA_ASSERT(findChild(entity) < 0);
        entity->mIsParentActive = IsActive();
        entity->mParentActivationStatusChangeListenerId = mActivationStatusChangeSignal.Register([entity](bool const isActive)->void
        {
            entity->OnParentActivationStatusChanged(isActive);
        });
        mChildEntities.emplace_back(entity);
    }

    //-------------------------------------------------------------------------------------------------

    void Entity::notifyAChildRemoved(Entity * entity)
    {
        mActivationStatusChangeSignal.UnRegister(entity->mParentActivationStatusChangeListenerId);
        auto const childIndex = findChild(entity);
        if (MFA_VERIFY(childIndex >= 0))
        {
            mChildEntities[childIndex] = mChildEntities.back();
            mChildEntities.pop_back();
        }
    }

    //-------------------------------------------------------------------------------------------------

    int Entity::findChild(Entity * entity)
    {
        MFA_ASSERT(entity != nullptr);
        for (int i = 0; i < static_cast<int>(mChildEntities.size()); ++i)
        {
            if (mChildEntities[i] == entity)
            {
                return i;
            }
        }
        return -1;
    }

    //-------------------------------------------------------------------------------------------------

    void Entity::linkComponent(Component * component)
    {
        // Linked entity
        component->mEntity = this;
        // Init event
        if ((component->RequiredEvents() & Component::EventTypes::InitEvent) > 0)
        {
            component->mInitEventId = mInitSignal.Register([component]()->void { component->Init(); });
        }
        // Update event
        if ((component->RequiredEvents() & Component::EventTypes::UpdateEvent) > 0)
        {
            component->mUpdateEventId = mUpdateSignal.Register([component](
                float const deltaTimeInSec,
                RT::CommandRecordState const & recordState)->void
            {
                component->Update(deltaTimeInSec, recordState);
            });
        }
        // Shutdown event
        if ((component->RequiredEvents() & Component::EventTypes::ShutdownEvent) > 0)
        {
            component->mShutdownEventId = mShutdownSignal.Register([component]()->void
            {
                component->Shutdown();
            });
        }
    }

    //-------------------------------------------------------------------------------------------------

    void Entity::onActivationStatusChanged()
    {
        if (mIsInitialized == false)
        {
            return;
        }
        EntitySystem::UpdateEntity(this);
    }

    //-------------------------------------------------------------------------------------------------

}
