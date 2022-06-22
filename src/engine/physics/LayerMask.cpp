#include "LayerMask.h"

#include "Core/NsoAssert.h"

LayerMaskDB * instance = nullptr;

//=================================================================================================

LayerMaskDB::LayerMaskDB()
{
    instance = this;
    for (uint8_t i = 0; i < LayerCount; ++i)
    {
        mLayerInfos[i].mask = 1 << i;
        mLayerInfos[i].index = i;
    }
}

bool LayerMaskDB::Create(char const * layerName, const uint8_t layerNumber)
{
    bool success = false;
    if (NSO_VERIFY(
        layerName != nullptr &&
        strlen(layerName) > 0 &&
        instance->mNameToLayerIndexMap.find(layerName) == instance->mNameToLayerIndexMap.end() &&
        layerNumber < 32
    ))
    {
        auto & layerInfo = instance->mLayerInfos[layerNumber];
        if (NSO_VERIFY(layerInfo.isUsed == false))
        {
            layerInfo.isUsed = true;
            layerInfo.name = layerName;
            success = true;
            instance->mNameToLayerIndexMap[layerName] = layerInfo.index;
        }
    } 
    return success;
}

//=================================================================================================

LayerMask LayerMaskDB::GetMask(std::initializer_list<const char *> maskNames)
{
    LayerMask mask {};
    for (const auto * maskName : maskNames)
    {
        mask.mValue |= getMask(maskName);
    }
    return mask;
}

//=================================================================================================

LayerMask LayerMaskDB::GetMask(const char * layerName)
{
    LayerMask mask {};
    const auto findResult = instance->mNameToLayerIndexMap.find(layerName);
    if (NSO_VERIFY(findResult != instance->mNameToLayerIndexMap.end()))
    {
        const auto & layerInfo = instance->mLayerInfos[findResult->second];
        mask.mValue = layerInfo.mask;
        mask.mLayerIndex = layerInfo.index;
    }
    return mask;
}

//=================================================================================================

int8_t LayerMaskDB::NameToLayer(const char * layerName)
{
    int8_t layerIndex = -1;
    const auto findResult = instance->mNameToLayerIndexMap.find(layerName);
    if (NSO_VERIFY(findResult != instance->mNameToLayerIndexMap.end()))
    {
        layerIndex = findResult->second;
    }
    return layerIndex;
}

//=================================================================================================

std::string LayerMaskDB::LayerToName(const uint8_t layerIndex)
{
    if (NSO_VERIFY(layerIndex >= 0 && layerIndex < LayerCount))
    {
        return instance->mLayerInfos[layerIndex].name;
    }
    return "";
}

//=================================================================================================

uint32_t LayerMaskDB::getMask(char const * layerName)
{
    uint32_t maskValue = 0;
    const auto findResult = instance->mNameToLayerIndexMap.find(layerName);
    if (NSO_VERIFY(findResult != instance->mNameToLayerIndexMap.end()))
    {
        maskValue = instance->mLayerInfos[findResult->second].mask;
    }
    return maskValue;
}
