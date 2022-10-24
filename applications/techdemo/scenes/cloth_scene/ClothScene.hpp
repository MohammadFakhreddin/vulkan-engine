#pragma once

#include "engine/scene_manager/Scene.hpp"

class ClothScene final : public MFA::Scene
{
public:

    explicit ClothScene();
    ~ClothScene() override;

    ClothScene(ClothScene const &) noexcept = delete;
    ClothScene(ClothScene &&) noexcept = delete;
    ClothScene & operator = (ClothScene const &) noexcept = delete;
    ClothScene & operator = (ClothScene &&) noexcept = delete;

    void Init() override;

    void Update(float deltaTimeInSec) override;

    void Shutdown() override;

    bool RequiresUpdate() override;

};
