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
                for (uint32_t i = threadNumber; i < static_cast<uint32_t>(mVariantsRef.size()); i += availableThreadCount)
                {
                    MFA_ASSERT(mVariantsRef[i] != nullptr);
                    mVariantsRef[i]->Update(deltaTime, drawPass);
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

    void BasePipeline::CreateDrawableEssence(
        char const * essenceName,
        RT::GpuModel const & gpuModel
    )
    {
        MFA_ASSERT(essenceName != nullptr);
        MFA_ASSERT(strlen(essenceName) > 0);
        MFA_ASSERT(mEssenceAndVariantsMap.find(essenceName) == mEssenceAndVariantsMap.end());

        auto essenceAndVariants = std::make_unique<EssenceAndItsVariants>(essenceName, gpuModel);

        internalCreateDrawableEssence(essenceAndVariants->essence);

        mEssenceAndVariantsMap[essenceName] = std::move(essenceAndVariants);
    }

    //-------------------------------------------------------------------------------------------------

    void BasePipeline::DestroyDrawableEssence(char const * essenceName)
    {
        MFA_ASSERT(essenceName != nullptr);
        MFA_ASSERT(strlen(essenceName) > 0);
        auto const findResult = mEssenceAndVariantsMap.find(essenceName);
        if (MFA_VERIFY(findResult != mEssenceAndVariantsMap.end()) == false)
        {
            return;
        }
        for (auto & variant : findResult->second->variants)
        {
            RemoveDrawableVariant(variant.get());
        }
        mEssenceAndVariantsMap.erase(findResult);
    }

    //-------------------------------------------------------------------------------------------------

    std::weak_ptr<DrawableVariant> BasePipeline::CreateDrawableVariant(char const * essenceName)
    {
        MFA_ASSERT(essenceName != nullptr);
        MFA_ASSERT(strlen(essenceName) > 0);

        auto const findResult = mEssenceAndVariantsMap.find(essenceName);
        MFA_ASSERT(findResult != mEssenceAndVariantsMap.end());

        auto & variantsList = findResult->second->variants;

        auto const variantSharedPtr = std::make_shared<DrawableVariant>(findResult->second->essence);

        variantsList.emplace_back(variantSharedPtr);

        std::weak_ptr variantWeakPtr = variantSharedPtr;

        mVariantsRef.emplace_back(variantSharedPtr.get());

        return variantWeakPtr;
    }

    //-------------------------------------------------------------------------------------------------

    void BasePipeline::RemoveDrawableVariant(DrawableVariant * variant)
    {
        MFA_ASSERT(variant != nullptr);

        auto const findResult = mEssenceAndVariantsMap.find(variant->GetEssence()->GetName());
        MFA_ASSERT(findResult != mEssenceAndVariantsMap.end());

        bool foundInVariantsRef = false;
        for (int i = static_cast<int>(mVariantsRef.size()) - 1; i >= 0; --i)
        {
            MFA_ASSERT(mVariantsRef[i] != nullptr);
            if (mVariantsRef[i] == variant)
            {
                mVariantsRef.erase(mVariantsRef.begin() + i);
                foundInVariantsRef = true;
                break;
            }
        }
        MFA_ASSERT(foundInVariantsRef == true);

        auto & variants = findResult->second->variants;
        for (int i = static_cast<int>(variants.size()) - 1; i >= 0; --i)
        {
            if (variants[i].get() == variant)
            {
                variant->Shutdown();

                variants[i].reset();
                variants[i] = variants.back();
                variants.pop_back();
                return;
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
            for (auto const & variant : item.second->variants)
            {
                variant->Shutdown();
            }
        }
        mEssenceAndVariantsMap.clear();

        RF::DestroyDescriptorPool(mDescriptorPool);
    }

    //-------------------------------------------------------------------------------------------------

}
