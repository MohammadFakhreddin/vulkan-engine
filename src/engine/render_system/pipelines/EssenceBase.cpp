#include "EssenceBase.hpp"

#include "engine/BedrockAssert.hpp"
#include "engine/render_system/RenderFrontend.hpp"

//-------------------------------------------------------------------------------------------------

MFA::EssenceBase::EssenceBase(std::string nameId)
    : mNameId(std::move(nameId))
{}

//-------------------------------------------------------------------------------------------------

MFA::EssenceBase::~EssenceBase() = default;

//-------------------------------------------------------------------------------------------------

std::string const & MFA::EssenceBase::getNameId() const
{
    return mNameId;
}

//-------------------------------------------------------------------------------------------------
