#pragma once

#include <egg/math/BoundBox.hh>
#include <egg/math/Matrix.hh>

namespace Kinoko::Field {

/// @brief Stores partial information pertaining to a collision
struct CollisionInfoPartial {
    EGG::BoundBox3f bbox;     ///< Bounding box of "push out" vectors
    EGG::Vector3f tangentOff; ///< The net "push out" vector

    void reset();
    void updateBBox(const EGG::Vector3f &offset);
    void transformInfo(const CollisionInfoPartial &rhs, const EGG::Matrix34f &mtx);
};

/// @brief Stores information pertaining to a collision
/// @details Tracks the distance for the closest floor, wall, and moving floor collision during a
/// collision query so that it can update its internal state to reflect the closest collision for
/// each category.
struct CollisionInfo {
    EGG::BoundBox3f bbox;       ///< Bounding box of "push out" vectors
    EGG::Vector3f tangentOff;   ///< The net "push out" vector
    EGG::Vector3f floorNrm;     ///< Colliding floor's up vector
    EGG::Vector3f wallNrm;      ///< Colliding wall face's normal vector
    EGG::Vector3f roadVelocity; ///< Optional velocity induced by the colliding object
    f32 floorDist;              ///< Distance from the colliding floor tri
    f32 wallDist;               ///< Distance from the colliding wall tri
    f32 movingFloorDist;        ///< Distance from the colliding moving floor tri
    f32 perpendicularity;       ///< Measures how much two colliding wall normals diverge

    void updateBBox(const EGG::Vector3f &offset);
    void updateFloor(f32 dist, const EGG::Vector3f &fnrm);
    void updateWall(f32 dist, const EGG::Vector3f &fnrm);
    void reset();
    void update(f32 now_dist, const EGG::Vector3f &offset, const EGG::Vector3f &fnrm,
            u32 kclAttributeTypeBit);
    void transformInfo(const CollisionInfo &rhs, const EGG::Matrix34f &mtx, const EGG::Vector3f &v);
};

} // namespace Kinoko::Field
