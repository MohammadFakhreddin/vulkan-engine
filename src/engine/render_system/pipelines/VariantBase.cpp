#include "VariantBase.hpp"

#include "engine/BedrockAssert.hpp"
#include "engine/entity_system/components/BoundingVolumeComponent.hpp"
#include "engine/entity_system/components/RendererComponent.hpp"
#include "engine/entity_system/components/TransformComponent.hpp"
#include "engine/render_system/RenderFrontend.hpp"

//-------------------------------------------------------------------------------------------------

MFA::VariantBase::VariantBase(EssenceBase const * essence)
    : mEssence(essence)
{
    static int NextId = 0;
    mId = ++NextId;
}

//-------------------------------------------------------------------------------------------------

MFA::VariantBase::~VariantBase()
{
    if (mIsInitialized == true)
    {
        Shutdown();
    }
}


//-------------------------------------------------------------------------------------------------

bool MFA::VariantBase::operator==(VariantBase const & rhs) const noexcept
{
    return mId == rhs.mId;
}

//-------------------------------------------------------------------------------------------------

void MFA::VariantBase::Init(
    Entity * entity,
    std::weak_ptr<RendererComponent> const & rendererComponent,
    std::weak_ptr<TransformComponent> const & transformComponent,
    std::weak_ptr<BoundingVolumeComponent> const & boundingVolumeComponent
)
{
    if (mIsInitialized)
    {
        return;
    }
    mIsInitialized = true;

    MFA_ASSERT(entity != nullptr);
    mEntity = entity;

    MFA_ASSERT(rendererComponent.expired() == false);
    mRendererComponent = rendererComponent;

    MFA_ASSERT(transformComponent.expired() == false);
    mTransformComponent = transformComponent;

    mTransformListenerId = mTransformComponent.lock()->RegisterChangeListener([this]()->void
        {
            mIsModelTransformChanged = true;
        }
    );

    MFA_ASSERT(boundingVolumeComponent.expired() == false);
    mBoundingVolumeComponent = boundingVolumeComponent;
}

//-------------------------------------------------------------------------------------------------

void MFA::VariantBase::Shutdown()
{
    if (mIsInitialized == false)
    {
        return;
    }
    mIsInitialized = false;

    if (auto const ptr = mRendererComponent.lock())
    {
        ptr->notifyVariantDestroyed();
    }

    if (auto const ptr = mTransformComponent.lock())
    {
        ptr->UnRegisterChangeListener(mTransformListenerId);
    }
}

//-------------------------------------------------------------------------------------------------

MFA::EssenceBase const * MFA::VariantBase::GetEssence() const noexcept
{
    return mEssence;
}

//-------------------------------------------------------------------------------------------------

MFA::RenderTypes::VariantId MFA::VariantBase::GetId() const noexcept
{
    return mId;
}

//-------------------------------------------------------------------------------------------------

MFA::RenderTypes::DescriptorSetGroup const & MFA::VariantBase::CreateDescriptorSetGroup(
    VkDescriptorPool descriptorPool,
    uint32_t descriptorSetCount,
    VkDescriptorSetLayout descriptorSetLayout
)
{
    MFA_ASSERT(mDescriptorSetGroup.IsValid() == false);
    mDescriptorSetGroup = RF::CreateDescriptorSets(
        descriptorPool,
        descriptorSetCount,
        descriptorSetLayout
    );
    MFA_ASSERT(mDescriptorSetGroup.IsValid() == true);
    return mDescriptorSetGroup;
}

//-------------------------------------------------------------------------------------------------

MFA::RenderTypes::DescriptorSetGroup const & MFA::VariantBase::GetDescriptorSetGroup() const
{
    MFA_ASSERT(mDescriptorSetGroup.IsValid() == true);
    return mDescriptorSetGroup;
}

//-------------------------------------------------------------------------------------------------

bool MFA::VariantBase::IsActive() const noexcept
{
    if (auto const ptr = mRendererComponent.lock())
    {
        return ptr->IsActive();
    }
    return false;
}

//-------------------------------------------------------------------------------------------------

bool MFA::VariantBase::IsInFrustum() const
{
    bool isInFrustum = true;
    if (auto const ptr = mBoundingVolumeComponent.lock())
    {
        isInFrustum = ptr->IsInFrustum();   
    }
    return isInFrustum;
}

//-------------------------------------------------------------------------------------------------

MFA::Entity * MFA::VariantBase::GetEntity() const
{
    return mEntity;
}

//-------------------------------------------------------------------------------------------------

bool MFA::VariantBase::IsVisible() const
{
    return IsActive() && mIsOccluded == false && IsInFrustum();
}

//-------------------------------------------------------------------------------------------------

bool MFA::VariantBase::IsOccluded() const
{
    return mIsOccluded;
}

//-------------------------------------------------------------------------------------------------

void MFA::VariantBase::SetIsOccluded(bool const isOccluded)
{
    mIsOccluded = isOccluded;
}

//-------------------------------------------------------------------------------------------------

MFA::BoundingVolumeComponent * MFA::VariantBase::GetBoundingVolume() const
{
    return mBoundingVolumeComponent.lock().get();
}

//-------------------------------------------------------------------------------------------------
