#include "BasePipeline.hpp"

#include "engine/BedrockAssert.hpp"
#include "engine/render_system/RenderTypes.hpp"
#include "engine/render_system/drawable_essence/DrawableEssence.hpp"
#include "engine/render_system/drawable_variant/DrawableVariant.hpp"
#include "engine/job_system/JobSystem.hpp"

namespace MFA {

//-------------------------------------------------------------------------------------------------

BasePipeline::BasePipeline() = default;

//-------------------------------------------------------------------------------------------------

BasePipeline::~BasePipeline() = default;

//-------------------------------------------------------------------------------------------------

void BasePipeline::Init() {}

//-------------------------------------------------------------------------------------------------

void BasePipeline::Shutdown() {
    mEssenceAndVariantsMap.clear();
}

//-------------------------------------------------------------------------------------------------

void BasePipeline::PreRender(
    RT::DrawPass & drawPass,
    float const deltaTime
) {
    uint32_t nextThreadNumber = 0;
    auto const availableThreadCount = JS::GetNumberOfAvailableThreads();
    for (const auto & essenceAndVariant : mEssenceAndVariantsMap) {
        for (const auto & variant : essenceAndVariant.second->variants) {
            JS::AssignTask(nextThreadNumber, [&variant, deltaTime, &drawPass]()->void{
                variant->Update(deltaTime, drawPass);
            });
            ++nextThreadNumber;
            if (nextThreadNumber >= availableThreadCount)
            {
                nextThreadNumber = 0;
            }
        }
    }
    JS::WaitForThreadsToFinish();
}

//-------------------------------------------------------------------------------------------------

void BasePipeline::Render(RT::DrawPass & drawPass, float const deltaTime) {}

//-------------------------------------------------------------------------------------------------

void BasePipeline::CreateDrawableEssence(char const * essenceName, RT::GpuModel const & gpuModel) {
    MFA_ASSERT(essenceName != nullptr);
    MFA_ASSERT(strlen(essenceName) > 0);
    MFA_ASSERT(mEssenceAndVariantsMap.find(essenceName) == mEssenceAndVariantsMap.end());

    mEssenceAndVariantsMap[essenceName] = std::make_unique<EssenceAndItsVariants>(EssenceAndItsVariants (std::make_unique<DrawableEssence>(essenceName, gpuModel)));
}

//-------------------------------------------------------------------------------------------------

DrawableVariant * BasePipeline::CreateDrawableVariant(char const * essenceName) {
    MFA_ASSERT(essenceName != nullptr);
    MFA_ASSERT(strlen(essenceName) > 0);
    
    auto findResult = mEssenceAndVariantsMap.find(essenceName);
    MFA_ASSERT(findResult != mEssenceAndVariantsMap.end());

    findResult->second->variants.emplace_back(new DrawableVariant(*findResult->second->essence));
    return findResult->second->variants.back().get();
}

//-------------------------------------------------------------------------------------------------

void BasePipeline::RemoveDrawableVariant(DrawableVariant * variant) {
    MFA_ASSERT(variant != nullptr);

    auto findResult = mEssenceAndVariantsMap.find(variant->GetEssence()->GetName());
    MFA_ASSERT(findResult != mEssenceAndVariantsMap.end());
    
}

//-------------------------------------------------------------------------------------------------

BasePipeline::EssenceAndItsVariants::EssenceAndItsVariants(std::unique_ptr<DrawableEssence> && essence)
    : essence(std::move(essence))
{}

//-------------------------------------------------------------------------------------------------

}
