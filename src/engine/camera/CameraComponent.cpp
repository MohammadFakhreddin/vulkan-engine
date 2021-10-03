#include "CameraComponent.hpp"

#include "engine/BedrockAssert.hpp"

//-------------------------------------------------------------------------------------------------

void MFA::CameraComponent::SetName(char const * name) {
    MFA_ASSERT(name != nullptr && strlen(name) > 0);
    mName = name;
}

//-------------------------------------------------------------------------------------------------

std::string const & MFA::CameraComponent::GetName() const noexcept {
    return mName;
}

//-------------------------------------------------------------------------------------------------

MFA::CameraComponent::CameraComponent() : Component() {}

//-------------------------------------------------------------------------------------------------
