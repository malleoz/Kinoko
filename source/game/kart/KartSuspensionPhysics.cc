#include "KartSuspensionPhysics.hh"

#include "game/kart/CollisionGroup.hh"
#include "game/kart/KartBody.hh"
#include "game/kart/KartCollide.hh"
#include "game/kart/KartDynamics.hh"
#include "game/kart/KartMove.hh"
#include "game/kart/KartSub.hh"
#include "game/kart/KartTire.hh"

namespace Kinoko::Kart {

/// @addr{0x8059940C}
/// @brief Constructor
/// @param wheelIdx The index of the wheel
/// @param bspWheelIdx The index of the wheel in the @ref BSP::wheels array
/// @details Sets @ref m_wheelIdx and @ref m_bspWheelIdx to the provided `wheelIdx` and
/// `bspWheelIdx`. Initializes @ref m_bspWheel to `nullptr`.
WheelPhysics::WheelPhysics(u16 wheelIdx, u16 bspWheelIdx)
    : m_wheelIdx(wheelIdx),
      m_bspWheelIdx(bspWheelIdx),
      m_bspWheel(nullptr) {}

/// @addr{0x8059A9C4}
/// @brief Virtual destructor that destroys the underlying @ref CollisionGroup subsystem for
/// this wheel
WheelPhysics::~WheelPhysics() {
    EGG::egg_delete(m_hitboxGroup);
}

/// @addr{0x80599470}
/// @brief Creates and initializes the wheel's @ref m_hitboxGroup and creates a single hitbox
/// for the tire
/// @details Calls @ref CollisionGroup::createSingleHitbox() to create a hitbox with radius
/// `10.0f` for the tire.
void WheelPhysics::init() {
    constexpr f32 RADIUS = 10.0f;

    m_hitboxGroup = EGG::egg_new<CollisionGroup>();
    m_hitboxGroup->createSingleHitbox(RADIUS, EGG::Vector3f::zero);
}

/// @addr{0x805994D4}
/// @brief Fetches the @ref BSP::Wheel pointer corresponding to this wheel
/// @details Sets @ref m_bspWheel to the wheel in the kart's @ref BSP::wheels array at index
/// @ref m_bspWheelIdx.
void WheelPhysics::initBsp() {
    m_bspWheel = &bsp().wheels[m_bspWheelIdx];
}

/// @addr{0x80599508}
/// @brief Resets the wheel physics to its initial state
/// @details Resets all position and velocity-related members to zero. Resets @ref m_suspTravel,
/// @ref m_effectiveRadius, @ref m_targetEffectiveRadius, and @ref m_hasSuspTravel to zero. If @ref
/// m_bspWheel is not `nullptr`, then sets @ref m_suspTravel and @ref m_effectiveRadius based on
/// properties of @ref m_bspWheel.
void WheelPhysics::reset() {
    m_pos.setZero();
    m_lastPos.setZero();
    m_lastTopDiff.setZero();
    m_suspTravel = 0.0f;
    m_colVel.setZero();
    m_relVel.setZero();
    m_wheelEdgePos.setZero();
    m_effectiveRadius = 0.0f;
    m_targetEffectiveRadius = 0.0f;
    m_hasSuspTravel = 0.0f;
    m_topmostPos.setZero();

    if (m_bspWheel) {
        m_suspTravel = m_bspWheel->maxTravel;
        m_effectiveRadius = m_bspWheel->wheelRadius;
    }
}

/// @addr{0x80599AD0}
/// @brief Computes the wheel's relative position and suspension travel based on the given bottom
/// direction and vehicle movement
/// @param bottom The "down" direction along the suspension axis
/// @param vehicleMovement The movement of the vehicle to account for in the wheel's position
/// @details Sets @ref m_suspTravel to the projection of the wheel's displacement along the
/// suspension axis, clamped between 0 and the maximum travel distance defined by @ref
/// BSP::Wheel::maxTravel scaled by @ref KartSub::m_suspScale. Updates the wheel's position and
/// relative velocity accordingly. @ref m_relVel is additionally adjusted to account for the kart's
/// internal velocity, moving object velocity, moving road velocity, and collision displacement.
/// Adds the resulting @ref m_relVel to the @ref CollisionData of @ref m_hitboxGroup. Updates @ref
/// m_lastPos and @ref m_lastTopDiff to reflect the resulting this current frame's wheel position
/// and suspension travel.
void WheelPhysics::realign(const EGG::Vector3f &bottom, const EGG::Vector3f &vehicleMovement) {
    const EGG::Vector3f topmostPos = m_topmostPos + vehicleMovement;
    f32 maxTravel = m_bspWheel->maxTravel * sub()->suspScale();
    m_suspTravel = std::clamp(bottom.dot(m_pos - topmostPos), 0.0f, maxTravel);
    m_pos = topmostPos + m_suspTravel * bottom;
    m_relVel = m_pos - m_lastPos;
    m_relVel -= intVel();
    m_relVel -= dynamics()->movingObjVel();
    m_relVel -= dynamics()->movingRoadVel();
    m_relVel -= collisionData().movement;
    m_relVel -= collide()->movement();
    m_hitboxGroup->collisionData().vel += m_relVel;
    m_lastPos = m_pos;
    m_lastTopDiff = m_pos - topmostPos;
}

/// @addr{0x80599690}
/// @brief Performs the wheel's collision query and updates the wheel's position and radius
/// @param bottom The "down" direction along the suspension axis
/// @param topmostPos The world position of the top of the suspension
/// @details If wheel collisions should not be skipped (indicated via the @ref
/// eStatus::SkipWheelCalc flag in the kart's @ref Status), then calls @ref
/// KartCollide::calcWheelCollision to perform a collision check centered on the bottom of the
/// wheel. If a floor or wall collision occurred, updates the wheel's @ref m_pos. If the tire
/// collided with KCL having "wheel depth", adjusts the wheel's @ref m_targetEffectiveRadius and
/// calls @ref KartBody::trySetTargetSinkDepth() with the corresponding sink depth.
///
/// Sets @ref m_topmostPos to the provided `topmostPos`. Updates @ref m_wheelEdgePos to reflect the
/// new wheel position. Interpolates @ref m_effectiveRadius towards @ref m_targetEffectiveRadius
/// with a rate of `0.1f`. Updates @ref m_suspTravel based on the difference between the wheel's
/// position and `topmostPos` along the suspension axis. If the suspension has overtravel, sets @ref
/// m_hasSuspTravel to `1.0f` and calls @ref KartSub::updateSuspOvertravel().
void WheelPhysics::calcCollision(const EGG::Vector3f &bottom, const EGG::Vector3f &topmostPos) {
    constexpr f32 RADIUS_INTERP_RATE = 0.1f;

    m_targetEffectiveRadius = m_bspWheel->wheelRadius;
    const auto &status = KartObjectProxy::status();

    if (status.offBit(eStatus::SkipWheelCalc)) {
        f32 nextRadius = m_bspWheel->sphereRadius;
        f32 scalar = m_effectiveRadius * scale().y - nextRadius * move()->totalScale();

        EGG::Vector3f center = m_pos + scalar * bottom;
        scalar = 0.3f * (nextRadius * move()->leanRot()) * move()->totalScale();
        center += scalar * bodyRight();

        if (status.onBit(eStatus::ZipperBypassInvisWall, eStatus::InCannon)) {
            m_hitboxGroup->collisionData().reset();
        } else {
            m_hitboxGroup->setHitboxScale(move()->totalScale());
            if (status.onBit(eStatus::SoftWallSuspension)) {
                m_hitboxGroup->hitbox(0).setLastPos(dynamics()->pos());
            }

            collide()->calcWheelCollision(m_wheelIdx, m_hitboxGroup, m_colVel, center, nextRadius);
            CollisionData &colData = m_hitboxGroup->collisionData();

            if (colData.bFloor || colData.bWall || colData.bWall3) {
                m_pos += colData.tangentOff;
                if (colData.intensity > -1) {
                    f32 sinkDepth = 3.0f * static_cast<f32>(colData.intensity);
                    m_targetEffectiveRadius = m_bspWheel->wheelRadius - sinkDepth;
                    body()->trySetTargetSinkDepth(sinkDepth);
                }
            }
        }
        m_hitboxGroup->hitbox(0).setLastPos(center);
    }

    m_topmostPos = topmostPos;
    m_wheelEdgePos = m_pos + m_effectiveRadius * move()->totalScale() * bottom;
    m_effectiveRadius += (m_targetEffectiveRadius - m_effectiveRadius) * RADIUS_INTERP_RATE;
    m_suspTravel = bottom.dot(m_pos - topmostPos);

    if (m_suspTravel < 0.0f) {
        m_hasSuspTravel = 1.0f;
        EGG::Vector3f suspBottom = m_suspTravel * bottom;
        sub()->updateSuspOvertravel(suspBottom);
    } else {
        m_hasSuspTravel = 0.0f;
    }
}

/// @addr{0x80599DC0}
/// @brief Applies floor moment using the wheel's current speed
/// @param forward The forward direction of the wheel
/// @details Calculates the sideways moment cap based on whether the kart is contacting a soft wall
/// or not. Dispatches to @ref KartCollide::applySomeFloorMoment() with the calculated cap.
void WheelPhysics::calcSuspension(const EGG::Vector3f &forward) {
    f32 rate = status().onBit(eStatus::SoftWallPush) ? 0.01f : collide()->floorMomentRate();

    collide()->applySomeFloorMoment(0.1f, rate, m_hitboxGroup, forward, move()->dir(), m_relVel,
            true, true, status().offBit(eStatus::LargeFlipHit, eStatus::WheelieRot));
}

/// @brief Sets the current suspension travel distance
/// @param suspTravel The new suspension travel distance to set
void WheelPhysics::setSuspTravel(f32 suspTravel) {
    m_suspTravel = suspTravel;
}

/// @brief Sets the current position of the wheel
/// @param pos The new position to set
void WheelPhysics::setPos(const EGG::Vector3f &pos) {
    m_pos = pos;
}

/// @brief Sets the last position of the wheel
/// @param pos The new last position to set
void WheelPhysics::setLastPos(const EGG::Vector3f &pos) {
    m_lastPos = pos;
}

/// @brief Sets the distance between the tire's position and the topmost point of the suspension
/// @param pos The new distance to set
void WheelPhysics::setLastTopDiff(const EGG::Vector3f &pos) {
    m_lastTopDiff = pos;
}

/// @brief Sets the position at the bottom outer edge of the wheel
/// @param pos The new wheel edge position to set
void WheelPhysics::setWheelEdgePos(const EGG::Vector3f &pos) {
    m_wheelEdgePos = pos;
}

/// @brief Sets the velocity of the wheel
/// @param vec The new velocity to set
void WheelPhysics::setColVel(const EGG::Vector3f &vec) {
    m_colVel = vec;
}

/// @brief Gets the current position of the tire
/// @return The current world position of the tire, @ref m_pos
const EGG::Vector3f &WheelPhysics::pos() const {
    return m_pos;
}

/// @brief Gets the distance between the tire and its topmost point of the suspension
/// @return The distance between the tire and the topmost point of the suspension, @ref
/// m_lastTopDiff
const EGG::Vector3f &WheelPhysics::lastTopDiff() const {
    return m_lastTopDiff;
}

/// @brief Gets the current suspension travel distance
/// @return The current suspension travel distance, @ref m_suspTravel
f32 WheelPhysics::suspTravel() const {
    return m_suspTravel;
}

/// @brief Gets the topmost point of the suspension
/// @return The topmost point of the suspension, @ref m_topmostPos
const EGG::Vector3f &WheelPhysics::topmostPos() const {
    return m_topmostPos;
}

/// @brief Gets the collision group associated with this wheel
/// @return Ref to the collision group, @ref m_hitboxGroup
CollisionGroup *WheelPhysics::hitboxGroup() {
    return m_hitboxGroup;
}

/// @brief Gets the collision group associated with this wheel
/// @return Const ref to the collision group, @ref m_hitboxGroup
const CollisionGroup *WheelPhysics::hitboxGroup() const {
    return m_hitboxGroup;
}

/// @brief Gets the wheel's velocity relative to the vehicle body
/// @return The wheel's relative velocity, @ref m_relVel
const EGG::Vector3f &WheelPhysics::relVel() const {
    return m_relVel;
}

/// @brief Gets the position at the bottom outer edge of the wheel
/// @return The position at the bottom outer edge of the wheel, @ref m_wheelEdgePos
const EGG::Vector3f &WheelPhysics::wheelEdgePos() const {
    return m_wheelEdgePos;
}

/// @brief Gets the current smoothed wheel radius
/// @return The current smoothed wheel radius, @ref m_effectiveRadius
f32 WheelPhysics::effectiveRadius() const {
    return m_effectiveRadius;
}

/// @brief Checks if the suspension has travel
/// @return The value of @ref m_hasSuspTravel
f32 WheelPhysics::hasSuspTravel() const {
    return m_hasSuspTravel;
}

/// @addr{0x80599ED4}
/// @brief Constructor
/// @param wheelIdx The index of the wheel
/// @param tireType The type of tire
/// @param bspWheelIdx The index of the wheel in the @ref BSP::wheels array
/// @details Sets @ref m_tireType, @ref m_bspWheelIdx, and @ref m_wheelIdx to the provided
/// `tireType`, `bspWheelIdx`, and `wheelIdx` respectively. Initializes @ref m_bspWheel to
/// `nullptr`.
KartSuspensionPhysics::KartSuspensionPhysics(u16 wheelIdx, TireType tireType, u16 bspWheelIdx)
    : m_tirePhysics(nullptr),
      m_tireType(tireType),
      m_bspWheelIdx(bspWheelIdx),
      m_wheelIdx(wheelIdx) {}

/// @addr{0x8059AA04}
/// @brief Default virtual destructor
KartSuspensionPhysics::~KartSuspensionPhysics() = default;

/// @addr{0x80599FA0}
/// @brief Initializes the @ref WheelPhysics and @ref BSP::Wheel pointers for this suspension
/// @details @ref m_tirePhysics is set by indexing into the kart's tire array using @ref m_wheelIdx,
/// and @ref m_bspWheel is set by indexing into the @ref BSP::wheel array using @ref m_bspWheelIdx.
void KartSuspensionPhysics::init() {
    m_tirePhysics = tire(m_wheelIdx)->wheelPhysics();
    m_bspWheel = &bsp().wheels[m_bspWheelIdx];
}

/// @addr{0x80599F54}
/// @brief Resets the suspension physics to its initial state
/// @details Resets @ref m_topmostPos, @ref m_maxTravelScaled, and @ref m_bottomDir to zero.
void KartSuspensionPhysics::reset() {
    m_topmostPos.setZero();
    m_maxTravelScaled = 0.0f;
    m_bottomDir.setZero();
}

/// @addr{0x8059A02C}
/// @brief Sets the initial state of the suspension, including the wheel's position and topmost
/// suspension point
/// @details If @ref m_tireType is @ref TireType::KartReflected, then the X-coordinate of the
/// wheel's relative position is flipped. Sets the tire's initial world position based on the
/// rotated relative position summed with the kart's position and the suspension's @ref
/// BSP::Wheel::maxTravel. The tire's @ref WheelPhysics::m_lastPos is set to the same initial world
/// position. The tire's @ref WheelPhysics::m_lastTopDiff and @ref WheelPhysics::m_wheelEdgePos are
/// also initialized based on this position. Copies the tire's world position to the hitbox's world
/// position, and sets the hitbox's last position to `100.0f` units above the kart's position.
/// Finally, sets @ref m_topmostPos to the rotated relative position summed with the kart's
/// position.
void KartSuspensionPhysics::setInitialState() {
    EGG::Vector3f relPos = m_bspWheel->springTop;
    if (m_tireType == TireType::KartReflected) {
        relPos.x = -relPos.x;
    }

    const EGG::Vector3f worldPos = fullRot().rotateVector(relPos) + pos();
    const EGG::Vector3f down = fullRot().rotateVector(-EGG::Vector3f::ey);

    m_tirePhysics->setPos(worldPos + m_bspWheel->maxTravel * down);
    m_tirePhysics->setLastPos(worldPos + m_bspWheel->maxTravel * down);
    m_tirePhysics->setLastTopDiff(m_tirePhysics->pos() - worldPos);
    m_tirePhysics->setWheelEdgePos(m_tirePhysics->pos() +
            (m_tirePhysics->effectiveRadius() * move()->totalScale() * down));
    m_tirePhysics->hitboxGroup()->hitbox(0).setWorldPos(m_tirePhysics->pos());
    m_tirePhysics->hitboxGroup()->hitbox(0).setLastPos(pos() + 100.0f * EGG::Vector3f::ey);
    m_topmostPos = worldPos;
}

/// @addr{0x8059A278}
/// @brief Updates the tire's position and calls @ref WheelPhysics::calcCollision
/// @param gravity The gravity vector affecting the wheel
/// @param mat The world transformation matrix for the wheel's position and orientation
/// @details Sets @ref m_maxTravelScaled based on the wheel's maximum travel and the suspension
/// scale. Updates @ref m_bottomDir based off the provided `mat` transformation matrix. Sets @ref
/// WheelPhysics::m_suspTravel based on the current suspension compression. Sets @ref
/// WheelPhysics::m_colVel to `dt * 10.0f * gravity`. Sets @ref WheelPhysics::m_pos based on the
/// topmost position and the suspension travel distance along @ref m_bottomDir. Finally, if wheel
/// collision should not be skipped (indicated via @ref eStatus::SkipWheelCalc), calls @ref
/// WheelPhysics::calcCollision() and updates @ref m_topmostPos.
void KartSuspensionPhysics::calcCollision(const EGG::Vector3f &gravity, const EGG::Matrix34f &mat) {
    m_maxTravelScaled = m_bspWheel->maxTravel * sub()->suspScale();

    EGG::Vector3f topmostRelPos = m_bspWheel->springTop * scale();
    if (m_tireType == TireType::KartReflected) {
        topmostRelPos.x = -topmostRelPos.x;
    }

    const EGG::Vector3f topmostPos = mat.ps_multVector(topmostRelPos);
    EGG::Matrix34f xRotMat;
    xRotMat.makeR(EGG::Vector3f(m_bspWheel->xRot * DEG2RAD, 0.0f, 0.0f));
    EGG::Vector3f localBottomDir = xRotMat.multVector33(-EGG::Vector3f::ey);
    m_bottomDir = mat.multVector33(localBottomDir);

    f32 y_down = m_tirePhysics->suspTravel() + 5.0f * sub()->suspScale();
    m_tirePhysics->setSuspTravel(std::clamp(y_down, 0.0f, m_maxTravelScaled));
    m_tirePhysics->setColVel(10.0f * gravity);
    m_tirePhysics->setPos(topmostPos + m_tirePhysics->suspTravel() * m_bottomDir);

    if (status().offBit(eStatus::SkipWheelCalc)) {
        m_tirePhysics->calcCollision(m_bottomDir, topmostPos);
        m_topmostPos = topmostPos;
    }
}

/// @addr{0x8059A574}
/// @brief Calculates linear force and rotation from the kart's suspension
/// @param forward The forward direction of the wheel
/// @param vehicleMovement The movement of the vehicle to account for in the wheel's position
/// @details Calls @ref WheelPhysics::realign() to update the wheel's position and velocity based on
/// @ref m_bottomDir and the provided `vehicleMovement`. If the tire did not collide with the floor
/// this frame, this function returns early. Based on the suspension travel, suspension stiffness,
/// and other properties, applies a torque to the vehicle via @ref
/// KartDynamics::applySuspensionWrench(). Finally, calls @ref WheelPhysics::calcSuspension() to
/// apply angular and external velocity to the vehicle.
void KartSuspensionPhysics::calcSuspension(const EGG::Vector3f &forward,
        const EGG::Vector3f &vehicleMovement) {
    EGG::Vector3f lastTopDiff = m_tirePhysics->lastTopDiff();

    m_tirePhysics->realign(m_bottomDir, vehicleMovement);

    CollisionData &collisionData = m_tirePhysics->hitboxGroup()->collisionData();
    if (!collisionData.bFloor) {
        return;
    }

    EGG::Vector3f topDiff = m_tirePhysics->pos() - m_topmostPos;
    f32 yDown = std::max(0.0f, m_bottomDir.dot(topDiff));
    EGG::Vector3f speed = lastTopDiff - topDiff;
    f32 travel = m_maxTravelScaled - yDown;
    f32 speedScalar = m_bottomDir.dot(speed);

    f32 springDamp =
            -(m_bspWheel->springStiffness * travel + m_bspWheel->dampingFactor * speedScalar);

    EGG::Vector3f fRot = m_bottomDir * springDamp;

    if (isPostRespawn()) {
        fRot.y = std::clamp(fRot.y, -1.0f, 1.0f);
    }

    EGG::Vector3f fLinear = fRot;
    EGG::Vector3f rotProj = fRot;
    rotProj.y = 0.0f;

    rotProj = rotProj.proj(collisionData.floorNrm);
    fLinear.y = std::min(fLinear.y + rotProj.y, param()->stats().maxNormalForce);

    auto &status = KartObjectProxy::status();

    if (extVel().y > 5.0f || status.onBit(eStatus::JumpPadDisableYsusForce)) {
        fLinear.y = 0.0f;
    }

    dynamics()->applySuspensionWrench(m_topmostPos, fLinear, fRot,
            status.onBit(eStatus::WheelieRot));

    m_tirePhysics->calcSuspension(forward);
}

} // namespace Kinoko::Kart
