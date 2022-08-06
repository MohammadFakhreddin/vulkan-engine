#include "ColorComponent.hpp"

#include "engine/BedrockMatrix.hpp"
#include "engine/entity_system/Entity.hpp"
#include "engine/ui_system/UI_System.hpp"
#include "tools/JsonUtils.hpp"

#include "libs/nlohmann/json.hpp"

//-------------------------------------------------------------------------------------------------

void MFA::ColorComponent::SetColor(float color[3])
{
    Copy<3>(mColor, color);
}

//-------------------------------------------------------------------------------------------------

void MFA::ColorComponent::SetColor(glm::vec3 const & color)
{
    Copy(mColor, color);
}

//-------------------------------------------------------------------------------------------------

void MFA::ColorComponent::GetColor(float outColor[3]) const
{
    Copy<3>(outColor, mColor);
}

//-------------------------------------------------------------------------------------------------

glm::vec3 const & MFA::ColorComponent::GetColor() const
{
    return mColor;
}

//-------------------------------------------------------------------------------------------------

void MFA::ColorComponent::OnUI()
{
    if (UI::TreeNode("Color"))
    {
        Component::OnUI();
        UI::InputFloat<3>("Color", mColor);
        UI::TreePop();
    }
}

//-------------------------------------------------------------------------------------------------

void MFA::ColorComponent::Clone(Entity * entity) const
{
    MFA_ASSERT(entity != nullptr);
    entity->AddComponent<ColorComponent>(mColor);
}

//-------------------------------------------------------------------------------------------------

void MFA::ColorComponent::Serialize(nlohmann::json & jsonObject) const
{
    JsonUtils::SerializeVec3(jsonObject, "color", mColor);
}

//-------------------------------------------------------------------------------------------------

void MFA::ColorComponent::Deserialize(nlohmann::json const & jsonObject)
{
    JsonUtils::DeserializeVec3(jsonObject, "color", mColor);
}

//-------------------------------------------------------------------------------------------------
