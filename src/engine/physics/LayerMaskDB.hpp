#pragma once

#include "LayerMask.hpp"

#include <cstdint>
#include <string>

namespace MFA::Physics::LayerMaskDB
{
    
    void Init();

    void Shutdown();

    /**
     * @param layerName Name of the value that must be unique
     * @param layerNumber Value between 0 and 31 (Inclusive)
     * \return Returns result
     */
    static bool Create(std::string const & layerName, uint8_t layerNumber);

    /**
     * @param maskNames Name of the created layers
     * \return All layers mask will be combined in 1 mask as result
     * \note Please be warned that you cannot use the result in LayerToName function
     */
    template<typename T>
    [[nodiscard]]
    static LayerMask GetMask(T const & maskNames)
    {
        LayerMask mask{};
        for (const auto * maskName : maskNames)
        {
            mask |= getMask(maskName);
        }
        return mask;
    }

    [[nodiscard]]
    static LayerMask GetMask(std::string const & layerName);

    /**
     * @param layerName Name of the layer
     * \return Returns value between 0 to 31 (Inclusive) if found, otherwise it returns 255 (-1)
     */
    [[nodiscard]]
    static uint8_t NameToLayer(std::string const & layerName);

    /**
     * @param layerIndex LayerIndex is between 0 to 31 (Inclusive)
     * \return Returns layerName or empty if not found
     */
    [[nodiscard]]
    static std::string LayerToName(uint8_t layerIndex);

};
