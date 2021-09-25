#pragma once

#include "IEntity.hpp"

#include "Component.hpp"
#include "engine/BedrockAssert.hpp"

#include <unordered_map>
#include <memory>

// TODO This class will need optimization in future

namespace MFA {

class Entity final : public IEntity {
public:

    explicit Entity(Entity * parent = nullptr)
        : mParent(parent)
    {}

    template<typename ComponentClass>
    ComponentClass * AddComponent() {
        MFA_ASSERT(GetComponent<ComponentClass>() == nullptr);
        auto component = std::make_unique<ComponentClass>();
        return static_cast<ComponentClass *>(insertComponent(component));
    }

    void AddComponent(std::unique_ptr<Component> && component) {
        auto exists = [this](Component * targetComponent)->bool{
            MFA_ASSERT(targetComponent != nullptr);
            for (auto const & componentRef : mComponentsRef) {
                if (typeid(*componentRef.Ptr) == typeid(*targetComponent)) {
                    return true;
                }
            }
            return false;
        };
        MFA_ASSERT(exists(component.get()) == false);
        insertComponent(std::move(component));
    }

    template<typename ComponentClass>
    [[nodiscard]]
    Component * GetComponent() {
        for (auto & componentRef : mComponentsRef) {
            auto * targetComponent = dynamic_cast<ComponentClass *>(componentRef.Ptr);
            if (targetComponent != nullptr) {
                return targetComponent;
            }
        }
        return nullptr;
    }

    [[nodiscard]]
    Entity * GetParent() const noexcept {
        return mParent;
    }

private:

    Component * insertComponent(std::unique_ptr<Component> && component) {
        mComponentsRef.emplace_back(ComponentRef {.Ptr = std::move(component)});
        auto * ptr = mComponentsRef.back().Ptr.get();
        ptr->mEntity = this;
        return ptr;
    }

    Entity * mParent = nullptr;

    struct ComponentRef {
        std::unique_ptr<Component> Ptr = nullptr;
    };
    std::vector<ComponentRef> mComponentsRef {};
};

}