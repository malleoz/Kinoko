#pragma once

#include "game/field/ObjectCollisionConvexHull.hh"

namespace Kinoko::Field {

/// @brief Defines the convex hull of a box-shaped collision object
/// @details The box is defined by its dimensions and center point, and the convex hull is
/// constructed from the eight vertices of the box.
class ObjectCollisionBox : public ObjectCollisionConvexHull {
public:
    ObjectCollisionBox(f32 x, f32 y, f32 z, const EGG::Vector3f &center);
    ~ObjectCollisionBox() override;

    void transform(const EGG::Matrix34f &mat, const EGG::Vector3f &scale) override;
    void transform(const EGG::Matrix34f &mat, const EGG::Vector3f &scale,
            const EGG::Vector3f &speed) override;

private:
    EGG::Vector3f m_dimensions; ///< Length, width, and height of the box
    EGG::Vector3f m_center;     ///< Center point of the box in local space
    EGG::Vector3f m_scale;      ///< Scale factor applied to the box dimensions
};

} // namespace Kinoko::Field
