#pragma once

#include "engine/render_system/pipelines/VariantBase.hpp"

namespace MFA
{
    class ColorComponent;
    class DebugEssence;

    class DebugVariant final : public VariantBase
    {
    public:

        explicit DebugVariant(DebugEssence const * essence);
        ~DebugVariant() override;

        DebugVariant(DebugVariant const &) noexcept = delete;
        DebugVariant(DebugVariant &&) noexcept = delete;
        DebugVariant & operator= (DebugVariant const & rhs) noexcept = delete;
        DebugVariant & operator= (DebugVariant && rhs) noexcept = delete;

        bool getColor(float outColor[3]) const;

        bool getTransform(float outTransform[16]) const;

        void Draw(RT::CommandRecordState const & recordState) const;

    protected:

        void internalInit() override;

    private:

        std::weak_ptr<ColorComponent> mColorComponent {};
        uint32_t mIndicesCount = 0;

    };
}
