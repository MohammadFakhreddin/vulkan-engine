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
    std::string const & GetNameOrAddress() const;

    [[nodiscard]]
    RT::GpuModel * GetGpuModel() const;

    RT::DescriptorSetGroup const & CreateDescriptorSetGroup(
        VkDescriptorPool descriptorPool,
        uint32_t descriptorSetCount,
        VkDescriptorSetLayout descriptorSetLayout
    );

    [[nodiscard]]
    RT::DescriptorSetGroup const & GetDescriptorSetGroup() const;

    void BindVertexBuffer(RT::CommandRecordState const & recordState) const;
    void BindIndexBuffer(RT::CommandRecordState const & recordState) const;
    void BindDescriptorSetGroup(RT::CommandRecordState const & recordState) const;
    void BindAllRenderRequiredData(RT::CommandRecordState const & recordState) const;

protected:

    std::shared_ptr<RT::GpuModel> const mGpuModel;

    RT::DescriptorSetGroup mDescriptorSetGroup {};
    bool mIsDescriptorSetGroupValid = false;

};

}
