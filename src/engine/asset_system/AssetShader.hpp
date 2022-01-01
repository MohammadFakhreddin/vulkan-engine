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
        
        explicit Shader();
        ~Shader() override;

        Shader(Shader const &) noexcept = delete;
        Shader(Shader &&) noexcept = delete;
        Shader & operator= (Shader const & rhs) noexcept = delete;
        Shader & operator= (Shader && rhs) noexcept = delete;

        [[nodiscard]]
        bool isValid() const;

        void init(
            char const * entryPoint,
            Stage stage,
            std::shared_ptr<SmartBlob> compiledShaderCode
        );

        [[nodiscard]]
        CBlob getCompiledShaderCode() const noexcept;

        [[nodiscard]]
        char const * getEntryPoint() const noexcept;

        [[nodiscard]]
        Stage getStage() const noexcept;

    private:

        std::string mEntryPoint{};     // Ex: main
        Stage mStage = Stage::Invalid;
        std::shared_ptr<SmartBlob> mCompiledShaderCode = nullptr;

    };

}


namespace MFA
{
    namespace AS = AssetSystem;
}
