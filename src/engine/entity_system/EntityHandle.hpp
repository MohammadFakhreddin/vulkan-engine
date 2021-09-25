#pragma once

#include <cstdint>

namespace MFA {

class EntityHandle {
public:

    explicit EntityHandle(uint64_t const value)
        : mData(Data {.Value = value})
    {}

    explicit EntityHandle(uint32_t const index, uint32_t const version)
        : mData(Data {
            .IndexAndVersion = {
                .Index = index,
                .Version = version
            }
        }) {}

    [[nodiscard]]
    uint64_t GetValue() const noexcept {
        return mData.Value;
    }

    [[nodiscard]]
    uint32_t GetIndex() const noexcept {
        return mData.IndexAndVersion.Index;
    }

    [[nodiscard]]
    uint32_t GetVersion() const noexcept {
        return mData.IndexAndVersion.Version;
    }

private:

    union Data {
        struct IndexAndVersion {
            uint32_t Index;
            uint32_t Version;
        };
        uint64_t Value;
        IndexAndVersion IndexAndVersion;
    } mData;

};

}