#include "ColorComponent.hpp"

#include "engine/BedrockMatrix.hpp"
#include "engine/ui_system/UISystem.hpp"

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

void MFA::ColorComponent::OnUI()
{
    if (UI::TreeNode("Color"))
    {
        Component::OnUI();
        UI::InputFloat3("Color", mColor.data.data);
        UI::TreePop();
    }
}

//-------------------------------------------------------------------------------------------------
