#include "ColorComponent.hpp"

#include "engine/BedrockMatrix.hpp"
#include "engine/entity_system/Entity.hpp"
#include "engine/ui_system/UISystem.hpp"
#include "tools/JsonUtils.hpp"

#include "libs/nlohmann/json.hpp"

//-------------------------------------------------------------------------------------------------

void MFA::ColorComponent::SetColor(float color[3])
{
    Matrix::CopyCellsToGlm(color, mColor);
}

//-------------------------------------------------------------------------------------------------

void MFA::ColorComponent::SetColor(glm::vec3 const & color)
{
    mColor = color;
}

//-------------------------------------------------------------------------------------------------

void MFA::ColorComponent::GetColor(float outColor[3]) const
{
    Matrix::CopyGlmToCells(mColor, outColor);
}

//-------------------------------------------------------------------------------------------------

glm::vec3 const & MFA::ColorComponent::GetColor() const
{
    return mColor;
}

//-------------------------------------------------------------------------------------------------

void MFA::ColorComponent::onUI()
{
    if (UI::TreeNode("Color"))
    {
        Component::onUI();
        UI::InputFloat3("Color", mColor.data.data);
        UI::TreePop();
    }
}

//-------------------------------------------------------------------------------------------------

void MFA::ColorComponent::clone(Entity * entity) const
{
    MFA_ASSERT(entity != nullptr);
    entity->AddComponent<ColorComponent>(mColor);
}

//-------------------------------------------------------------------------------------------------

void MFA::ColorComponent::serialize(nlohmann::json & jsonObject) const
{
    JsonUtils::SerializeVec3(jsonObject, "color", mColor);
}

//-------------------------------------------------------------------------------------------------

void MFA::ColorComponent::deserialize(nlohmann::json const & jsonObject)
{
    JsonUtils::DeserializeVec3(jsonObject, "color", mColor);
}

//-------------------------------------------------------------------------------------------------
