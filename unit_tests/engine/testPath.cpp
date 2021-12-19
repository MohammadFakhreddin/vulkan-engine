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

    auto const relativePath = Path::RelativeToAssetFolder(absolutePath.c_str());
    REQUIRE(relativePath == "models\\mira\\scene.gltf");

    Path::Shutdown();
}

