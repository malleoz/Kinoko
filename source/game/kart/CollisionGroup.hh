#pragma once

#include "game/kart/KartParam.hh"

#include <egg/math/Matrix.hh>

/// @brief Pertains to kart-related functionality.
namespace Kinoko::Kart {

/// @brief Information about the current collision and its properties.
/// @see KCollisionTypes.hh
struct CollisionData {
    void reset();

    EGG::Vector3f tangentOff;      ///< The offset to move the kart by to resolve the collision
    EGG::Vector3f floorNrm;        ///< The normal of the colliding floor KCL
    EGG::Vector3f wallNrm;         ///< The normal of the colliding wall KCL
    EGG::Vector3f noBounceWallNrm; ///< The normal of the colliding soft wall KCL
    EGG::Vector3f vel;             ///< Velocity at the time of collision
    EGG::Vector3f relPos;   ///< Position of the colliding hitbox relative to the kart's position
    EGG::Vector3f movement; ///< Displacement from a wall collision applied to the kart's body
    EGG::Vector3f roadVelocity; ///< Velocity of moving road KCL (e.g. from @ref Field::ObjectBelt)
    f32 speedFactor;            ///< Speed multiplier based on the floor KCL type (e.g. offroad)
    f32 rotFactor;              ///< Rotation multiplier based on the floor KCL type (e.g. offroad)
    Field::KCLTypeMask closestFloorFlags; ///< The colliding floor KCL flag's @ref KColType.
    u32 closestFloorSettings;             ///< The colliding floor KCL flag's "variant"
    Field::KCLTypeMask closestWallFlags;  ///< The colliding wall KCL flag's @ref KColType.
    u32 closestWallSettings;              ///< The colliding wall KCL flag's "variant"
    s32 intensity;                        ///< The KCL flag's "wheel depth"
    f32 colPerpendicularity; ///< Dot product to measure how much two colliding wall normals diverge

    bool bFloor;         ///< Set if colliding with KCL which satisfies #KCL_TYPE_FLOOR
    bool bWall;          ///< Set if colliding with KCL which satisfies #KCL_TYPE_WALL
    bool bInvisibleWall; ///< Set if colliding with KCL which satisfies #KCL_TYPE_ANY_INVISIBLE_WALL
    bool bTrickable;     ///< Set if the colliding KCL is trickable
    bool bMovingWaterMomentum;     ///< Player will maintain speed for a bit after leaving KCL
    bool bWall3;                   ///< Set if colliding with #COL_TYPE_WALL_2
    bool bInvisibleWallOnly;       ///< Set if only colliding with COL_TYPE_INVISIBLE_WALL
    bool bMovingWaterDecaySpeed;   ///< Player speed will drop if not in mushroom
    bool bSoftWall;                ///< Set if the colliding KCL is a soft wall
    bool bMovingWaterStickyRoad;   ///< KC pipe vertical water section
    bool bMovingWaterDisableAccel; ///< KC last turn prevents mini-turbo acceleration
    bool bHasRoadVel; ///< Set if colliding with moving road KCL (e.g. from @ref Field::ObjectBelt)
    bool bWallAtLeftCloser;  ///< Set if the wall collision on the left side of the kart is closer
    bool bWallAtRightCloser; ///< Set if the wall collision on the right side of the kart is closer
    bool bMovingWaterVertical; ///< KC last turn vertical water
};

/// @brief Represents a hitbox for the kart body or a wheel.
/// @details A hitbox's position information is directly used in the KCL collision check functions.
/// Every frame, updates the state of the hitbox's position relative to the kart's position and
/// orientation and computes the effective world position.
class Hitbox {
public:
    Hitbox();
    ~Hitbox();

    void calc(f32 totalScale, f32 sinkDepth, const EGG::Vector3f &scale, const EGG::Quatf &rot,
            const EGG::Vector3f &pos);

    /// @addr{0x805B7F84}
    /// @brief Resets the hitbox's position information to the origin
    void reset() {
        m_worldPos.setZero();
        m_lastPos.setZero();
        m_relPos.setZero();
    }

    /// @beginSetters
    void setRadius(f32 radius) {
        m_radius = radius;
    }

    /// @brief Sets the @ref BSP::Hitbox pointer and whether this Hitbox is responsible for freeing
    /// it on destruction
    /// @param hitbox The BSP::Hitbox pointer to set
    /// @param owns Whether this Hitbox is responsible for freeing the pointer on destruction
    void setBspHitbox(const BSP::Hitbox *hitbox, bool owns = false) {
        m_ownsBSP = owns;
        m_bspHitbox = hitbox;
    }

    void setWorldPos(const EGG::Vector3f &pos) {
        m_worldPos = pos;
    }

    void setLastPos(const EGG::Vector3f &pos) {
        m_lastPos = pos;
    }

    void setLastPos(const EGG::Vector3f &scale, const EGG::Matrix34f &pose);
    /// @endSetters

    /// @beginGetters
    [[nodiscard]] const BSP::Hitbox *bspHitbox() const {
        return m_bspHitbox;
    }

    [[nodiscard]] const EGG::Vector3f &worldPos() const {
        return m_worldPos;
    }

    [[nodiscard]] const EGG::Vector3f &lastPos() const {
        return m_lastPos;
    }

    [[nodiscard]] const EGG::Vector3f &relPos() const {
        return m_relPos;
    }

    [[nodiscard]] f32 radius() const {
        return m_radius;
    }
    /// @endGetters

private:
    const BSP::Hitbox *m_bspHitbox; ///< Pointer to the @ref BSP::Hitbox that this Hitbox represents
    f32 m_radius;                   ///< The radius of the hitbox's sphere
    EGG::Vector3f m_worldPos;       ///< The world position of the hitbox's sphere
    EGG::Vector3f m_lastPos;        ///< The world position in the previous frame
    EGG::Vector3f m_relPos;         ///< The position of the hitbox relative to the kart's position
    bool m_ownsBSP; ///< Whether this Hitbox's destructor should free the @ref BSP::Hitbox pointer
};

/// @brief Houses hitbox and collision info for a portion of the kart (body or wheel).
/// @details The game uses one group to represent the hitboxes of the kart's body, and each tire on
/// the kart has its own group containing a single hitbox. Various @ref Kart classes will modify the
/// collision data member in this class to reflect collisions that occur based off the hitboxes in
/// this group.
class CollisionGroup {
public:
    CollisionGroup();
    ~CollisionGroup();

    [[nodiscard]] f32 initHitboxes(const std::array<BSP::Hitbox, 16> &hitboxes);
    [[nodiscard]] f32 computeCollisionLimits();
    void createSingleHitbox(f32 radius, const EGG::Vector3f &relPos);

    /// @addr{0x805B8330}
    void reset() {
        m_collisionData.reset();

        for (auto &hitbox : m_hitboxes) {
            hitbox.reset();
            hitbox.setRadius(hitbox.bspHitbox()->radius * m_hitboxScale);
        }
    }

    void resetCollision() {
        m_collisionData.reset();
    }

    /// @addr{0x805B83D8}
    void setHitboxScale(f32 scale) {
        m_hitboxScale = scale;

        for (auto &hitbox : m_hitboxes) {
            hitbox.setRadius(hitbox.bspHitbox()->radius * m_hitboxScale);
        }
    }

    /// @beginGetters
    [[nodiscard]] f32 boundingRadius() const {
        return m_boundingRadius;
    }

    [[nodiscard]] Hitbox &hitbox(u16 hitboxIdx) {
        return m_hitboxes[hitboxIdx];
    }

    [[nodiscard]] u16 hitboxCount() const {
        return m_hitboxes.size();
    }

    [[nodiscard]] CollisionData &collisionData() {
        return m_collisionData;
    }

    [[nodiscard]] const CollisionData &collisionData() const {
        return m_collisionData;
    }
    /// @endGetters

private:
    f32 m_boundingRadius;
    CollisionData m_collisionData;
    owning_span<Hitbox> m_hitboxes;
    f32 m_hitboxScale;
};

} // namespace Kinoko::Kart
