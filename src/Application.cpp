#include "Application.hpp"

#include "engine/RenderFrontend.hpp"

Application::Application() {
    using namespace MFA;
    RenderFrontend::Init({800, 600, "Cool app"});
}

void Application::run() {
    
}

