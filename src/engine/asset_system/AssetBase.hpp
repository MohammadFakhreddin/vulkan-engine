#pragma once

#include "engine/BedrockCommon.hpp"

namespace MFA::AssetSystem
{
    class Base {
    public:

        virtual ~Base() = default;

        [[nodiscard]]
        virtual int serialize(CBlob const & writeBuffer);

        // Maybe import BitStream from NSO project
        virtual void deserialize(CBlob const & readBuffer);
    };
}
