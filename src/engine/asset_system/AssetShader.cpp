#include "AssetShader.hpp"

#include "engine/BedrockMemory.hpp"

namespace MFA::AssetSystem
{
    //-------------------------------------------------------------------------------------------------

    Shader::Shader(
        std::string entryPoint_,
        Stage const stage_,
        std::shared_ptr<SmartBlob> compiledShaderCode_
    )
        : entryPoint(std::move(entryPoint_))
        , stage(stage_)
        , compiledShaderCode(std::move(compiledShaderCode_))
    {}

    //-------------------------------------------------------------------------------------------------

    Shader::~Shader() = default;

    //-------------------------------------------------------------------------------------------------

    bool Shader::isValid() const
    {
        return entryPoint.empty() == false &&
            Stage::Invalid != stage &&
            compiledShaderCode->memory.ptr != nullptr &&
            compiledShaderCode->memory.len > 0;
    }

    //-------------------------------------------------------------------------------------------------

}
