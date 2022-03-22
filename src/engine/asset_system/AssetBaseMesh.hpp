#pragma once

#include "AssetBase.hpp"

#include <cstdint>

namespace MFA
{
    struct SmartBlob;
}

namespace MFA::AssetSystem
{
    class MeshBase : public Base
    {

    public:

        explicit MeshBase();
        ~MeshBase() override;

        MeshBase(MeshBase const &) noexcept = delete;
        MeshBase(MeshBase &&) noexcept = delete;
        MeshBase & operator= (MeshBase const & rhs) noexcept = delete;
        MeshBase & operator= (MeshBase && rhs) noexcept = delete;

        virtual void initForWrite(
            uint32_t vertexCount,
            uint32_t indexCount,
            std::shared_ptr<SmartBlob> const & vertexBuffer,
            std::shared_ptr<SmartBlob> const & indexBuffer
        );

        virtual void finalizeData();

        [[nodiscard]]
        uint32_t getVertexCount() const;

        [[nodiscard]]
        SmartBlob const * getVertexData() const;

        [[nodiscard]]
        uint32_t getIndexCount() const;

        [[nodiscard]]
        SmartBlob const * getIndexBuffer() const;

        [[nodiscard]]
        virtual bool isValid() const;
        
    protected:

        uint32_t mVertexCount{};
        std::shared_ptr<SmartBlob> mVertexBuffer{}; // TODO: We should separate vertexBuffer and index buffer because most of the time we need them only to create gpu buffers

        uint32_t mIndexCount{};
        std::shared_ptr<SmartBlob> mIndexBuffer{};

    };
    
}

namespace MFA
{
    namespace AS = AssetSystem;
}
