#pragma once

#include "Physics/ILayerMask.h"

#include <stdint.h>
#include <unordered_map>
#include <string>

class LayerMaskDB;

class LayerMask
{
    friend LayerMaskDB;
public:

    LayerMask() = default;

    [[nodiscard]]
    bool operator == (LayerMask const & other) const noexcept
    {
        return IsEqual(other);
    }

    [[nodiscard]]
    uint32_t GetValue() const noexcept
    {
        return mValue;
    }

    [[nodiscard]]
    bool HasCollision(const LayerMask & other) const noexcept
    {
        return (mValue & other.mValue) > 0;
    }

    [[nodiscard]]
    bool IsEqual(const LayerMask & other) const noexcept
    {
        return mValue == other.mValue;
    }

private:
    int8_t mLayerIndex = -1; // -1 means invalid
    uint32_t mValue{};
};

class LayerMaskDB : public ILayerMaskDB
{
public:
    explicit LayerMaskDB();

    /**
     * @param layerName Name of the value that must be unique
     * @param layerNumber Value between 0 and 31 (Inclusive)
     * \return Returns result
     */
    static bool Create(char const * layerName, uint8_t layerNumber);

    /**
     * @param maskNames Name of the created layers
     * \return All layers mask will be combined in 1 mask as result
     * \note Please be warned that you cannot use the result in LayerToName function
     */
    [[nodiscard]]
    static LayerMask GetMask(std::initializer_list<const char *> maskNames);

    [[nodiscard]]
    static LayerMask GetMask(const char * layerName);

    /**
     * @param layerName Name of the layer
     * \return Returns value between 0 to 31 (Inclusive) if found, otherwise it returns -1
     */
    [[nodiscard]]
    static int8_t NameToLayer(const char * layerName);

    /**
     * @param layerIndex LayerIndex is between 0 to 31 (Inclusive)
     * \return Returns layerName or empty if not found
     */
    [[nodiscard]]
    static std::string LayerToName(uint8_t layerIndex);

private:

    static constexpr uint8_t LayerCount = sizeof(uint32_t) * 8;

    [[nodiscard]]
    static uint32_t getMask(const char * layerName);

    std::unordered_map<std::string, uint8_t> mNameToLayerIndexMap{};
    struct LayerInfo
    {
        LayerInfo() = default;
        bool isUsed = false;
        uint8_t index{};
        uint32_t mask{};
        std::string name{};
    };
    LayerInfo mLayerInfos[LayerCount];

};
