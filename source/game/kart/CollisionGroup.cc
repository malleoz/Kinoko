#include "CollisionGroup.hh"

namespace Kinoko::Kart {

/// @addr{0x805B821C}
/// @brief Resets the collision data to default values to prep for a new collision check
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
}

/// @addr{0x805B7F48}
Hitbox::Hitbox() : m_bspHitbox(nullptr), m_ownsBSP(false) {}

/// @addr{0x805B8480}
/// @brief Frees the @ref BSP::Hitbox pointer if this instance owns it
Hitbox::~Hitbox() {
    if (m_ownsBSP) {
        EGG::egg_delete(m_bspHitbox);
    }
}

/// @addr{0x805B7FBC}
/// @brief Calculates the position of a given hitbox, both relative to the player and world
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

/// @addr{0x805B82BC}
CollisionGroup::CollisionGroup() : m_hitboxScale(1.0f) {
    m_collisionData.reset();
}

CollisionGroup::~CollisionGroup() = default;

/// @addr{0x805B84C0}
/// @brief Initializes the hitbox array based on the provided @ref BSP::Hitbox array
/// @details The BSP always contains 16 hitboxes, but only some of them are valid/enabled.
/// The game iterates the @ref BSP::Hitbox array to see how many are enabled, allocates a Hitbox
/// array of that size, sets all the enabled BSP hitboxes, and computes the bounding radius of the
/// kart based on the enabled hitboxes.
/// @param hitboxes The hitboxes from @p KartParam.bin
/// @return Half of the largest Z-axis extent among the enabled hitboxes
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
