#include "BasePipeline.hpp"

#include "engine/BedrockAssert.hpp"
#include "engine/render_system/RenderTypes.hpp"
#include "engine/job_system/JobSystem.hpp"
#include "engine/render_system/RenderFrontend.hpp"

namespace MFA
{

    //-------------------------------------------------------------------------------------------------

    BasePipeline::EssenceAndVariants::EssenceAndVariants() = default;

    //-------------------------------------------------------------------------------------------------

    BasePipeline::EssenceAndVariants::EssenceAndVariants(std::shared_ptr<RT::GpuModel> const & gpuModel)
        : Essence(std::make_unique<DrawableEssence>(gpuModel))
    {}

    //-------------------------------------------------------------------------------------------------

    BasePipeline::EssenceAndVariants::~EssenceAndVariants() = default;

    //-------------------------------------------------------------------------------------------------

    BasePipeline::BasePipeline(uint32_t const maxSets)
        : mMaxSets(maxSets)
    {}

    //-------------------------------------------------------------------------------------------------

    BasePipeline::~BasePipeline() = default;

    //-------------------------------------------------------------------------------------------------

    void BasePipeline::PreRender(RT::CommandRecordState & drawPass, float deltaTime)
    {
        // Multi-thread update of variant animation
        auto const availableThreadCount = JS::GetNumberOfAvailableThreads();
        for (uint32_t threadNumber = 0; threadNumber < availableThreadCount; ++threadNumber)
        {
            JS::AssignTask(threadNumber, [this, &drawPass, deltaTime, threadNumber, availableThreadCount]()->void
            {
                for (uint32_t i = threadNumber; i < static_cast<uint32_t>(mAllVariantsList.size()); i += availableThreadCount)
                {
                    MFA_ASSERT(mAllVariantsList[i] != nullptr);
                    mAllVariantsList[i]->Update(deltaTime, drawPass);
                }
            });
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

    bool BasePipeline::CreateEssenceIfNotExists(std::shared_ptr<RT::GpuModel> const & gpuModel)
    {
        if(mEssenceAndVariantsMap.find(gpuModel->id) != mEssenceAndVariantsMap.end())
        {
            return false;
        }

        EssenceAndVariants const essenceAndVariants {gpuModel};
        internalCreateDrawableEssence(*essenceAndVariants.Essence);
        mEssenceAndVariantsMap[gpuModel->id] = essenceAndVariants;

        return true;
    }

    //-------------------------------------------------------------------------------------------------

    void BasePipeline::DestroyDrawableEssence(RT::GpuModelId const id)
    {
        auto const findResult = mEssenceAndVariantsMap.find(id);
        if (MFA_VERIFY(findResult != mEssenceAndVariantsMap.end()) == false)
        {
            return;
        }
        for (auto & variant : findResult->second.Variants)
        {
            RemoveDrawableVariant(*variant);
        }
        mEssenceAndVariantsMap.erase(findResult);
    }

    //-------------------------------------------------------------------------------------------------

    void BasePipeline::DestroyDrawableEssence(RT::GpuModel const & gpuModel)
    {
        DestroyDrawableEssence(gpuModel.id);
    }

    //-------------------------------------------------------------------------------------------------

    DrawableVariant * BasePipeline::CreateDrawableVariant(RT::GpuModel const & gpuModel)
    {
        return CreateDrawableVariant(gpuModel.id);
    }

    //-------------------------------------------------------------------------------------------------

    DrawableVariant * BasePipeline::CreateDrawableVariant(RT::GpuModelId const id)
    {
        auto const findResult = mEssenceAndVariantsMap.find(id);
        MFA_ASSERT(findResult != mEssenceAndVariantsMap.end());

        auto & variantsList = findResult->second.Variants;
        variantsList.emplace_back(std::make_unique<DrawableVariant>(*findResult->second.Essence));

        auto * newVariant = variantsList.back().get();
        MFA_ASSERT(newVariant != nullptr);

        mAllVariantsList.emplace_back(newVariant);
        return newVariant;
    }

    //-------------------------------------------------------------------------------------------------

    void BasePipeline::RemoveDrawableVariant(DrawableVariant & variant)
    {
        {// Removing from all variants list
            bool foundInAllVariantsList = false;
            for (int i = static_cast<int>(mAllVariantsList.size()) - 1; i >= 0; --i)
            {
                MFA_ASSERT(mAllVariantsList[i] != nullptr);
                if (mAllVariantsList[i]->GetId() == variant.GetId())
                {
                    std::iter_swap(
                        mAllVariantsList.begin() + i,
                        mAllVariantsList.begin() +
                            static_cast<int>(mAllVariantsList.size()) - 1
                    );
                    mAllVariantsList.pop_back();

                    foundInAllVariantsList = true;
                    break;
                }
            }
            MFA_ASSERT(foundInAllVariantsList == true);
        }

        {// Removing from essence and variants' list
            auto const findResult = mEssenceAndVariantsMap.find(variant.GetEssence()->GetId());
            MFA_ASSERT(findResult != mEssenceAndVariantsMap.end());
            auto & variants = findResult->second.Variants;
            for (int i = static_cast<int>(variants.size()) - 1; i >= 0; --i)
            {
                if (variants[i]->GetId() == variant.GetId())
                {
                    variant.Shutdown();
                    variants[i] = variants.back();
                    variants.pop_back();
                    return;
                }
            }
        }

        MFA_ASSERT(false);
    }

    //-------------------------------------------------------------------------------------------------

    void BasePipeline::Init()
    {
        mDescriptorPool = RF::CreateDescriptorPool(mMaxSets);
    }

    //-------------------------------------------------------------------------------------------------

    void BasePipeline::Shutdown()
    {
        for (auto const & item : mEssenceAndVariantsMap)
        {
            for (auto & variant : item.second.Variants)
            {
                variant->Shutdown();
            }
        }
        mEssenceAndVariantsMap.clear();

        RF::DestroyDescriptorPool(mDescriptorPool);
    }

    //-------------------------------------------------------------------------------------------------

}
