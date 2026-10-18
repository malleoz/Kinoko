#include "CollisionGroup.hh"

namespace Kinoko::Kart {

/// @addr{0x805B821C}
/// @brief Resets the collision data to default values to prep for a new collision check
/// @details Resets all vectors to zero and all booleans to `false`.
void CollisionData::reset() {
    tangentOff.setZero();
    floorNrm.setZero();
    wallNrm.setZero();
    vel.setZero();
    relPos.setZero();
    movement.setZero();
    roadVelocity.setZero();
    speedFactor = 1.0f;
    rotFactor = 0.0f;
    closestFloorFlags = 0;
    closestFloorSettings = 0xffffffff;
    closestWallFlags = 0;
    closestFloorSettings = 0xffffffff;
    intensity = 0.0f;
    colPerpendicularity = 0.0f;

    bFloor = false;
    bWall = false;
    bInvisibleWall = false;
    bTrickable = false;
    bMovingWaterMomentum = false;
    bWall3 = false;
    bInvisibleWallOnly = false;
    bMovingWaterDecaySpeed = false;
    bSoftWall = false;
    bMovingWaterStickyRoad = false;
    bMovingWaterDisableAccel = false;
    bHasRoadVel = false;
    bWallAtLeftCloser = false;
    bWallAtRightCloser = false;
    bMovingWaterVertical = false;
}

/// @addr{0x805B7F48}
/// @brief Constructor
/// @details Initializes @ref m_bspHitbox to `nullptr` and @ref m_ownsBSP to `false`.
Hitbox::Hitbox() : m_bspHitbox(nullptr), m_ownsBSP(false) {}

/// @addr{0x805B8480}
/// @brief Destructor that frees the @ref BSP::Hitbox pointer if this instance owns it
Hitbox::~Hitbox() {
    if (m_ownsBSP) {
        EGG::egg_delete(m_bspHitbox);
    }
}

/// @addr{0x805B7FBC}
/// @brief Calculates the position of a given hitbox, both relative to the player and world
/// @param totalScale The total scale factor to apply to the hitbox
/// @param sinkDepth The depth to sink the hitbox
/// @param scale The scale vector for the hitbox
/// @param rot The rotation quaternion for the hitbox
/// @param pos The world position of the hitbox
/// @details Updates @ref m_relPos and @ref m_worldPos based on the provided scale, rotation, and
/// world position. `totalScale` is used to adjust the hitbox's vertical position upwards even when
/// `scale` is smaller, such as when the player was crushed by a stomper. Also applies `sinkDepth`
/// to the hitbox's vertical position to account for instances when the vehicle can sink into the
/// road KCL.
void Hitbox::calc(f32 totalScale, f32 sinkDepth, const EGG::Vector3f &scale, const EGG::Quatf &rot,
        const EGG::Vector3f &pos) {
    f32 scaledHeightOffset = 0.0f;
    if (scale.y < totalScale) {
        scaledHeightOffset = (totalScale - scale.y) * m_bspHitbox->radius;
    }

    EGG::Vector3f scaledPos = m_bspHitbox->position * scale;
    scaledPos.y = (m_bspHitbox->position.y + sinkDepth) * scale.y + scaledHeightOffset;

    m_relPos = rot.rotateVector(scaledPos);
    m_worldPos = m_relPos + pos;
}

/// @addr{0x805B7F84}
/// @brief Resets the hitbox's position information to the origin
/// @details Sets @ref m_worldPos, @ref m_lastPos, and @ref m_relPos to zero vectors.
void Hitbox::reset() {
    m_worldPos.setZero();
    m_lastPos.setZero();
    m_relPos.setZero();
}

/// @brief Sets the radius of the hitbox's sphere
/// @param radius The radius to set
void Hitbox::setRadius(f32 radius) {
    m_radius = radius;
}

/// @brief Sets the @ref BSP::Hitbox pointer and whether this Hitbox is responsible for freeing
/// it on destruction
/// @param hitbox The BSP::Hitbox pointer to set
/// @param owns Whether this Hitbox is responsible for freeing the pointer on destruction
/// @details Sets @ref m_bspHitbox to the provided `hitbox` and @ref m_ownsBSP to the provided
/// boolean.
void Hitbox::setBspHitbox(const BSP::Hitbox *hitbox, bool owns /*= false*/) {
    m_ownsBSP = owns;
    m_bspHitbox = hitbox;
}

/// @brief Sets the world position of the hitbox
/// @param pos The world position to set
void Hitbox::setWorldPos(const EGG::Vector3f &pos) {
    m_worldPos = pos;
}

/// @brief Sets the last world position of the hitbox
/// @param pos The last world position to set
void Hitbox::setLastPos(const EGG::Vector3f &pos) {
    m_lastPos = pos;
}

/// @addr{0x805B80A8}
/// @brief Saves the current position of the hitbox so it can be referenced after updating the
/// Hitbox's next position
/// @param scale The scale vector for the hitbox
/// @param pose The transformation matrix for the hitbox
/// @details Updates @ref m_lastPos based on the provided scale and transformation matrix. Offsets
/// the hitbox's vertical position upwards if the Y-scale is different from the Z-scale and less
/// than 1.0f.
void Hitbox::setLastPos(const EGG::Vector3f &scale, const EGG::Matrix34f &pose) {
    f32 yScaleFactor = scale.y;
    EGG::Vector3f scaledPos = m_bspHitbox->position;
    scaledPos.x *= scale.x;
    scaledPos.z *= scale.z;

    if (scale.y != scale.z && scale.y < 1.0f) {
        scaledPos.y += (1.0f - scale.y) * m_radius;
        yScaleFactor = scale.z;
    }

    scaledPos.y *= yScaleFactor;
    m_lastPos = pose.ps_multVector(scaledPos);
}

/// @brief Retrieves the BSP::Hitbox pointer associated with this Hitbox
/// @return The BSP::Hitbox pointer, @ref m_bspHitbox
const BSP::Hitbox *Hitbox::bspHitbox() const {
    return m_bspHitbox;
}

/// @brief Retrieves the world position of the hitbox
/// @return The world position, @ref m_worldPos
const EGG::Vector3f &Hitbox::worldPos() const {
    return m_worldPos;
}

/// @brief Retrieves the last world position of the hitbox
/// @return The last world position, @ref m_lastPos
const EGG::Vector3f &Hitbox::lastPos() const {
    return m_lastPos;
}

/// @brief Retrieves the position of the hitbox relative to the kart's position
/// @return The relative position, @ref m_relPos
const EGG::Vector3f &Hitbox::relPos() const {
    return m_relPos;
}

/// @brief Retrieves the radius of the hitbox's sphere
/// @return The radius, @ref m_radius
f32 Hitbox::radius() const {
    return m_radius;
}

/// @addr{0x805B82BC}
/// @brief Constructor
/// @details Initializes @ref m_hitboxScale to `1.0f` and resets @ref m_collisionData.
CollisionGroup::CollisionGroup() : m_hitboxScale(1.0f) {
    m_collisionData.reset();
}

/// @brief Default destructor
CollisionGroup::~CollisionGroup() = default;

/// @addr{0x805B84C0}
/// @brief Initializes the hitbox array based on the provided @ref BSP::Hitbox array
/// @details The BSP always contains 16 hitboxes, but only some of them are valid/enabled.
/// The game iterates the @ref BSP::Hitbox array to see how many are enabled, allocates a Hitbox
/// array of that size, sets all the enabled BSP hitboxes, and computes the bounding radius of the
/// kart based on the enabled hitboxes.
/// @param hitboxes The hitboxes from `KartParam.bin`
/// @return Half of the largest Z-axis extent among the enabled hitboxes
/// @details Counts the number of BSP hitboxes that are actually enabled and sizes @ref m_hitboxes
/// accordingly. For each enabled hitbox, a corresponding @ref Hitbox instance is created and
/// initialized with the provided @ref BSP::Hitbox. Finally, returns the bounding radius of the
/// hitboxes.
f32 CollisionGroup::initHitboxes(const std::array<BSP::Hitbox, 16> &hitboxes) {
    u16 bspHitboxCount = 0;

    for (const auto &hitbox : hitboxes) {
        if (parse<u16>(hitbox.enable)) {
            ++bspHitboxCount;
        }
    }

    m_hitboxes = owning_span<Hitbox>(bspHitboxCount);
    u16 hitboxIdx = 0;

    for (const auto &bspHitbox : hitboxes) {
        if (parse<u16>(bspHitbox.enable)) {
            m_hitboxes[hitboxIdx++].setBspHitbox(&bspHitbox);
        }
    }

    return computeCollisionLimits();
}

/// @addr{0x805B875C}
/// @brief Creates a hitbox to represent a tire
/// @param radius The radius of the tire
/// @param relPos The position of the tire relative to the kart's position
/// @details Creates a single hitbox to represent the tire. Creates a new @ref BSP::Hitbox instance
/// and associates it with the @ref Hitbox. Sets the BSP hitbox's position and radius based on the
/// provided parameters, the hitbox's radius based on the provided `radius`, and sets @ref
/// m_boundingRadius to the provided `radius`.
void CollisionGroup::createSingleHitbox(f32 radius, const EGG::Vector3f &relPos) {
    m_hitboxes = owning_span<Hitbox>(1);

    for (auto &hitbox : m_hitboxes) {
        hitbox.reset();
        BSP::Hitbox *bspHitbox = EGG::egg_new<BSP::Hitbox>();
        hitbox.setBspHitbox(bspHitbox, true);
        bspHitbox->position = relPos;
        bspHitbox->radius = radius;
        hitbox.setRadius(radius);
    }
    m_boundingRadius = radius;
}

/// @addr{0x805B8330}
/// @brief Resets the collision data and hitboxes to their default values.
/// @details Calls @ref CollisionData::reset() on @ref m_collisionData. For each hitbox, calls
/// @ref Hitbox::reset() and updates its radius based on @ref m_hitboxScale times the BSP
/// hitbox's radius.
void CollisionGroup::reset() {
    m_collisionData.reset();

    for (auto &hitbox : m_hitboxes) {
        hitbox.reset();
        hitbox.setRadius(hitbox.bspHitbox()->radius * m_hitboxScale);
    }
}

/// @brief Resets the collision data to its default values.
/// @details Calls @ref CollisionData::reset() on @ref m_collisionData.
void CollisionGroup::resetCollision() {
    m_collisionData.reset();
}

/// @addr{0x805B83D8}
/// @brief Sets the scale factor for the hitboxes
/// @param scale The scale factor to apply to all hitboxes in this group
/// @details Updates the radius of each hitbox based on the new scale factor times the BSP
/// hitbox's radius.
void CollisionGroup::setHitboxScale(f32 scale) {
    m_hitboxScale = scale;

    for (auto &hitbox : m_hitboxes) {
        hitbox.setRadius(hitbox.bspHitbox()->radius * m_hitboxScale);
    }
}

/// @brief Retrieves the bounding radius of the collision group
/// @return The bounding radius, @ref m_boundingRadius
[[nodiscard]] f32 CollisionGroup::boundingRadius() const {
    return m_boundingRadius;
}

/// @brief Retrieves a specific hitbox by its index
/// @param hitboxIdx The index of the hitbox to retrieve
/// @return The hitbox at the specified index from @ref m_hitboxes
[[nodiscard]] Hitbox &CollisionGroup::hitbox(u16 hitboxIdx) {
    return m_hitboxes[hitboxIdx];
}

/// @brief Retrieves the number of hitboxes in the collision group
/// @return The number of hitboxes based on the size of @ref m_hitboxes
[[nodiscard]] u16 CollisionGroup::hitboxCount() const {
    return m_hitboxes.size();
}

/// @brief Retrieves the collision data for the collision group
/// @return Ref to the collision data, @ref m_collisionData
[[nodiscard]] CollisionData &CollisionGroup::collisionData() {
    return m_collisionData;
}

/// @brief Retrieves the collision data for the collision group
/// @return Const ref to the collision data, @ref m_collisionData
[[nodiscard]] const CollisionData &CollisionGroup::collisionData() const {
    return m_collisionData;
}

/// @addr{0x805B883C}
/// @brief Sets the bounding radius
/// @return Half of the largest Z-axis extent among the enabled hitboxes
/// @details This effectively computes the required radius of a bounding sphere that encompasses all
/// enabled hitboxes.
f32 CollisionGroup::computeCollisionLimits() {
    EGG::Vector3f max = EGG::Vector3f::zero;

    for (const auto &hitbox : m_hitboxes) {
        const BSP::Hitbox *bspHitbox = hitbox.bspHitbox();

        if (bspHitbox->enable == 0) {
            continue;
        }

        max = max.maximize(bspHitbox->position.abs() + bspHitbox->radius);
    }

    // Get largest component of the vector
    f32 maxComponent = max.z;

    if (max.x <= max.y) {
        if (max.z < max.y) {
            maxComponent = max.y;
        }
    } else if (max.z < max.x) {
        maxComponent = max.x;
    }

    m_boundingRadius = maxComponent;

    return max.z * 0.5f;
}

} // namespace Kinoko::Kart
