#include "BaseMaterial.hpp"

#include "EssenceBase.hpp"
#include "VariantBase.hpp"
#include "engine/BedrockAssert.hpp"
#include "engine/BedrockPath.hpp"
#include "engine/render_system/RenderTypes.hpp"
#include "engine/render_system/RenderFrontend.hpp"
#include "engine/scene_manager/SceneManager.hpp"

namespace MFA
{

    //-------------------------------------------------------------------------------------------------

    BaseMaterial::EssenceAndVariants::EssenceAndVariants() = default;

    //-------------------------------------------------------------------------------------------------

    BaseMaterial::EssenceAndVariants::EssenceAndVariants(std::shared_ptr<EssenceBase> essence)
        : essence(std::move(essence))
    {}

    //-------------------------------------------------------------------------------------------------

    BaseMaterial::BaseMaterial(uint32_t const maxSets)
        : mMaxSets(maxSets)
    {}

    //-------------------------------------------------------------------------------------------------

    BaseMaterial::~BaseMaterial() = default;

    //-------------------------------------------------------------------------------------------------

    void BaseMaterial::preRender(RT::CommandRecordState & recordState, float deltaTime)
    {
    }

    //-------------------------------------------------------------------------------------------------

    void BaseMaterial::Render(RT::CommandRecordState & recordState, float deltaTime)
    {}

    //-------------------------------------------------------------------------------------------------

    void BaseMaterial::Update(float deltaTime)
    {}

    //-------------------------------------------------------------------------------------------------

    void BaseMaterial::compute(RT::CommandRecordState & recordState, float deltaTime)
    {}

    //-------------------------------------------------------------------------------------------------

    bool BaseMaterial::hasEssence(std::string const & nameId) const
    {
        return mEssenceAndVariantsMap.find(Path::RelativeToAssetFolder(nameId)) != mEssenceAndVariantsMap.end();
    }

    //-------------------------------------------------------------------------------------------------

    void BaseMaterial::destroyEssence(std::string const & nameId)
    {
        MFA_ASSERT(mIsInitialized == true);
        auto const findResult = mEssenceAndVariantsMap.find(nameId);
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

    std::weak_ptr<VariantBase> BaseMaterial::createVariant(std::string const & path)
    {
        auto const nameId = Path::RelativeToAssetFolder(path);
        //MFA_ASSERT(mIsInitialized == true);
        auto const findResult = mEssenceAndVariantsMap.find(nameId);
        if(findResult == mEssenceAndVariantsMap.end())
        {
            // Note: Is it correct to acquire the missing essence from resource manager ?
            MFA_LOG_ERROR("Cannot create variant. Essence with name %s does not exists", nameId.c_str());
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

    void BaseMaterial::removeVariant(VariantBase & variant)
    {
        SceneManager::AssignMainThreadTask([this, &variant]()->void {
            RF::DeviceWaitIdle();
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
            auto const findResult = mEssenceAndVariantsMap.find(variant.getEssence()->getNameId());
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
        }, false);
    }

    //-------------------------------------------------------------------------------------------------

    void BaseMaterial::freeUnusedEssences()
    {
        RF::DeviceWaitIdle();
        std::vector<std::string> unusedEssenceNames {};
        for (auto & essenceAndVariants : mEssenceAndVariantsMap)
        {
            if (essenceAndVariants.second.variants.empty())
            {
                unusedEssenceNames.emplace_back(essenceAndVariants.second.essence->getNameId());
            }
        }
        for (auto const & nameOrAddress : unusedEssenceNames)
        {
            destroyEssence(nameOrAddress);
        }
    }

    //-------------------------------------------------------------------------------------------------

    void BaseMaterial::Init()
    {
        MFA_ASSERT(mIsInitialized == false);
        mIsInitialized = true;
        mDescriptorPool = RF::CreateDescriptorPool(mMaxSets);
    }

    //-------------------------------------------------------------------------------------------------

    void BaseMaterial::Shutdown()
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

    void BaseMaterial::changeActivationStatus(bool const enabled)
    {
        if (mIsActive == enabled)
        {
            return;
        }
        mIsActive = enabled;
        SceneManager::UpdatePipeline(this);
    }

    //-------------------------------------------------------------------------------------------------

    bool BaseMaterial::isActive() const
    {
        return mIsActive;
    }

    //-------------------------------------------------------------------------------------------------

    bool BaseMaterial::addEssence(std::shared_ptr<EssenceBase> const & essence)
    {
        //MFA_ASSERT(mIsInitialized == true);
        auto const & address = essence->getNameId();

        bool success = false;
        if(MFA_VERIFY(mEssenceAndVariantsMap.find(address) == mEssenceAndVariantsMap.end()))
        {
            mEssenceAndVariantsMap[address] = EssenceAndVariants(essence);
            mAllEssencesList.emplace_back(essence.get());
            internalAddEssence(essence.get());
            success = true;
        }
        return success;
    }

    //-------------------------------------------------------------------------------------------------

    std::shared_ptr<EssenceBase> BaseMaterial::GetEssence(std::string const & nameId)
    {
        auto const findResult = mEssenceAndVariantsMap.find(nameId);
        if(findResult == mEssenceAndVariantsMap.end())
        {
            return nullptr;
        }
        return findResult->second.essence;
    }

    //-------------------------------------------------------------------------------------------------

}
