#include "Component.hpp"

#include "Entity.hpp"
#include "engine/ui_system/UI_System.hpp"

//-------------------------------------------------------------------------------------------------

MFA::Component::Component() = default;

//-------------------------------------------------------------------------------------------------

MFA::Component::~Component() = default;

//-------------------------------------------------------------------------------------------------

void MFA::Component::Init() {}

//-------------------------------------------------------------------------------------------------

void MFA::Component::LateInit() {}

//-------------------------------------------------------------------------------------------------

void MFA::Component::Shutdown() {}

//-------------------------------------------------------------------------------------------------

void MFA::Component::SetActive(bool const isActive)
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
            mUpdateEventId = GetEntity()->mUpdateSignal.Register([this](float const deltaTimeInSec)->void{
                Update(deltaTimeInSec);
            });
        } else
        {
            GetEntity()->mUpdateSignal.UnRegister(mUpdateEventId);
        }
    }
}

//-------------------------------------------------------------------------------------------------

bool MFA::Component::IsActive() const
{
    return mIsActive && mEntity->IsActive();
}

//-------------------------------------------------------------------------------------------------

void MFA::Component::Update(float deltaTimeInSec) {}

//-------------------------------------------------------------------------------------------------

void MFA::Component::onUI()
{
    bool isActive = mIsActive;
    UI::Checkbox("IsActive", &isActive);
    SetActive(isActive);

    EditorSignal.Emit(this, GetEntity());
}

//-------------------------------------------------------------------------------------------------
