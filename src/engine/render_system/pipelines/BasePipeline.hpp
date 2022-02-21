#pragma once

#include "engine/render_system/RenderTypesFWD.hpp"
#include "DescriptorSetSchema.hpp"
#include "engine/BedrockSignalTypes.hpp"

#include <memory>
#include <unordered_map>
#include <vector>

#define PIPELINE_PROPS(className, eventTypes, renderOrder_)             \
                                                                        \
static constexpr char const * Name = #className;                        \
char const * GetName() const override {                                 \
    return Name;                                                        \
}                                                                       \
                                                                        \
className (className const &) noexcept = delete;                        \
className (className &&) noexcept = delete;                             \
className & operator = (className const &) noexcept = delete;           \
className & operator = (className &&) noexcept = delete;                \
                                                                        \
[[nodiscard]]                                                           \
EventType requiredEvents() const override                               \
{                                                                       \
    return eventTypes;                                                  \
}                                                                       \
                                                                        \
[[nodiscard]]                                                           \
RenderOrder renderOrder() const {                                       \
    return renderOrder_;                                                \
}                                                                       \

namespace MFA
{

    class VariantBase;
    class EssenceBase;
    class BasePipeline;
    namespace SceneManager
    {
        void UpdatePipeline(BasePipeline * pipeline);
    }

    namespace AssetSystem
    {
        class MeshBase;
    }

    class BasePipeline
    {
    public:

        friend void SceneManager::UpdatePipeline(BasePipeline * pipeline);

        using EventType = uint8_t;

        struct EventTypes {
            static constexpr EventType EmptyEvent = 0b0;
            static constexpr EventType PreRenderEvent = 0b1;
            static constexpr EventType RenderEvent = 0b10;
            static constexpr EventType PostRenderEvent = 0b100;
            // Resize is not included here because every pipeline requires the resize to be called
        };

        enum class RenderOrder : uint8_t
        {
            BeforeEverything = 0,
            DontCare = 1,
            AfterEverything = 2
        };

        explicit BasePipeline(uint32_t maxSets);

        virtual ~BasePipeline();

        BasePipeline(BasePipeline const &) noexcept = delete;
        BasePipeline(BasePipeline &&) noexcept = delete;
        BasePipeline & operator = (BasePipeline const &) noexcept = delete;
        BasePipeline & operator = (BasePipeline && rhs) noexcept = delete;

        [[nodiscard]]
        virtual EventType requiredEvents() const
        {
            return EventTypes::EmptyEvent;
        }

        virtual void preRender(
            RT::CommandRecordState & recordState,
            float deltaTime
        );

        virtual void render(
            RT::CommandRecordState & recordState,
            float deltaTime
        );

        virtual void postRender(float deltaTime);

        bool addEssence(std::shared_ptr<EssenceBase> const & essence);

        [[nodiscard]]
        bool essenceExists(std::string const & nameOrAddress) const;

        // Editor only function
        void destroyEssence(std::string const & nameOrAddress);

        void destroyEssence(RT::GpuModel const & gpuModel);

        virtual std::weak_ptr<VariantBase> createVariant(RT::GpuModel const & gpuModel);

        virtual std::weak_ptr<VariantBase> createVariant(std::string const & nameOrAddress);

        void removeVariant(VariantBase & variant);

        virtual void onResize() = 0;

        [[nodiscard]]
        virtual char const * GetName() const = 0;

        virtual void freeUnusedEssences();

        virtual void init();

        virtual void shutdown();

        void changeActivationStatus(bool enabled);

        [[nodiscard]]
        bool isActive() const;

        [[nodiscard]]
        virtual RenderOrder renderOrder() const = 0;
    
    protected:

        virtual void internalAddEssence(EssenceBase * essence) = 0;

        virtual std::shared_ptr<VariantBase> internalCreateVariant(EssenceBase * essence) = 0;

        
        struct EssenceAndVariants
        {
            std::shared_ptr<EssenceBase> essence;
            std::vector<std::shared_ptr<VariantBase>> variants;

            explicit EssenceAndVariants();
            explicit EssenceAndVariants(std::shared_ptr<EssenceBase> essence);
        };
        std::unordered_map<std::string, EssenceAndVariants> mEssenceAndVariantsMap;

        std::vector<VariantBase *> mAllVariantsList{};
        std::vector<EssenceBase *> mAllEssencesList{};

        VkDescriptorPool mDescriptorPool{};
        uint32_t mMaxSets;

        bool mIsInitialized = false;

        SignalId mPreRenderListenerId = -1;
        SignalId mRenderListenerId = -1;
        SignalId mPostRenderListenerId = -1;

        bool mIsActive = true;
    
    };

}
