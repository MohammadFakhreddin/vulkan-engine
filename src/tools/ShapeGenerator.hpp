#pragma once

#include "./engine/FoundationAsset.hpp"

namespace MFA::ShapeGenerator {
    /*
     * Generates sphere with radius of 1, Compatible with texture materials
     */
    AssetSystem::Model Sphere(float scaleFactor = 1);

    AssetSystem::Model Sheet();

}