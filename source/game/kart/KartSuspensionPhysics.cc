#include "KartSuspensionPhysics.hh"

#include "game/kart/KartCollide.hh"
#include "game/kart/KartSub.hh"
#include "game/kart/KartTire.hh"

namespace Kinoko::Kart {

/// @addr{0x8059940C}
/// @brief Constructor
/// @param wheelIdx The index of the wheel
/// @param bspWheelIdx The index of the wheel in the @ref BSP::wheels array
WheelPhysics::WheelPhysics(u16 wheelIdx, u16 bspWheelIdx)
    : m_wheelIdx(wheelIdx), m_bspWheelIdx(bspWheelIdx), m_bspWheel(nullptr) {}

/// @addr{0x8059A9C4}
/// @brief Destructor that destroys the underlying @CollisionGroup subsystem for this wheel
WheelPhysics::~WheelPhysics() {
    EGG::egg_delete(m_hitboxGroup);
}

/// @addr{0x80599508}
/// @brief Resets the wheel physics to its initial state
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
void WheelPhysics::realign(const EGG::Vector3f &bottom, const EGG::Vector3f &vehicleMovement) {
    const EGG::Vector3f topmostPos = m_topmostPos + vehicleMovement;
    f32 scaledMaxTravel = m_bspWheel->maxTravel * sub()->suspScale();
    f32 suspTravel = bottom.dot(m_pos - topmostPos);
    m_suspTravel = std::max(0.0f, std::min(scaledMaxTravel, suspTravel));
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
void WheelPhysics::calcCollision(const EGG::Vector3f &bottom, const EGG::Vector3f &topmostPos) {
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
    m_effectiveRadius += (m_targetEffectiveRadius - m_effectiveRadius) * 0.1f;
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
void WheelPhysics::calcSuspension(const EGG::Vector3f &forward) {
    f32 rate = status().onBit(eStatus::SoftWallPush) ? 0.01f : collide()->floorMomentRate();

    collide()->applySomeFloorMoment(0.1f, rate, m_hitboxGroup, forward, move()->dir(), m_relVel,
            true, true, status().offBit(eStatus::LargeFlipHit, eStatus::WheelieRot));
}

/// @addr{0x80599ED4}
/// @brief Constructor
/// @param wheelIdx The index of the wheel
/// @param tireType The type of tire
/// @param bspWheelIdx The index of the wheel in the @ref BSP::wheels array
KartSuspensionPhysics::KartSuspensionPhysics(u16 wheelIdx, TireType tireType, u16 bspWheelIdx)
    : m_tirePhysics(nullptr), m_tireType(tireType), m_bspWheelIdx(bspWheelIdx),
      m_wheelIdx(wheelIdx) {}

/// @addr{0x8059AA04}
/// @brief Default destructor
KartSuspensionPhysics::~KartSuspensionPhysics() = default;

/// @addr{0x80599FA0}
/// @brief Fetches the @ref WheelPhysics and @ref BSP::Wheel pointers for this suspension
void KartSuspensionPhysics::init() {
    m_tirePhysics = tire(m_wheelIdx)->wheelPhysics();
    m_bspWheel = &bsp().wheels[m_bspWheelIdx];
}

/// @addr{0x8059A02C}
/// @brief Sets the initial state of the suspension, including the wheel's position and topmost
/// suspension point
void KartSuspensionPhysics::setInitialState() {
    EGG::Vector3f relPos = m_bspWheel->springTop;
    if (m_tireType == TireType::KartReflected) {
        relPos.x = -relPos.x;
    }

    const EGG::Vector3f rotatedRelPos = fullRot().rotateVector(relPos) + pos();
    const EGG::Vector3f unitRotated = fullRot().rotateVector(-EGG::Vector3f::ey);

    m_tirePhysics->setPos(rotatedRelPos + m_bspWheel->maxTravel * unitRotated);
    m_tirePhysics->setLastPos(rotatedRelPos + m_bspWheel->maxTravel * unitRotated);
    m_tirePhysics->setLastTopDiff(m_tirePhysics->pos() - rotatedRelPos);
    m_tirePhysics->setWheelEdgePos(m_tirePhysics->pos() +
            (m_tirePhysics->effectiveRadius() * move()->totalScale() * unitRotated));
    m_tirePhysics->hitboxGroup()->hitbox(0).setWorldPos(m_tirePhysics->pos());
    m_tirePhysics->hitboxGroup()->hitbox(0).setLastPos(pos() + 100 * EGG::Vector3f::ey);
    m_topmostPos = rotatedRelPos;
}

/// @addr{0x8059A278}
/// @brief Updates the tire's position and calls @ref WheelPhysics::calcCollision
/// @param dt The time step (always 1.0f)
/// @param gravity The gravity vector affecting the wheel
/// @param mat The world transformation matrix for the wheel's position and orientation
void KartSuspensionPhysics::calcCollision(f32 dt, const EGG::Vector3f &gravity,
        const EGG::Matrix34f &mat) {
    m_maxTravelScaled = m_bspWheel->maxTravel * sub()->suspScale();

    EGG::Vector3f scaledRelPos = m_bspWheel->springTop * scale();
    if (m_tireType == TireType::KartReflected) {
        scaledRelPos.x = -scaledRelPos.x;
    }

    const EGG::Vector3f topmostPos = mat.ps_multVector(scaledRelPos);
    EGG::Matrix34f xRotMat;
    EGG::Vector3f euler_angles(m_bspWheel->xRot * DEG2RAD, 0.0f, 0.0f);
    xRotMat.makeR(euler_angles);
    EGG::Vector3f localBottomDir = xRotMat.multVector33(EGG::Vector3f(0.0f, -1.0f, 0.0f));
    m_bottomDir = mat.multVector33(localBottomDir);

    f32 y_down = m_tirePhysics->suspTravel() + 5.0f * sub()->suspScale();
    m_tirePhysics->setSuspTravel(std::max(0.0f, std::min(m_maxTravelScaled, y_down)));
    m_tirePhysics->setColVel(dt * 10.0f * gravity);
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
        fRot.y = std::max(-1.0f, std::min(1.0f, fRot.y));
    }

    EGG::Vector3f fLinear = fRot;
    EGG::Vector3f rotProj = fRot;
    rotProj.y = 0.0f;

    rotProj = rotProj.proj(collisionData.floorNrm);
    fLinear.y += rotProj.y;
    fLinear.y = std::min(fLinear.y, param()->stats().maxNormalForce);

    auto &status = KartObjectProxy::status();

    if (extVel().y > 5.0f || status.onBit(eStatus::JumpPadDisableYsusForce)) {
        fLinear.y = 0.0f;
    }

    dynamics()->applySuspensionWrench(m_topmostPos, fLinear, fRot,
            status.onBit(eStatus::WheelieRot));

    m_tirePhysics->calcSuspension(forward);
}

} // namespace Kinoko::Kart
