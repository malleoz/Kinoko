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

} // namespace Kinoko::Kart
