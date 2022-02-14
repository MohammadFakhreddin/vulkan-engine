#include "BasePipeline.hpp"

#include "EssenceBase.hpp"
#include "VariantBase.hpp"
#include "engine/BedrockAssert.hpp"
#include "engine/BedrockPath.hpp"
#include "engine/render_system/RenderTypes.hpp"
#include "engine/render_system/RenderFrontend.hpp"
#include "engine/asset_system/AssetBaseMesh.hpp"

namespace MFA
{

    //-------------------------------------------------------------------------------------------------

    BasePipeline::EssenceAndVariants::EssenceAndVariants() = default;

    //-------------------------------------------------------------------------------------------------

    BasePipeline::EssenceAndVariants::EssenceAndVariants(std::shared_ptr<EssenceBase> essence)
        : essence(std::move(essence))
    {}

    //-------------------------------------------------------------------------------------------------

    BasePipeline::BasePipeline(uint32_t const maxSets)
        : mMaxSets(maxSets)
    {}

    //-------------------------------------------------------------------------------------------------

    BasePipeline::~BasePipeline() = default;

    //-------------------------------------------------------------------------------------------------

    void BasePipeline::preRender(RT::CommandRecordState & recordState, float deltaTime)
    {}

    //-------------------------------------------------------------------------------------------------

    void BasePipeline::render(RT::CommandRecordState & recordState, float deltaTime)
    {}

    //-------------------------------------------------------------------------------------------------

    bool BasePipeline::essenceExists(std::string const & nameOrAddress) const
    {
        std::string relativePath;
        Path::RelativeToAssetFolder(nameOrAddress, relativePath);
        return mEssenceAndVariantsMap.contains(relativePath);
    }

    //-------------------------------------------------------------------------------------------------

    void BasePipeline::destroyEssence(std::string const & nameOrAddress)
    {
        MFA_ASSERT(mIsInitialized == true);
        auto const findResult = mEssenceAndVariantsMap.find(nameOrAddress);
        if (MFA_VERIFY(findResult != mEssenceAndVariantsMap.end()) == false)
        {
            return;
        }

        // Removing essence variants
        for (auto & variant : findResult->second.variants)
        {
            removeVariant(*variant);
        }

        // Removing essence from essenceList
        bool foundEssenceInList = false;
        for (uint32_t i = 0; i < static_cast<uint32_t>(mAllEssencesList.size()); ++i)
        {
            if (mAllEssencesList[i] == findResult->second.essence.get())
            {
                mAllEssencesList.erase(mAllEssencesList.begin() + i);
                foundEssenceInList = true;
                break;
            }
        }
        MFA_ASSERT(foundEssenceInList == true);

        mEssenceAndVariantsMap.erase(findResult);
    }

    //-------------------------------------------------------------------------------------------------

    void BasePipeline::destroyEssence(RT::GpuModel const & gpuModel)
    {
        destroyEssence(gpuModel.nameOrAddress);
    }

    //-------------------------------------------------------------------------------------------------

    std::weak_ptr<VariantBase> BasePipeline::createVariant(RT::GpuModel const & gpuModel)
    {
        return createVariant(gpuModel.nameOrAddress);
    }

    //-------------------------------------------------------------------------------------------------

    std::weak_ptr<VariantBase> BasePipeline::createVariant(std::string const & nameOrAddress)
    {
        MFA_ASSERT(mIsInitialized == true);
        auto const findResult = mEssenceAndVariantsMap.find(nameOrAddress);
        if(findResult == mEssenceAndVariantsMap.end())
        {
            // Note: Is it correct to acquire the missing essence from resource manager ?
            MFA_LOG_ERROR("Cannot create variant. Essence with name %s does not exists", nameOrAddress.c_str());
            return {};
        }

        auto & variantsList = findResult->second.variants;
        variantsList.emplace_back(internalCreateVariant(findResult->second.essence.get()));

        auto newVariant = variantsList.back();
        MFA_ASSERT(newVariant != nullptr);

        mAllVariantsList.emplace_back(newVariant.get());
        if (mAllVariantsList.size() == 1)
        {
            SceneManager::UpdatePipeline(this);
        }
        return newVariant;
    }

    //-------------------------------------------------------------------------------------------------

    void BasePipeline::removeVariant(VariantBase & variant)
    {
        MFA_ASSERT(mIsInitialized == true);
        // Removing from all variants list
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

        if (mAllVariantsList.empty())
        {
            SceneManager::UpdatePipeline(this);
        }
        
        // Removing from essence and variants' list
        auto const findResult = mEssenceAndVariantsMap.find(variant.GetEssence()->getNameOrAddress());
        MFA_ASSERT(findResult != mEssenceAndVariantsMap.end());
        auto & variants = findResult->second.variants;
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
        
        MFA_ASSERT(false);
    }

    //-------------------------------------------------------------------------------------------------

    void BasePipeline::freeUnusedEssences()
    {
        RF::DeviceWaitIdle();
        std::vector<std::string> unusedEssenceNames {};
        for (auto & essenceAndVariants : mEssenceAndVariantsMap)
        {
            if (essenceAndVariants.second.variants.empty())
            {
                unusedEssenceNames.emplace_back(essenceAndVariants.second.essence->getGpuModel()->nameOrAddress);
            }
        }
        for (auto const & nameOrAddress : unusedEssenceNames)
        {
            destroyEssence(nameOrAddress);
        }
    }

    //-------------------------------------------------------------------------------------------------

    void BasePipeline::init()
    {
        MFA_ASSERT(mIsInitialized == false);
        mIsInitialized = true;
        mDescriptorPool = RF::CreateDescriptorPool(mMaxSets);
    }

    //-------------------------------------------------------------------------------------------------

    void BasePipeline::shutdown()
    {
        MFA_ASSERT(mIsInitialized == true);
        mIsInitialized = false;
        for (auto const & item : mEssenceAndVariantsMap)
        {
            for (auto & variant : item.second.variants)
            {
                variant->Shutdown();
            }
        }
        mEssenceAndVariantsMap.clear();

        RF::DestroyDescriptorPool(mDescriptorPool);
    }

    //-------------------------------------------------------------------------------------------------

    void BasePipeline::changeActivationStatus(bool const enabled)
    {
        if (mIsActive == enabled)
        {
            return;
        }
        mIsActive = enabled;
        SceneManager::UpdatePipeline(this);
    }

    //-------------------------------------------------------------------------------------------------

    bool BasePipeline::isActive() const
    {
        return mIsActive;
    }

    //-------------------------------------------------------------------------------------------------

    bool BasePipeline::addEssence(std::shared_ptr<EssenceBase> const & essence)
    {
        MFA_ASSERT(mIsInitialized == true);
        auto const & address = essence->getGpuModel()->nameOrAddress;

        bool success = false;
        if(MFA_VERIFY(mEssenceAndVariantsMap.contains(address) == false))
        {
            mEssenceAndVariantsMap[address] = EssenceAndVariants(essence);
            mAllEssencesList.emplace_back(essence.get());
            internalAddEssence(essence.get());
            success = true;
        }
        return success;
    }

    //-------------------------------------------------------------------------------------------------

}
