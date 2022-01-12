#pragma once

#include "engine/render_system/RenderTypes.hpp"

#include <memory>

namespace MFA
{

class EssenceBase
{
public:

    explicit EssenceBase(std::shared_ptr<RT::GpuModel> gpuModel);
    virtual ~EssenceBase();

    EssenceBase & operator= (EssenceBase && rhs) noexcept = delete;
    EssenceBase (EssenceBase const &) noexcept = delete;
    EssenceBase (EssenceBase && rhs) noexcept = delete;
    EssenceBase & operator = (EssenceBase const &) noexcept = delete;

    [[nodiscard]]
    std::string const & getNameOrAddress() const;

    [[nodiscard]]
    RT::GpuModel * getGpuModel() const;

    RT::DescriptorSetGroup const & createDescriptorSetGroup(
        VkDescriptorPool descriptorPool,
        uint32_t descriptorSetCount,
        RT::DescriptorSetLayoutGroup const & descriptorSetLayoutGroup
    );

    [[nodiscard]]
    RT::DescriptorSetGroup const & getDescriptorSetGroup() const;

    virtual void bindVertexBuffer(RT::CommandRecordState const & recordState) const = 0;
    void bindIndexBuffer(RT::CommandRecordState const & recordState) const;

    void bindDescriptorSetGroup(RT::CommandRecordState const & recordState) const;
    void bindAllRenderRequiredData(RT::CommandRecordState const & recordState) const;

protected:

    std::shared_ptr<RT::GpuModel> const mGpuModel;

    RT::DescriptorSetGroup mDescriptorSetGroup {};
    bool mIsDescriptorSetGroupValid = false;

};

}
