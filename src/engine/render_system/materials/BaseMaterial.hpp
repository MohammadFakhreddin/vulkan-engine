#pragma once

#include "engine/render_system/RenderTypesFWD.hpp"
#include "DescriptorSetSchema.hpp"
#include "engine/BedrockSignalTypes.hpp"

#include <memory>
#include <unordered_map>
#include <vector>

#define PIPELINE_PROPS(className, eventTypes, renderOrder)              \
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
EventType GetRequiredEvents() const override                            \
{                                                                       \
    return eventTypes;                                                  \
}                                                                       \
                                                                        \
[[nodiscard]]                                                           \
RenderOrder GetRenderOrder() const override                             \
{                                                                       \
    return renderOrder;                                                 \
}                                                                       \

namespace MFA
{

    class VariantBase;
    class EssenceBase;
    class BaseMaterial;
    namespace SceneManager
    {
        void UpdatePipeline(BaseMaterial * pipeline);
    }

    namespace AssetSystem
    {
        struct Model;
        class MeshBase;
    }

    class BaseMaterial
    {
    public:

        friend void SceneManager::UpdatePipeline(BaseMaterial * pipeline);

        using EventType = uint8_t;

        struct EventTypes {
            static constexpr EventType EmptyEvent = 0b0;
            //static constexpr EventType PreRenderEvent = 0b1;
            static constexpr EventType RenderEvent = 0b10;
            static constexpr EventType UpdateEvent = 0b100;
            static constexpr EventType ComputeEvent = 0b1000;
            // Resize is not included here because every pipeline requires the resize to be called
        };

        enum class RenderOrder : uint8_t
        {
            BeforeEverything = 0,
            DontCare = 1,
            AfterEverything = 2
        };

        explicit BaseMaterial(uint32_t maxSets);

        virtual ~BaseMaterial();

        BaseMaterial(BaseMaterial const &) noexcept = delete;
        BaseMaterial(BaseMaterial &&) noexcept = delete;
        BaseMaterial & operator = (BaseMaterial const &) noexcept = delete;
        BaseMaterial & operator = (BaseMaterial && rhs) noexcept = delete;

        [[nodiscard]]
        virtual EventType GetRequiredEvents() const
        {
            return EventTypes::EmptyEvent;
        }

        virtual void Render(
            RT::CommandRecordState & recordState,
            float deltaTime
        );

        virtual void Update(float deltaTime);

        virtual void compute(
            RT::CommandRecordState & recordState,
            float deltaTime
        );

        virtual std::shared_ptr<EssenceBase> CreateEssence(
            std::string const & nameId,
            std::shared_ptr<AssetSystem::Model> const & cpuModel,
            std::vector<std::shared_ptr<RT::GpuTexture>> const & gpuTextures
        ) = 0;

        bool addEssence(std::shared_ptr<EssenceBase> const & essence);

        [[nodiscard]]
        bool hasEssence(std::string const & nameId) const;

        // Editor only function
        void destroyEssence(std::string const & nameId);
 
        // TODO: I think we should return sharedPtr to avoid extra locking for initialization.
        virtual std::weak_ptr<VariantBase> createVariant(std::string const & path);

        void removeVariant(VariantBase & variant);

        virtual void onResize() = 0;

        [[nodiscard]]
        virtual char const * GetName() const = 0;

        virtual void freeUnusedEssences();

        virtual void Init();

        virtual void Shutdown();

        void changeActivationStatus(bool enabled);

        [[nodiscard]]
        bool isActive() const;

        [[nodiscard]]
        virtual RenderOrder GetRenderOrder() const = 0;

        std::shared_ptr<EssenceBase> GetEssence(std::string const & nameId);
        
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

        //SignalId mPreRenderListenerId = -1;
        SignalId mRenderListenerId = -1;
        SignalId mUpdateListenerId = -1;
        SignalId mComputeListenerId = -1;

        bool mIsActive = true;
    
    };

}
