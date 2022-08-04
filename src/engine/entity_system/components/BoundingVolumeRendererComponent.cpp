#include "BoundingVolumeRendererComponent.hpp"

#include "engine/BedrockAssert.hpp"
#include "engine/render_system/pipelines/debug_renderer/DebugRendererPipeline.hpp"
#include "BoundingVolumeComponent.hpp"
#include "TransformComponent.hpp"
#include "engine/entity_system/Entity.hpp"
#include "engine/render_system/pipelines/VariantBase.hpp"
#include "engine/ui_system/UI_System.hpp"
#include "engine/scene_manager/SceneManager.hpp"
#include "engine/resource_manager/ResourceManager.hpp"

namespace MFA
{
    
    //-------------------------------------------------------------------------------------------------

    BoundingVolumeRendererComponent::BoundingVolumeRendererComponent(std::string const & nameId)
    {
        mPipeline = SceneManager::GetPipeline<DebugRendererPipeline>();
        MFA_ASSERT(mPipeline != nullptr);

        mNameId = nameId;
    }

    //-------------------------------------------------------------------------------------------------

    BoundingVolumeRendererComponent::~BoundingVolumeRendererComponent() = default;

    //-------------------------------------------------------------------------------------------------

    void BoundingVolumeRendererComponent::Init()
    {
        RendererComponent::Init();

        RC::AcquireEssence(mNameId, mPipeline, [this](bool const success)->void{
            MFA_ASSERT(JS::IsMainThread());
            MFA_ASSERT(success == true);
            mEssenceLoaded = true;
            if (mLateInitIsCalled)
            {
                createVariant();
            }
        });
    }

    //-------------------------------------------------------------------------------------------------

    void BoundingVolumeRendererComponent::LateInit()
    {
        RendererComponent::LateInit();
        mLateInitIsCalled = true;
        if (mEssenceLoaded)
        {
            createVariant();
        }
    }

    //-------------------------------------------------------------------------------------------------

    void BoundingVolumeRendererComponent::OnUI()
    {
        if (UI::TreeNode("BoundingVolumeRenderer"))
        {
            RendererComponent::OnUI();
            UI::TreePop();
        }
    }

    //-------------------------------------------------------------------------------------------------

    void BoundingVolumeRendererComponent::Clone(Entity * entity) const
    {
        entity->AddComponent<BoundingVolumeRendererComponent>(mNameId);
    }

    //-------------------------------------------------------------------------------------------------

    void BoundingVolumeRendererComponent::Shutdown()
    {
        RendererComponent::Shutdown();
    }

    //-------------------------------------------------------------------------------------------------

    void BoundingVolumeRendererComponent::createVariant()
    {
        auto const bvComponent = GetEntity()->GetComponent<BoundingVolumeComponent>();
        MFA_ASSERT(bvComponent != nullptr);

        auto const bvTransform = bvComponent->GetVolumeTransform();
        MFA_ASSERT(bvTransform.expired() == false);

        mVariant = mPipeline->createVariant(mNameId);

        auto const variant = mVariant.lock();
        MFA_ASSERT(variant != nullptr);

        variant->Init(
            GetEntity(),
            selfPtr(),
            bvTransform,
            bvComponent
        );
    }

    //-------------------------------------------------------------------------------------------------

}
