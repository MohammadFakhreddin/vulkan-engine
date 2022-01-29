#pragma once

#include "Application.hpp"

class PrefabEditorScene;

class PrefabEditorApplication final : public Application
{
public:
    
    explicit PrefabEditorApplication();
    ~PrefabEditorApplication() override;

    PrefabEditorApplication(PrefabEditorApplication const &) noexcept = delete;
    PrefabEditorApplication(PrefabEditorApplication &&) noexcept = delete;
    PrefabEditorApplication & operator = (PrefabEditorApplication const &) noexcept = delete;
    PrefabEditorApplication & operator = (PrefabEditorApplication &&) noexcept = delete;

protected:

    void internalInit() override;    

private:

    std::shared_ptr<PrefabEditorScene> mPrefabEditorScene;
    

};
