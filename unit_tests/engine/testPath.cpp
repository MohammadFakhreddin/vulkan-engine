//======================================================================
// 
//======================================================================

#include "catch.hpp"

#include "engine/BedrockPath.hpp"

//======================================================================

TEST_CASE("Path TestCase1", "[Path][0]") {
    using namespace MFA;

    Path::Init();

    std::string const absolutePath = "C:\\projects\\vulkan-engine\\assets\\models\\mira\\scene.gltf";

    std::string relativePath = "";
    Path::RelativeToAssetFolder(absolutePath.c_str(), relativePath);
    REQUIRE(relativePath == "models\\mira\\scene.gltf");

    Path::Shutdown();
}

