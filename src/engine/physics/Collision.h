#pragma once

#include <glm/vec3.hpp>

#include <vector>

struct IEntity;
class Collider;

struct ContactPoint
{
    // The point of contact.
    glm::vec3 point {};
    
    // Normal of the contact point.
    glm::vec3 normal {};
    
    // The first collider in contact at the point.
    Collider * thisCollider {};
    
    // The other collider in contact at the point.
    Collider * otherCollider {};
};

struct Collision
{
    // The GameObject whose collider you are colliding with. (Read Only).
    IEntity * otherEntity {};
    std::vector<ContactPoint> contactPoints;
};
