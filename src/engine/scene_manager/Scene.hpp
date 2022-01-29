#pragma once

#include "engine/render_system/RenderTypes.hpp"

namespace MFA
{
    class BasePipeline;

    class CameraComponent;
    class Scene;
    class PointLightComponent;
    class DirectionalLightComponent;
    class SpotLightComponent;
    class Entity;

    class Scene
    {
    public:

        static constexpr int MAX_POINT_LIGHT_COUNT = 10;        // It can be more but currently 10 is more than enough for me
        static constexpr int MAX_DIRECTIONAL_LIGHT_COUNT = 3;

        static constexpr uint32_t POINT_LIGHT_SHADOW_WIDTH = 1024;
        static constexpr uint32_t POINT_LIGHT_SHADOW_HEIGHT = 1024;

        static constexpr uint32_t DIRECTIONAL_LIGHT_SHADOW_TEXTURE_WIDTH = 1024 * 10;
        static constexpr uint32_t DIRECTIONAL_LIGHT_SHADOW_TEXTURE_HEIGHT = 1024 * 10;
        static constexpr uint32_t DIRECTIONAL_LIGHT_PROJECTION_WIDTH = DIRECTIONAL_LIGHT_SHADOW_TEXTURE_WIDTH / 200;
        static constexpr uint32_t DIRECTIONAL_LIGHT_PROJECTION_HEIGHT = DIRECTIONAL_LIGHT_SHADOW_TEXTURE_HEIGHT / 200;

        explicit Scene() = default;
        virtual ~Scene() = default;
        Scene & operator= (Scene && rhs) noexcept = delete;
        Scene(Scene const &) noexcept = delete;
        Scene(Scene && rhs) noexcept = delete;
        Scene & operator = (Scene const &) noexcept = delete;

        virtual void OnPreRender(float deltaTimeInSec, RT::CommandRecordState & recordState);
        virtual void OnRender(float deltaTimeInSec, RT::CommandRecordState & drawPass) {}
        virtual void OnPostRender(float deltaTimeInSec) {}

        virtual void OnResize();;
        virtual void Init();
        virtual void Shutdown();

        void SetActiveCamera(std::weak_ptr<CameraComponent> const & camera);

        [[nodiscard]]
        std::weak_ptr<CameraComponent> const & GetActiveCamera() const;

        [[nodiscard]]
        Entity * GetRootEntity() const;

        [[nodiscard]]
        RT::UniformBufferGroup const & GetCameraBuffers() const;

        void RegisterPointLight(std::weak_ptr<PointLightComponent> const & pointLight);

        void RegisterDirectionalLight(std::weak_ptr<DirectionalLightComponent> const & directionalLight);

        [[nodiscard]]
        RT::UniformBufferGroup const & GetPointLightsBuffers() const;

        [[nodiscard]]
        RT::UniformBufferGroup const & GetDirectionalLightBuffers() const;

        [[nodiscard]]
        uint32_t getPointLightCount() const;

        [[nodiscard]]
        uint32_t GetDirectionalLightCount() const;

        void RegisterPipeline(BasePipeline * pipeline);

        [[nodiscard]]
        BasePipeline * GetPipelineByName(std::string const & name) const;

        [[nodiscard]]
        std::vector<BasePipeline *> GetPipelines() const;

        [[nodiscard]]
        std::vector<PointLightComponent *> const & getActivePointLights() const;

        virtual bool isDisplayPassDepthImageInitialLayoutUndefined() = 0;

    private:

        void prepareCameraBuffer();

        void preparePointLightsBuffer();

        void prepareDirectionalLightsBuffer();

        void updateCameraBuffer(RT::CommandRecordState const & recordState) const;

        void updatePointLightsBuffer(RT::CommandRecordState const & recordState);

        void updateDirectionalLightsBuffer(RT::CommandRecordState const & recordState);

        std::weak_ptr<CameraComponent> mActiveCamera{};

        // Scene root entity
        Entity * mRootEntity = nullptr;

        std::vector<std::weak_ptr<PointLightComponent>> mPointLightComponents{};
        std::vector<std::weak_ptr<DirectionalLightComponent>> mDirectionalLightComponents{};

        // Buffers 
        std::shared_ptr<RT::UniformBufferGroup> mCameraBuffer{};

        // Point light
        struct PointLight
        {
            float position[3]{};
            float placeholder0 = 0.0f;
            float color[3]{};
            float maxSquareDistance = 0.0f;
            float linearAttenuation = 0.0f;
            float quadraticAttenuation = 0.0f;
            float placeholder1[2]{};
            float viewProjectionMatrices[6][16]{};
        };
        struct PointLightsBufferData
        {
            uint32_t count = 0;
            float constantAttenuation = 1.0f;
            float placeholder[2]{};
            PointLight items[MAX_POINT_LIGHT_COUNT]{};
        };
        PointLightsBufferData mPointLightData{};
        std::vector<PointLightComponent *> mActivePointLights{};
        std::shared_ptr<RT::UniformBufferGroup> mPointLightsBuffers{};

        // https://stackoverflow.com/questions/9486364/why-cant-c-compilers-rearrange-struct-members-to-eliminate-alignment-padding
        // Directional light
        struct DirectionalLight
        {
            float direction[3]{};
            float placeholder0 = 0.0f;
            float color[3]{};
            float placeholder1 = 0.0f;
            float viewProjectionMatrix[16]{};
        };
        struct DirectionalLightData
        {
            uint32_t count = 0;
            float placeholder0 = 0.0f;
            float placeholder1 = 0.0f;
            float placeholder2 = 0.0f;
            DirectionalLight items[MAX_DIRECTIONAL_LIGHT_COUNT]{};
        };
        DirectionalLightData mDirectionalLightData{};
        std::shared_ptr<RT::UniformBufferGroup> mDirectionalLightBuffers{};

        // TODO Spot light

        // TODO Maybe we can have pipeline shared between entire application
        std::vector<BasePipeline *> mActivePipelines{};
    };


}
