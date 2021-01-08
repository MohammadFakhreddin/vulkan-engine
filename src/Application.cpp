#include "Application.hpp"

#include "engine/RenderFrontend.hpp"

void Application::run() {
    using namespace MFA;
    RenderFrontend::Init({800, 600, "Cool app"});
    RenderFrontend::Shutdown();
}

