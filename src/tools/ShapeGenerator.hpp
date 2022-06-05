#pragma once

#include "engine/asset_system/AssetModel.hpp"

#include <memory>

namespace MFA::ShapeGenerator {
    namespace Debug // This shapes are for debug pipeline
    {
        /*
         * Generates sphere with radius of 1, Compatible with texture materials
         */
        std::shared_ptr<AS::Model> Sphere();

        std::shared_ptr<AS::Model> Sheet();

        std::shared_ptr<AS::Model> CubeStrip();

        std::shared_ptr<AS::Model> CubeFill();

    }

}
