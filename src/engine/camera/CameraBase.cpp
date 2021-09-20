#include "CameraBase.hpp"

#include "engine/BedrockAssert.hpp"

//-------------------------------------------------------------------------------------------------

void MFA::CameraBase::SetName(char const * name) {
    MFA_ASSERT(name != nullptr && strlen(name) > 0);
    mName = name;
}

//-------------------------------------------------------------------------------------------------

std::string const & MFA::CameraBase::GetName() const noexcept {
    return mName;
}

//-------------------------------------------------------------------------------------------------