#pragma once

#include "Application.hpp"

class PrefabEditorApplication final : public Application
{
public:
    
    explicit PrefabEditorApplication();
    ~PrefabEditorApplication() override;

protected:

    void internalInit() override;    

private:

};
