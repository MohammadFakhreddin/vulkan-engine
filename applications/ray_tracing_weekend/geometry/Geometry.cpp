#include "Geometry.hpp"

Geometry::Geometry(std::shared_ptr<Material> material_) 
    : material(std::move(material_))
{}
