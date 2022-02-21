#pragma once

#include "engine/render_system/RenderTypes.hpp"

namespace MFA
{
    class Entity;
    class RendererComponent;
    class TransformComponent;
    class BoundingVolumeComponent;
    class EssenceBase;

    class VariantBase
    {
    public:
        
        explicit VariantBase(EssenceBase const * essence);
        virtual ~VariantBase();

        VariantBase(VariantBase const &) noexcept = delete;
        VariantBase(VariantBase &&) noexcept = delete;
        VariantBase & operator= (VariantBase const & rhs) noexcept = delete;
        VariantBase & operator= (VariantBase && rhs) noexcept = delete;

        bool operator== (VariantBase const & rhs) const noexcept;

        void Init(
            Entity * entity,
            std::weak_ptr<RendererComponent> const & rendererComponent,
            std::weak_ptr<TransformComponent> const & transformComponent,
            std::weak_ptr<BoundingVolumeComponent> const & boundingVolumeComponent
        );

        virtual void Update(float deltaTimeInSec, RT::CommandRecordState const & drawPass) {}

        void Shutdown();

        [[nodiscard]]
        EssenceBase const * GetEssence() const noexcept;

        [[nodiscard]]
        RT::VariantId GetId() const noexcept;

        RT::DescriptorSetGroup const & CreateDescriptorSetGroup(
            VkDescriptorPool descriptorPool,
            uint32_t descriptorSetCount,
            RT::DescriptorSetLayoutGroup const & descriptorSetLayout
        );

        [[nodiscard]]
        RT::DescriptorSetGroup const & GetDescriptorSetGroup() const;

        [[nodiscard]]
        bool IsActive() const noexcept;

        [[nodiscard]]
        bool IsInFrustum() const;

        [[nodiscard]]
        Entity * GetEntity() const;

        [[nodiscard]]
        bool IsVisible() const;

        [[nodiscard]]
        bool IsOccluded() const;

        void SetIsOccluded(bool isOccluded);

        [[nodiscard]]
        BoundingVolumeComponent * GetBoundingVolume() const;

        virtual void OnUI() {}

    protected:

        virtual void internalInit() {}

        virtual void internalShutdown() {}

        bool mIsInitialized = false;
        RT::VariantId mId = 0;
        EssenceBase const * mEssence = nullptr;
        Entity * mEntity = nullptr;

        std::weak_ptr<RendererComponent> mRendererComponent{};
        std::weak_ptr<BoundingVolumeComponent> mBoundingVolumeComponent{};
        std::weak_ptr<TransformComponent> mTransformComponent{};
        int mTransformListenerId = 0;
        RT::DescriptorSetGroup mDescriptorSetGroup{};

        bool mIsModelTransformChanged = true;
        bool mIsOccluded = false;

    };
}
