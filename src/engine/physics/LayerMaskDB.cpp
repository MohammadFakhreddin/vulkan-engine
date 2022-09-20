#include "LayerMaskDB.hpp"

#include "engine/BedrockLog.hpp"

namespace MFA::Physics::LayerMaskDB
{

    static constexpr uint8_t LayerCount = 32;   // Unity only had 32 layer, We won't need more

    struct LayerInfo
    {
        bool isUsed = false;
        uint8_t index{};
        uint32_t mask{};
        std::string name{};
    };

    struct State
    {
        LayerInfo layerInfos[LayerCount] {};
    };

    State * state = nullptr;

    //=================================================================================================

    static LayerInfo const * FindLayer(std::string const & layerName)
    {
        for (auto const & layerInfo : state->layerInfos)
        {
            if (layerInfo.name == layerName)
            {
                return &layerInfo;
            }
        }

        MFA_LOG_WARN("Layer with name %s was not found.", layerName.c_str());
        return nullptr;
    }

    //=================================================================================================

    void Init()
    {
        state = new State();
        for (uint8_t i = 0; i < LayerCount; ++i)
        {
            auto & layerInfo = state->layerInfos[i];
            layerInfo.mask = 1 << i;
            layerInfo.index = i;
        }
    }

    //=================================================================================================

    void Shutdown()
    {
        delete state;    
    }

    //=================================================================================================

    bool Create(std::string const & layerName, uint8_t layerNumber)
    {
        if (layerName.empty())
        {
            MFA_LOG_WARN("Layer name cannot be empty");
            return false;
        }

        if (layerNumber >= 32)
        {
            MFA_LOG_WARN("Layer number is more than or equal 32 %d", static_cast<int>(layerNumber));
            return false;
        }

        for (auto const & layerInfo : state->layerInfos)
        {
            if (layerInfo.name == layerName)
            {
                MFA_LOG_WARN("Another layer with name %s already exists.", layerName.c_str());
                return false;
            }
        }

        auto & layerInfo = state->layerInfos[layerNumber];
        if (layerInfo.isUsed)
        {
            MFA_LOG_WARN("Layer number %d is already used.", static_cast<int>(layerNumber));
            return false;
        }

        layerInfo.isUsed = true;
        layerInfo.name = layerName;

        return true;
    }

    //=================================================================================================

    LayerMask GetMask(std::string const & layerName)
    {
        auto * layerInfo = FindLayer(layerName);

        if (layerInfo != nullptr)
        {
            return LayerMask {layerInfo->mask};
        }

        return LayerMask {};
    }

    //=================================================================================================

    uint8_t NameToLayer(std::string const & layerName)
    {
        auto * layerInfo = FindLayer(layerName);

        if (layerInfo != nullptr)
        {
            return layerInfo->index;
        }

        return -1;
    }

    //=================================================================================================

    std::string LayerToName(uint8_t const layerIndex)
    {
        if (layerIndex >= 32)
        {
            return "";
        }

        auto & layerInfo = state->layerInfos[layerIndex];

        return layerInfo.name;
    }

    //=================================================================================================

}
