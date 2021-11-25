#pragma once

#include "Application.hpp"

#include <memory>

class GLTFMeshViewerScene;
class Demo3rdPersonScene;

class TechDemoApplication final : public Application
{
public:
    
    explicit TechDemoApplication();
    ~TechDemoApplication() override;

    static void OnUI();

protected:

    void internalInit() override;    

private:

    std::shared_ptr<Demo3rdPersonScene> mThirdPersonDemoScene;
    std::shared_ptr<GLTFMeshViewerScene> mGltfMeshViewerScene;

};
