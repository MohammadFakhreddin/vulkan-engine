#include "BasePipeline.hpp"

#include "engine/BedrockAssert.hpp"
#include "engine/render_system/RenderTypes.hpp"
#include "engine/render_system/drawable_essence/DrawableEssence.hpp"
#include "engine/render_system/drawable_variant/DrawableVariant.hpp"

namespace MFA {

//-------------------------------------------------------------------------------------------------

BasePipeline::BasePipeline()
    : mEssenceAndVariantsMap({})
{}

//-------------------------------------------------------------------------------------------------

BasePipeline::~BasePipeline() = default;

//-------------------------------------------------------------------------------------------------

void BasePipeline::Init() {}

//-------------------------------------------------------------------------------------------------

void BasePipeline::Shutdown() {
    for (auto & essence : mEssenceAndVariantsMap) {
        delete essence.second.essence;
        for (auto & variant : essence.second.variants) {
            delete variant;
        }
    }
    mEssenceAndVariantsMap.clear();
}

//-------------------------------------------------------------------------------------------------

void BasePipeline::PreRender(
    RT::DrawPass & drawPass,
    float const deltaTime
) {
    for (auto & essenceAndVariant : mEssenceAndVariantsMap) {
        for (auto & variant : essenceAndVariant.second.variants) {
            variant->Update(deltaTime, drawPass);
        }
    }
}

//-------------------------------------------------------------------------------------------------

void BasePipeline::Render(RT::DrawPass & drawPass, float const deltaTime) {}

//-------------------------------------------------------------------------------------------------

void BasePipeline::CreateDrawableEssence(char const * essenceName, RT::GpuModel const & gpuModel) {
    MFA_ASSERT(essenceName != nullptr);
    MFA_ASSERT(strlen(essenceName) > 0);
    MFA_ASSERT(mEssenceAndVariantsMap.find(essenceName) == mEssenceAndVariantsMap.end());

    mEssenceAndVariantsMap[essenceName] = EssenceAndItsVariants (new DrawableEssence(essenceName, gpuModel));
}

//-------------------------------------------------------------------------------------------------

DrawableVariant * BasePipeline::CreateDrawableVariant(char const * essenceName) {
    MFA_ASSERT(essenceName != nullptr);
    MFA_ASSERT(strlen(essenceName) > 0);
    
    auto findResult = mEssenceAndVariantsMap.find(essenceName);
    MFA_ASSERT(findResult != mEssenceAndVariantsMap.end());

    findResult->second.variants.emplace_back(new DrawableVariant(*findResult->second.essence));
    return findResult->second.variants.back();
}

//-------------------------------------------------------------------------------------------------

void BasePipeline::RemoveDrawableVariant(DrawableVariant * variant) {
    MFA_ASSERT(variant != nullptr);

    auto findResult = mEssenceAndVariantsMap.find(variant->GetEssence()->GetName());
    MFA_ASSERT(findResult != mEssenceAndVariantsMap.end());
    
}

//-------------------------------------------------------------------------------------------------

BasePipeline::EssenceAndItsVariants::EssenceAndItsVariants()
    : essence(nullptr)
    , variants({})
{}

//-------------------------------------------------------------------------------------------------

BasePipeline::EssenceAndItsVariants::EssenceAndItsVariants(DrawableEssence * essence)
    : essence(essence)
    , variants({})
{}

//-------------------------------------------------------------------------------------------------

}
