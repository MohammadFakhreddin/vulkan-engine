#include "BasePipeline.hpp"

#include "engine/BedrockAssert.hpp"
#include "engine/render_system/RenderTypes.hpp"
#include "engine/job_system/JobSystem.hpp"
#include "engine/render_system/RenderFrontend.hpp"

namespace MFA
{

    //-------------------------------------------------------------------------------------------------

    BasePipeline::EssenceAndItsVariants::EssenceAndItsVariants(
        char const * name,
        RT::GpuModel const & model_
    ) : essence(name, model_)
    {}

    //-------------------------------------------------------------------------------------------------

    BasePipeline::BasePipeline(uint32_t maxSets)
    {
        mDescriptorPool = RF::CreateDescriptorPool(maxSets);
    }

    //-------------------------------------------------------------------------------------------------

    BasePipeline::~BasePipeline()
    {
        RF::DestroyDescriptorPool(mDescriptorPool);
    }

    //-------------------------------------------------------------------------------------------------

    void BasePipeline::PreRender(RT::CommandRecordState & drawPass, float deltaTime)
    {
        // Multi-thread update of variant animation
        uint32_t nextThreadNumber = 0;
        auto const availableThreadCount = JS::GetNumberOfAvailableThreads();
        for (const auto & essenceAndVariant : mEssenceAndVariantsMap)
        {
            auto & variants = essenceAndVariant.second->variants;
            for (auto & variant : variants)
            {
                JS::AssignTask(nextThreadNumber, [&variant, deltaTime, &drawPass]()->void
                    {
                        // TODO We might need to separate update tasks for buffer update and normal computation
                        variant->Update(deltaTime, drawPass);
                    }
                );
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

    void BasePipeline::Render(RT::CommandRecordState & drawPass, float deltaTime)
    {}

    //-------------------------------------------------------------------------------------------------

    void BasePipeline::PostRender(RT::CommandRecordState & drawPass, float deltaTime)
    {}

    //-------------------------------------------------------------------------------------------------

    void BasePipeline::CreateDrawableEssence(char const * essenceName, RT::GpuModel const & gpuModel)
    {
        MFA_ASSERT(essenceName != nullptr);
        MFA_ASSERT(strlen(essenceName) > 0);
        MFA_ASSERT(mEssenceAndVariantsMap.find(essenceName) == mEssenceAndVariantsMap.end());

        auto essenceAndVariants = std::make_unique<EssenceAndItsVariants>(essenceName, gpuModel);

        internalCreateDrawableEssence(essenceAndVariants->essence);

        mEssenceAndVariantsMap[essenceName] = std::move(essenceAndVariants);
    }

    //-------------------------------------------------------------------------------------------------

    DrawableVariant * BasePipeline::CreateDrawableVariant(char const * essenceName)
    {
        MFA_ASSERT(essenceName != nullptr);
        MFA_ASSERT(strlen(essenceName) > 0);

        auto const findResult = mEssenceAndVariantsMap.find(essenceName);
        MFA_ASSERT(findResult != mEssenceAndVariantsMap.end());

        auto & variants = findResult->second->variants;

        variants.emplace_back(std::make_unique<DrawableVariant>(findResult->second->essence));

        return variants.back().get();
    }

    //-------------------------------------------------------------------------------------------------

    void BasePipeline::RemoveDrawableVariant(DrawableVariant * variant)
    {
        MFA_ASSERT(variant != nullptr);

        auto const findResult = mEssenceAndVariantsMap.find(variant->GetEssence()->GetName());
        MFA_ASSERT(findResult != mEssenceAndVariantsMap.end());

        auto & variants = findResult->second->variants;
        for (int i = static_cast<int>(variants.size()) - 1; i >= 0; --i)
        {
            if (variants[i].get() == variant)
            {
                variant->Shutdown();

                variants[i].swap(variants.back());
                variants.pop_back();
                return;
            }
        }
        MFA_ASSERT(false);
    }

    //-------------------------------------------------------------------------------------------------

    void BasePipeline::Init()
    {}

    //-------------------------------------------------------------------------------------------------

    void BasePipeline::Shutdown()
    {
        for (auto const & item : mEssenceAndVariantsMap)
        {
            for (auto const & variant : item.second->variants)
            {
                variant->Shutdown();
            }
        }
        mEssenceAndVariantsMap.clear();
    }

    //-------------------------------------------------------------------------------------------------

}
