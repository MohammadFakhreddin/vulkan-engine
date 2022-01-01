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

    void BasePipeline::PreRender(RT::CommandRecordState & drawPass, float deltaTime)
    {}

    //-------------------------------------------------------------------------------------------------

    void BasePipeline::Render(RT::CommandRecordState & drawPass, float deltaTime)
    {}

    //-------------------------------------------------------------------------------------------------

    void BasePipeline::PostRender(RT::CommandRecordState & drawPass, float deltaTime)
    {}

    //-------------------------------------------------------------------------------------------------

    bool BasePipeline::CreateEssenceIfNotExists(
        std::shared_ptr<RT::GpuModel> const & gpuModel,
        std::shared_ptr<AS::MeshBase> const & cpuMesh
    )
    {
        if(mEssenceAndVariantsMap.contains(gpuModel->id))
        {
            return false;
        }
        mEssenceAndVariantsMap[gpuModel->id] = EssenceAndVariants {internalCreateEssence(gpuModel, cpuMesh)};

        return true;
    }

    //-------------------------------------------------------------------------------------------------

    void BasePipeline::DestroyEssence(RT::GpuModelId const id)
    {
        auto const findResult = mEssenceAndVariantsMap.find(id);
        if (MFA_VERIFY(findResult != mEssenceAndVariantsMap.end()) == false)
        {
            return;
        }
        for (auto & variant : findResult->second.variants)
        {
            RemoveVariant(*variant);
        }
        mEssenceAndVariantsMap.erase(findResult);
    }

    //-------------------------------------------------------------------------------------------------

    void BasePipeline::DestroyEssence(RT::GpuModel const & gpuModel)
    {
        DestroyEssence(gpuModel.id);
    }

    //-------------------------------------------------------------------------------------------------

    VariantBase * BasePipeline::CreateVariant(RT::GpuModel const & gpuModel)
    {
        return CreateVariant(gpuModel.id);
    }

    //-------------------------------------------------------------------------------------------------

    VariantBase * BasePipeline::CreateVariant(RT::GpuModelId const id)
    {
        auto const findResult = mEssenceAndVariantsMap.find(id);
        MFA_ASSERT(findResult != mEssenceAndVariantsMap.end());

        auto & variantsList = findResult->second.variants;
        variantsList.emplace_back(internalCreateVariant(findResult->second.essence.get()));

        auto * newVariant = variantsList.back().get();
        MFA_ASSERT(newVariant != nullptr);

        mAllVariantsList.emplace_back(newVariant);
        return newVariant;
    }

    //-------------------------------------------------------------------------------------------------

    void BasePipeline::RemoveVariant(VariantBase & variant)
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
            for (auto & variant : item.second.variants)
            {
                variant->Shutdown();
            }
        }
        mEssenceAndVariantsMap.clear();

        RF::DestroyDescriptorPool(mDescriptorPool);
    }

    //-------------------------------------------------------------------------------------------------

}
