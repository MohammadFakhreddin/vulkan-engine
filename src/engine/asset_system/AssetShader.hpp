#pragma once

#include "AssetBase.hpp"
#include "AssetTypes.hpp"

namespace MFA
{
    struct SmartBlob;
}

namespace MFA::AssetSystem
{
    class Shader final : public Base
    {
    public:

        using Stage = ShaderStage;

        explicit Shader(
            std::string entryPoint_,
            Stage stage_,
            std::shared_ptr<SmartBlob> compiledShaderCode_
        );
        ~Shader() override;

        Shader(Shader const &) noexcept = delete;
        Shader(Shader &&) noexcept = delete;
        Shader & operator= (Shader const & rhs) noexcept = delete;
        Shader & operator= (Shader && rhs) noexcept = delete;

        [[nodiscard]]
        bool isValid() const;

        std::string const entryPoint;     // Ex: main
        Stage const stage;
        std::shared_ptr<SmartBlob> const compiledShaderCode;

    };

}


namespace MFA
{
    namespace AS = AssetSystem;
}
