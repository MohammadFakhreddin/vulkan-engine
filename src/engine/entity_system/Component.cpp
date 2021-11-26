#include "Component.hpp"

#include "Entity.hpp"
#include "engine/ui_system/UISystem.hpp"

//-------------------------------------------------------------------------------------------------

MFA::Component::Component() = default;

//-------------------------------------------------------------------------------------------------

MFA::Component::~Component() = default;

//-------------------------------------------------------------------------------------------------

std::vector<MFA::Component::ClassType> const & MFA::Component::GetClassType()
{
    MFA_ASSERT(false);
    return {};
}

//-------------------------------------------------------------------------------------------------

void MFA::Component::Init() {}

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
    if ((RequiredEvents() & EventTypes::UpdateEvent) > 0)
    {
        if (mIsActive)
        {
            mUpdateEventId = GetEntity()->mUpdateSignal.Register([this](
                float const deltaTimeInSec,
                RT::CommandRecordState const & recordState
            )->void
                {
                    Update(deltaTimeInSec, recordState);
                }
            );
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

void MFA::Component::Update(float deltaTimeInSec, RT::CommandRecordState const & recordState) {}

//-------------------------------------------------------------------------------------------------

void MFA::Component::OnUI()
{
    bool isActive = mIsActive;
    UI::Checkbox("IsActive", &isActive);
    SetActive(isActive);
}

//-------------------------------------------------------------------------------------------------
