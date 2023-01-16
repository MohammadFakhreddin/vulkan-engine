#include "DebugVariant.hpp"

#include "engine/asset_system/AssetDebugMesh.hpp"
#include "engine/entity_system/Entity.hpp"
#include "engine/entity_system/components/ColorComponent.hpp"
#include "engine/entity_system/components/TransformComponent.hpp"
#include "engine/render_system/pipelines/debug_renderer/DebugEssence.hpp"
#include "engine/BedrockMatrix.hpp"
#include "engine/render_system/RenderFrontend.hpp"

namespace MFA
{
    using namespace AS::Debug;
    
    //-------------------------------------------------------------------------------------------------

    DebugVariant::DebugVariant(DebugEssence const * essence)
        : VariantBase(essence)
        , mIndicesCount(essence->getIndicesCount())
    {}

    //-------------------------------------------------------------------------------------------------

    DebugVariant::~DebugVariant() = default;

    //-------------------------------------------------------------------------------------------------

    bool DebugVariant::getColor(float outColor[3]) const
    {
        if (auto const ptr = mColorComponent.lock())
        {
            ptr->GetColor(outColor);
            return true;
        }
        return false;
    }

    //-------------------------------------------------------------------------------------------------

    bool DebugVariant::getTransform(float outTransform[16]) const
    {
        if (auto const ptr = mTransformComponent.lock())
        {
            Copy<16>(outTransform, ptr->GetWorldTransform());
            return true;
        }
        return false;
    }

    //-------------------------------------------------------------------------------------------------

    void DebugVariant::Draw(RT::CommandRecordState const & recordState) const
    {
        RF::DrawIndexed(recordState, mIndicesCount);
    }

    //-------------------------------------------------------------------------------------------------

    void DebugVariant::internalInit()
    {
        VariantBase::internalInit();

        mColorComponent = mEntity->GetComponent<ColorComponent>();
        MFA_ASSERT(mColorComponent.expired() == false);
    }

    //-------------------------------------------------------------------------------------------------

}
