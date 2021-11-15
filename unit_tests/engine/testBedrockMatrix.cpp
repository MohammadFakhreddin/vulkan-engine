//======================================================================
// 
//======================================================================

#include "catch.hpp"

#include "engine/BedrockMatrix.hpp"
#include "engine/render_system/RenderTypes.hpp"

//======================================================================

TEST_CASE("Matrix TestCase1", "[Matrix][0]") {
    using namespace MFA;
    // TODO Not working Fix it!
    glm::mat4 forwardVectorMat {};
    forwardVectorMat[0][0] = RT::ForwardVector[0];
    forwardVectorMat[0][1] = RT::ForwardVector[1];
    forwardVectorMat[0][2] = RT::ForwardVector[2];

    glm::vec3 direction {1.0f, 1.0f, 1.0f};

    glm::mat4 directionVectorMat {};
    directionVectorMat[0][0] = direction[0];
    directionVectorMat[0][1] = direction[1];
    directionVectorMat[0][2] = direction[2];


    glm::mat4 const rotationMatrix = forwardVectorMat * glm::inverse(directionVectorMat);

    glm::vec3 const generatedDirectionVector = rotationMatrix * RT::ForwardVector;
    REQUIRE(Matrix::IsEqual(generatedDirectionVector, direction));

}

