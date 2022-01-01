#include "AssetShader.hpp"

#include "engine/BedrockMemory.hpp"

namespace MFA::AssetSystem
{
    //-------------------------------------------------------------------------------------------------

    Shader::Shader() = default;
    Shader::~Shader() = default;

    //-------------------------------------------------------------------------------------------------

    bool Shader::isValid() const
    {
        return mEntryPoint.empty() == false && 
        Stage::Invalid != mStage && 
        mCompiledShaderCode->memory.ptr != nullptr && 
        mCompiledShaderCode->memory.len > 0;
    }

    //-------------------------------------------------------------------------------------------------

    void Shader::init(char const * entryPoint, Stage const stage, std::shared_ptr<SmartBlob> compiledShaderCode)
    {
        mEntryPoint = entryPoint;
        mStage = stage;
        mCompiledShaderCode = std::move(compiledShaderCode);
    }

    //-------------------------------------------------------------------------------------------------

    CBlob Shader::getCompiledShaderCode() const noexcept
    {
        return mCompiledShaderCode->memory;
    }

    //-------------------------------------------------------------------------------------------------

    char const * Shader::getEntryPoint() const noexcept
    {
        return mEntryPoint.c_str();
    }

    //-------------------------------------------------------------------------------------------------

    Shader::Stage Shader::getStage() const noexcept
    {
        return mStage;
    }

}
