#include "BasePipeline.hpp"

#include "EssenceBase.hpp"
#include "VariantBase.hpp"
#include "engine/BedrockAssert.hpp"
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

    void BasePipeline::PreRender(RT::CommandRecordState & recordState, float deltaTime)
    {}

    //-------------------------------------------------------------------------------------------------

    void BasePipeline::Render(RT::CommandRecordState & recordState, float deltaTime)
    {}

    //-------------------------------------------------------------------------------------------------

    bool BasePipeline::EssenceExists(std::string const & nameOrAddress) const
    {
        return mEssenceAndVariantsMap.contains(nameOrAddress);
    }

    //-------------------------------------------------------------------------------------------------

    bool BasePipeline::CreateEssence(
        std::shared_ptr<RT::GpuModel> const & gpuModel,
        std::shared_ptr<AS::MeshBase> const & cpuMesh
    )
    {
        return addEssence(internalCreateEssence(
            gpuModel,
            cpuMesh
        ));
    }

    //-------------------------------------------------------------------------------------------------

    void BasePipeline::DestroyEssence(std::string const & nameOrAddress)
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
            RemoveVariant(*variant);
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

    void BasePipeline::DestroyEssence(RT::GpuModel const & gpuModel)
    {
        DestroyEssence(gpuModel.nameOrAddress);
    }

    //-------------------------------------------------------------------------------------------------

    std::weak_ptr<VariantBase> BasePipeline::CreateVariant(RT::GpuModel const & gpuModel)
    {
        return CreateVariant(gpuModel.nameOrAddress);
    }

    //-------------------------------------------------------------------------------------------------

    std::weak_ptr<VariantBase> BasePipeline::CreateVariant(std::string const & nameOrAddress)
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
        return newVariant;
    }

    //-------------------------------------------------------------------------------------------------

    void BasePipeline::RemoveVariant(VariantBase & variant)
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
            DestroyEssence(nameOrAddress);
        }
    }

    //-------------------------------------------------------------------------------------------------

    void BasePipeline::Init()
    {
        MFA_ASSERT(mIsInitialized == false);
        mIsInitialized = true;
        mDescriptorPool = RF::CreateDescriptorPool(mMaxSets);
    }

    //-------------------------------------------------------------------------------------------------

    void BasePipeline::Shutdown()
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

    bool BasePipeline::addEssence(std::shared_ptr<EssenceBase> const & essence)
    {
        MFA_ASSERT(mIsInitialized == true);
        auto const & address = essence->getGpuModel()->nameOrAddress;

        bool success = false;
        if(MFA_VERIFY(mEssenceAndVariantsMap.contains(address) == false))
        {
            mEssenceAndVariantsMap[address] = EssenceAndVariants(essence);
            mAllEssencesList.emplace_back(essence.get());
            success = true;
        }
        return success;
    }

    //-------------------------------------------------------------------------------------------------

}
