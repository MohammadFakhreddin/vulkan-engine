#pragma once

#include "engine/render_system/RenderTypes.hpp"

namespace MFA
{

class EssenceBase
{
public:

    explicit EssenceBase(std::string nameId);
    virtual ~EssenceBase();

    EssenceBase & operator= (EssenceBase && rhs) noexcept = delete;
    EssenceBase (EssenceBase const &) noexcept = delete;
    EssenceBase (EssenceBase && rhs) noexcept = delete;
    EssenceBase & operator = (EssenceBase const &) noexcept = delete;

    [[nodiscard]]
    std::string const & getNameId() const;

protected:

    std::string mNameId; // Refers to address or unique name
};

}
