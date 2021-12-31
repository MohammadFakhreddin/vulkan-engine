#pragma once

#include "engine/render_system/RenderTypes.hpp"

#include <memory>

namespace MFA
{

class Essence
{
public:

    explicit Essence(std::shared_ptr<RT::GpuModel> gpuModel);
    virtual ~Essence();

    Essence & operator= (Essence && rhs) noexcept = delete;
    Essence (Essence const &) noexcept = delete;
    Essence (Essence && rhs) noexcept = delete;
    Essence & operator = (Essence const &) noexcept = delete;

    [[nodiscard]]
    RT::GpuModelId GetId() const;

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
