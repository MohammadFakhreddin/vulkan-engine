#pragma once

#include "engine/Scene.hpp"

class GLTFMeshViewerScene;
class SpecularHighlightScene;
class PBRScene;
class TexturedSphereScene;
class TextureViewerScene;

class Application {
public:
    Application();
    ~Application();

    Application & operator= (Application && rhs) noexcept;
    Application (Application const &) noexcept = delete;
    Application (Application && rhs) noexcept;
    Application & operator = (Application const &) noexcept = delete;

    void run();
private:

    MFA::SceneSubSystem mSceneSubSystem {};

    std::unique_ptr<GLTFMeshViewerScene> mGltfMeshViewerScene;
    std::unique_ptr<SpecularHighlightScene> mSpecularHighlightScene;
    std::unique_ptr<PBRScene> mPbrScene;
    std::unique_ptr<TexturedSphereScene> mTexturedSphereScene;
    std::unique_ptr<TextureViewerScene> mTextureViewerScene;
};
