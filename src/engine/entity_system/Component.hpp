#pragma once

#include "IComponent.hpp"

namespace MFA {

class Entity;

class Component final : public IComponent {
    friend Entity;
private:
    Entity * mEntity = nullptr;
};

}