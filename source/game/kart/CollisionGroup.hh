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
    /// @addr{0x805B7F48}
    /// @brief Constructor
    /// @details Initializes @ref m_bspHitbox to `nullptr` and @ref m_ownsBSP to `false`.
    Hitbox() : m_bspHitbox(nullptr), m_ownsBSP(false) {}

    /// @addr{0x805B8480}
    /// @brief Destructor that frees the @ref BSP::Hitbox pointer if this instance owns it
    ~Hitbox() {
        if (m_ownsBSP) {
            EGG::egg_delete(m_bspHitbox);
        }
    }

    void calc(f32 totalScale, f32 sinkDepth, const EGG::Vector3f &scale, const EGG::Quatf &rot,
            const EGG::Vector3f &pos);

    /// @addr{0x805B7F84}
    /// @brief Resets the hitbox's position information to the origin
    /// @details Sets @ref m_worldPos, @ref m_lastPos, and @ref m_relPos to zero vectors.
    void reset() {
        m_worldPos.setZero();
        m_lastPos.setZero();
        m_relPos.setZero();
    }

    /// @beginSetters

    /// @brief Sets the radius of the hitbox's sphere
    /// @param radius The radius to set
    void setRadius(f32 radius) {
        m_radius = radius;
    }

    /// @brief Sets the @ref BSP::Hitbox pointer and whether this Hitbox is responsible for freeing
    /// it on destruction
    /// @param hitbox The BSP::Hitbox pointer to set
    /// @param owns Whether this Hitbox is responsible for freeing the pointer on destruction
    /// @details Sets @ref m_bspHitbox to the provided `hitbox` and @ref m_ownsBSP to the provided
    /// boolean.
    void setBspHitbox(const BSP::Hitbox *hitbox, bool owns = false) {
        m_ownsBSP = owns;
        m_bspHitbox = hitbox;
    }

    /// @brief Sets the world position of the hitbox
    /// @param pos The world position to set
    void setWorldPos(const EGG::Vector3f &pos) {
        m_worldPos = pos;
    }

    /// @brief Sets the last world position of the hitbox
    /// @param pos The last world position to set
    void setLastPos(const EGG::Vector3f &pos) {
        m_lastPos = pos;
    }

    /// @endSetters

    void setLastPos(const EGG::Vector3f &scale, const EGG::Matrix34f &pose);

    /// @beginGetters

    /// @brief Retrieves the BSP::Hitbox pointer associated with this Hitbox
    /// @return The BSP::Hitbox pointer, @ref m_bspHitbox
    [[nodiscard]] const BSP::Hitbox *bspHitbox() const {
        return m_bspHitbox;
    }

    /// @brief Retrieves the world position of the hitbox
    /// @return The world position, @ref m_worldPos
    [[nodiscard]] const EGG::Vector3f &worldPos() const {
        return m_worldPos;
    }

    /// @brief Retrieves the last world position of the hitbox
    /// @return The last world position, @ref m_lastPos
    [[nodiscard]] const EGG::Vector3f &lastPos() const {
        return m_lastPos;
    }

    /// @brief Retrieves the position of the hitbox relative to the kart's position
    /// @return The relative position, @ref m_relPos
    [[nodiscard]] const EGG::Vector3f &relPos() const {
        return m_relPos;
    }

    /// @brief Retrieves the radius of the hitbox's sphere
    /// @return The radius, @ref m_radius
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
    /// @addr{0x805B82BC}
    /// @brief Constructor
    /// @details Initializes @ref m_hitboxScale to `1.0f` and resets @ref m_collisionData.
    CollisionGroup() : m_hitboxScale(1.0f) {
        m_collisionData.reset();
    }

    /// @brief Default destructor
    ~CollisionGroup() = default;

    [[nodiscard]] f32 initHitboxes(const std::array<BSP::Hitbox, 16> &hitboxes);
    void createSingleHitbox(f32 radius, const EGG::Vector3f &relPos);

    /// @addr{0x805B8330}
    /// @brief Resets the collision data and hitboxes to their default values.
    /// @details Calls @ref CollisionData::reset() on @ref m_collisionData. For each hitbox, calls
    /// @ref Hitbox::reset() and updates its radius based on @ref m_hitboxScale times the BSP
    /// hitbox's radius.
    void reset() {
        m_collisionData.reset();

        for (auto &hitbox : m_hitboxes) {
            hitbox.reset();
            hitbox.setRadius(hitbox.bspHitbox()->radius * m_hitboxScale);
        }
    }

    /// @brief Resets the collision data to its default values.
    /// @details Calls @ref CollisionData::reset() on @ref m_collisionData.
    void resetCollision() {
        m_collisionData.reset();
    }

    /// @addr{0x805B83D8}
    /// @brief Sets the scale factor for the hitboxes
    /// @param scale The scale factor to apply to all hitboxes in this group
    /// @details Updates the radius of each hitbox based on the new scale factor times the BSP
    /// hitbox's radius.
    void setHitboxScale(f32 scale) {
        m_hitboxScale = scale;

        for (auto &hitbox : m_hitboxes) {
            hitbox.setRadius(hitbox.bspHitbox()->radius * m_hitboxScale);
        }
    }

    /// @beginGetters

    /// @brief Retrieves the bounding radius of the collision group
    /// @return The bounding radius, @ref m_boundingRadius
    [[nodiscard]] f32 boundingRadius() const {
        return m_boundingRadius;
    }

    /// @brief Retrieves a specific hitbox by its index
    /// @param hitboxIdx The index of the hitbox to retrieve
    /// @return The hitbox at the specified index from @ref m_hitboxes
    [[nodiscard]] Hitbox &hitbox(u16 hitboxIdx) {
        return m_hitboxes[hitboxIdx];
    }

    /// @brief Retrieves the number of hitboxes in the collision group
    /// @return The number of hitboxes based on the size of @ref m_hitboxes
    [[nodiscard]] u16 hitboxCount() const {
        return m_hitboxes.size();
    }

    /// @brief Retrieves the collision data for the collision group
    /// @return Ref to the collision data, @ref m_collisionData
    [[nodiscard]] CollisionData &collisionData() {
        return m_collisionData;
    }

    /// @brief Retrieves the collision data for the collision group
    /// @return Const ref to the collision data, @ref m_collisionData
    [[nodiscard]] const CollisionData &collisionData() const {
        return m_collisionData;
    }

    /// @endGetters

private:
    [[nodiscard]] f32 computeCollisionLimits();

    f32 m_boundingRadius; ///< Radius of the sphere that encompasses all hitboxes in this group
    CollisionData m_collisionData;  ///< Stores the collision data for the group of hitboxes
    owning_span<Hitbox> m_hitboxes; ///< Stores the hitboxes for the collision group
    f32 m_hitboxScale;              ///< Scale factor applied to all hitboxes in this group
};

} // namespace Kinoko::Kart
