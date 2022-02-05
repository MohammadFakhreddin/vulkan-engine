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

    class  Scene
    {
    public:

        explicit Scene() = default;
        virtual ~Scene() = default;
        Scene & operator= (Scene && rhs) noexcept = delete;
        Scene(Scene const &) noexcept = delete;
        Scene(Scene && rhs) noexcept = delete;
        Scene & operator = (Scene const &) noexcept = delete;

        virtual bool RequiresUpdate() {return false;}
        virtual void Update(float deltaTimeInSec) {}

        virtual void Init();
        virtual void Shutdown();

        void SetActiveCamera(std::weak_ptr<CameraComponent> const & camera);

        [[nodiscard]]
        std::weak_ptr<CameraComponent> const & GetActiveCamera() const;

        [[nodiscard]]
        Entity * GetRootEntity() const;

        // TODO: We should detect initial layout dynamically
        virtual bool isDisplayPassDepthImageInitialLayoutUndefined() = 0;

    private:

        std::weak_ptr<CameraComponent> mActiveCamera{};

        // Scene root entity
        Entity * mRootEntity = nullptr;

    };


}
