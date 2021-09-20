#pragma once

#include "engine/Scene.hpp"

class Demo3rdPersonScene final : public MFA::Scene {
public:

    explicit Demo3rdPersonScene();

    ~Demo3rdPersonScene() override;

    Demo3rdPersonScene (Demo3rdPersonScene const &) noexcept = delete;
    Demo3rdPersonScene (Demo3rdPersonScene &&) noexcept = delete;
    Demo3rdPersonScene & operator = (Demo3rdPersonScene const &) noexcept = delete;
    Demo3rdPersonScene & operator = (Demo3rdPersonScene &&) noexcept = delete;

    void Init() override;

    void OnPreRender(float deltaTimeInSec, MFA::RT::DrawPass & drawPass) override;

    void OnRender(float deltaTimeInSec, MFA::RT::DrawPass & drawPass) override;

    void OnPostRender(float deltaTimeInSec, MFA::RT::DrawPass & drawPass) override;

    void Shutdown() override;

    void OnResize() override;

private:

    

};
