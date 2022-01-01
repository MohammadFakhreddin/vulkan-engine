#include "AssetBase.hpp"

#include "engine/BedrockAssert.hpp"

namespace MFA::AssetSystem
{
    
    //-------------------------------------------------------------------------------------------------

    int Base::serialize(CBlob const & writeBuffer)
    {
        MFA_CRASH("Not implemented");
    }

    //-------------------------------------------------------------------------------------------------

    void Base::deserialize(CBlob const & readBuffer)
    {
        MFA_CRASH("Not implemented");
    }

    //-------------------------------------------------------------------------------------------------

}
