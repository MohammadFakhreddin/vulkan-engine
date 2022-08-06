#pragma once

#include "Collider.h"

#include <glm/vec3.hpp>

class BoxCollider final : public Collider, public ICollisionListener
{
public:

    void SetSize(const glm::vec3 &size)
    {
        mSize = size;
    }

    [[nodiscard]]
    glm::vec3 GetSize() const noexcept
    {
        return mSize;
    }

    void Deserialize(const FileLoaderType::JsonObject &jsonObject) override;

protected:

    bool init() override;

    void onCollision(const Collision &collision) override;

private:

    static glm::vec3 deserializeSize(const FileLoaderType::JsonObject &jsonObject);
    static glm::vec3 deserializeCenter(const FileLoaderType::JsonObject &jsonObject);

    static const int SerializedVersion = 1;

    glm::vec3 mSize{};		    // The size of the box, measured in the object's local space.

};
