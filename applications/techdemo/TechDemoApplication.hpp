#pragma once

#include "Application.hpp"

class TechDemoApplication final : public Application
{
public:
    
    explicit TechDemoApplication();
    ~TechDemoApplication() override;

    static void OnUI();

protected:

    void internalInit() override;    

private:

};
