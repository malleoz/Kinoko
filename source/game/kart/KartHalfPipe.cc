#include "KartHalfPipe.hh"

#include "game/kart/KartCollide.hh"
#include "game/kart/KartPhysics.hh"

#include "game/field/KColData.hh"

namespace Kinoko::Kart {

/// @addr{0x80574114}
/// @brief Constructor
KartHalfPipe::KartHalfPipe() : m_prevPos(EGG::Vector3f::zero) {}

/// @addr{0x80574170}
/// @brief Default destructor
KartHalfPipe::~KartHalfPipe() = default;

/// @addr{0x80574340}
/// @brief Every frame, checks if the kart is on over went over a half-pipe zipper and updates the
/// kart's rotation accordingly. When the player starts getting airtime off the zipper, derives the
/// total duration the kart will be in the air based on the current velocity. Determines if the
/// player is attempting a trick and references the derived duration to determine if a trick is
/// allowed to occur. Alters the kart's rotation based on the trick being performed and applies a
/// boost if the player lands.
void KartHalfPipe::calc() {
    constexpr s16 LANDING_BOOST_DELAY = 3;
    constexpr f32 GRAVITY = 1.3f;

    auto &status = KartObjectProxy::status();

    if (state()->airtime() > 15 && status.onBit(eStatus::OverZipper)) {
        m_landingBoostDelayTimer = LANDING_BOOST_DELAY;
    }

    bool isOnZipper = status.onBit(eStatus::HalfPipeRamp) && m_landingBoostDelayTimer <= 0;

    calcTrick();

    if (status.offBit(eStatus::InAction) &&
            collide()->surfaceFlags().offBit(KartCollide::eSurfaceFlags::EndHalfPipe)) {
        if (m_touchingZipper && status.onBit(eStatus::AirStart)) {
            dynamics()->setExtVel(EGG::Vector3f::zero);
            status.setBit(eStatus::OverZipper);

            EGG::Vector3f upXZ = move()->up();
            upXZ.y = 0.0f;
            upXZ.normalise();
            EGG::Vector3f up = move()->dir().perpInPlane(upXZ, true);

            EGG::Vector3f local_64 = up.cross(bodyUp().perpInPlane(up, true));
            m_nextSign = local_64.dot(EGG::Vector3f::ey) > 0.0f ? 1.0f : -1.0f;

            EGG::Vector3f velNorm = velocity();
            velNorm.normalise();
            EGG::Vector3f rot = mainRot().rotateVectorInv(velNorm);

            m_launchRot.makeVectorRotation(rot, EGG::Vector3f::ez);
            m_prevPos = prevPos();

            calcCollision(false);

            f32 scaledDir = std::min(65.0f, move()->dir().y * speed());
            m_attemptedTrickTimer = std::max<s32>(0, scaledDir * 2.0f / GRAVITY - 1.0f);
        } else if (status.onBit(eStatus::OverZipper)) {
            dynamics()->setGravity(-GRAVITY);

            EGG::Vector3f side = mainRot().rotateVector(EGG::Vector3f::ez);
            EGG::Vector3f velNorm = velocity();
            velNorm.normalise();

            EGG::Quatf sideRot;
            sideRot.makeVectorRotation(side, velNorm);
            sideRot = sideRot.multSwap(mainRot()).multSwap(m_launchRot);

            f32 t = move()->CalcSlerpRate(DEG2RAD360, mainRot(), sideRot);
            EGG::Quatf slerp = mainRot().slerpTo(sideRot, t);
            dynamics()->setFullRot(slerp);
            dynamics()->setMainRot(slerp);

            --m_attemptedTrickTimer;

            calcTrickRot();
            calcCollision(false);
        } else if (status.onBit(eStatus::HalfPipeRamp)) {
            calcCollision(true);
        } else {
            status.resetBit(eStatus::ZipperBypassInvisWall);
        }
    }

    m_landingBoostDelayTimer = std::max(0, m_landingBoostDelayTimer - 1);
    m_touchingZipper = isOnZipper;
}

/// @addr{0x80574C90}
/// @brief Evaluates whether the player is attempting a trick and whether the trick can be performed
/// @details Caches the attempted trick input for 10 frames. If the player is over the zipper, there
/// is a cached trick input, and the player has between 3 and 10 frames of airtime, activates the
/// input trick. Finally, decrements the cache/leniency timer.
void KartHalfPipe::calcTrick() {
    constexpr s16 TRICK_COOLDOWN = 10;

    auto &trick = inputs()->currentState().trick;

    if (trick != System::Trick::None) {
        m_leniencyTimer = TRICK_COOLDOWN;
        m_trick = trick;
    }

    const auto &status = KartObjectProxy::status();

    if (status.onBit(eStatus::OverZipper)) {
        if (status.offBit(eStatus::ZipperTrick) && m_leniencyTimer > 0 && state()->airtime() > 3 &&
                state()->airtime() < 10) {
            activateTrick(m_attemptedTrickTimer, m_trick);
        }
    }

    m_leniencyTimer = std::max(0, m_leniencyTimer - 1);
}

/// @addr{0x805750CC}
/// @brief Influences the rotation of the kart based on the trick being performed
/// @details Does nothing if there is no trick being performed. Computes the cumulative rotation
/// angle based on the properties of the trick being performed. With that angle, evaluates the
/// quaternion to represent the rotation of the kart based on the type of trick being performed.
void KartHalfPipe::calcTrickRot() {
    if (m_stunt == StuntType::None) {
        return;
    }

    m_stuntManager.calcAngle();

    f32 angle = m_rotSign * (DEG2RAD * m_stuntManager.angle);

    switch (m_stunt) {
    case StuntType::Side360:
    case StuntType::Side720:
        m_stuntRot.setRPY(0.0f, angle, 0.0f);
        break;
    case StuntType::Backside: {
        auto rpy = EGG::Quatf::FromRPY(0.0f, DEG2RAD * (0.25f * -m_rotSign * m_stuntManager.angle),
                0.0f);
        EGG::Vector3f rot = rpy.rotateVector(EGG::Vector3f::ez);
        m_stuntRot.setAxisRotation(angle, rot);
    } break;
    case StuntType::Frontside: {
        EGG::Quatf rpy = EGG::Quatf::FromRPY(0.0f, 0.0f,
                DEG2RAD * (0.2f * -m_rotSign * m_stuntManager.angle));
        EGG::Vector3f rot = rpy.rotateVector(EGG::Vector3f::ey);
        m_stuntRot.setAxisRotation(angle, rot);
    } break;
    case StuntType::Frontflip:
        m_stuntRot.setRPY(m_rotSign * angle, 0.0f, 0.0f);
        break;
    case StuntType::Backflip:
        m_stuntRot.setRPY(-m_rotSign * angle, 0.0f, 0.0f);
        break;
    default:
        break;
    }

    physics()->composeStuntRot(m_stuntRot);
}

/// @addr{0x805752E8}
/// @brief Checks for collisions with the half-pipe walls and floor
/// @param notAirborne True if the kart is not airborne, false otherwise
/// @details If the kart is colliding with the half-pipe wall collision, then the kart's up vector
/// is interpolated towards the wall normal and the kart's position is adjusted to be on the wall.
void KartHalfPipe::calcCollision(bool notAirborne) {
    constexpr f32 LANDING_RADIUS = 150.0f;
    constexpr f32 PREVIOUS_RADIUS = 200.0f;
    constexpr f32 MIDAIR_RADIUS = 50.0f;
    constexpr f32 WALL_RADIUS = 100.0f;
    constexpr f32 UP_INTERP_RATE = 0.2f;

    Field::CollisionInfo colInfoFloor;
    Field::CollisionInfo colInfoWall;
    Field::KCLTypeMask maskOut;
    EGG::Vector3f pos;
    EGG::Vector3f upLocal;

    auto &status = KartObjectProxy::status();
    Field::KCLTypeMask mask = KCL_TYPE_ANY_INVISIBLE_WALL;
    bool overZipper = status.onBit(eStatus::OverZipper);
    if (!overZipper) {
        if (notAirborne && velocity().y < 0.0f) {
            mask = KCL_NONE;
        } else {
            mask = KCL_TYPE_BIT(COL_TYPE_HALFPIPE_INVISIBLE_WALL);
        }
    }

    status.resetBit(eStatus::ZipperBypassInvisWall);

    EGG::Vector3f prevPos = m_prevPos + EGG::Vector3f::ey * PREVIOUS_RADIUS;

    bool hasDriverFloorCollision = move()->calcCollisions(LANDING_RADIUS, bsp().offsetY, pos,
            upLocal, prevPos, &colInfoFloor, &maskOut, KCL_TYPE_DRIVER_FLOOR);

    prevPos = hasDriverFloorCollision ? EGG::Vector3f::inf : prevPos;

    if (overZipper) {
        if (!move()->calcCollisions(MIDAIR_RADIUS, bsp().offsetY, pos, upLocal, prevPos,
                    &colInfoWall, &maskOut, mask)) {
            mask |= KCL_TYPE_DRIVER_WALL;
        }
    }

    if (move()->calcCollisions(WALL_RADIUS, bsp().offsetY, pos, upLocal, prevPos, &colInfoWall,
                &maskOut, mask)) {
        if ((maskOut & ~KCL_TYPE_BIT(COL_TYPE_HALFPIPE_INVISIBLE_WALL)) == 0) {
            status.setBit(eStatus::ZipperBypassInvisWall);
        }

        EGG::Vector3f up = move()->up();
        move()->setUp(up + (colInfoWall.wallNrm - up) * UP_INTERP_RATE);
        move()->setSmoothedUp(move()->up());

        f32 yScale = bsp().offsetY * scale().y;
        EGG::Vector3f newPos = pos + colInfoWall.tangentOff + -WALL_RADIUS * colInfoWall.wallNrm +
                yScale * upLocal;
        newPos.y += move()->hopPosY();

        dynamics()->setPos(newPos);
        move()->setDir(move()->dir().perpInPlane(move()->up(), true));
        move()->setVel1Dir(move()->dir());

        if (overZipper) {
            status.setBit(eStatus::ZipperStick);
        }

        m_prevPos = newPos;
    } else {
        if (overZipper) {
            status.resetBit(eStatus::ZipperStick);
        }
    }

    if (!hasDriverFloorCollision || status.onBit(eStatus::ZipperBypassInvisWall) ||
            state()->airtime() <= 5) {
        return;
    }

    if (colInfoFloor.floorNrm.dot(EGG::Vector3f::ey) <= COS_PI_OVER_4) {
        return;
    }

    if (status.onBit(eStatus::OverZipper)) {
        status.resetBit(eStatus::ZipperStick);
    }
}

/// @addr{0x80574E60}
/// @brief Activates a trick based on the provided duration and trick input
/// @param duration The duration of the airtime, must be greater than 50 frames to activate a trick
/// @param trick The trick type to activate
/// @details Requires that the duration of the attempted trick is greater than 50 frames. Determines
/// which trick type to activate based on the trick input and the duration of the airtime. Finally,
/// initializes the properties of the trick in the @ref StuntManager and sets the @ref
/// eStatus::ZipperTrick bit.
void KartHalfPipe::activateTrick(s32 duration, System::Trick trick) {
    if (duration < 51 || trick == System::Trick::None) {
        m_stunt = StuntType::None;
    } else {
        m_rotSign = m_nextSign;
        bool timerThreshold = duration > 70;

        switch (trick) {
        case System::Trick::Up:
            m_stunt = timerThreshold ? StuntType::Backside : StuntType::Backflip;
            break;
        case System::Trick::Down:
            m_stunt = timerThreshold ? StuntType::Frontside : StuntType::Frontflip;
            break;
        case System::Trick::Left:
        case System::Trick::Right:
            m_stunt = timerThreshold ? StuntType::Side720 : StuntType::Side360;
            m_rotSign = trick == System::Trick::Left ? 1.0f : -1.0f;
            break;
        default:
            break;
        }

        m_stuntManager.setProperties(static_cast<size_t>(m_stunt));

        status().setBit(eStatus::ZipperTrick);
    }

    m_stuntRot = EGG::Quatf::ident;
}

/// @addr{0x805758E4}
/// @brief Called when the kart should end the half-pipe state and applies any necessary boosts
/// @param boost True if a boost should be applied, false otherwise
/// @details If the kart was over the zipper and had more than 5 frames of airtime, applies a boost
/// whose duration depends on whether a trick occurred.
void KartHalfPipe::end(bool boost) {
    auto &status = KartObjectProxy::status();
    bool overZipper = status.onBit(eStatus::OverZipper);

    if (overZipper && state()->airtime() > 5 && boost) {
        move()->activateZipperBoost();
    }

    if (status.onBit(eStatus::ZipperTrick)) {
        physics()->composeDecayingStuntRot(m_stuntRot);
    }

    if (overZipper) {
        move()->setDir(mainRot().rotateVector(EGG::Vector3f::ez));
        move()->setVel1Dir(move()->dir());
    }

    status.resetBit(eStatus::OverZipper, eStatus::ZipperTrick, eStatus::ZipperStick,
            eStatus::ZipperBypassInvisWall);

    m_stunt = StuntType::None;
}

/// @brief Calculates the cumulative angle of rotation for the current trick
/// @details If the current angle is greater than the target rotation multiplied by the angle decay
/// threshold, the rotation speed is decreased by multiplying it with the decay rate, and the decay
/// rate is decreased by subtracting the decay rate delta from it. The angle is then updated by
/// adding the rotation speed to it, capped at the target rotation.
void KartHalfPipe::StuntManager::calcAngle() {
    if (properties.targetRot * properties.angleDecayThreshold < angle) {
        rotSpeed = std::max(properties.minRotSpeed, rotSpeed * decayRate);
        decayRate = std::max(properties.initDecayRate, decayRate - properties.decayRateDelta);
    }

    angle = std::min(properties.targetRot, angle + rotSpeed);
}

/// @brief Sets the properties of the current stunt based on the provided index
/// @param idx The index of the stunt properties to set
void KartHalfPipe::StuntManager::setProperties(size_t idx) {
    static constexpr std::array<StuntProperties, 6> STUNT_PROPERTIES = {{
            {6.0f, 2.5f, 0.955f, 0.01f, 0.7f, 360.0f},
            {7.0f, 3.0f, 0.955f, 0.01f, 0.7f, 360.0f},
            {7.0f, 3.0f, 0.95f, 0.01f, 0.7f, 360.0f},
            {12.0f, 2.5f, 0.955f, 0.01f, 0.0f, 360.0f},
            {4.0f, 4.0f, 0.98f, 0.01f, 0.0f, 360.0f},
            {9.0f, 3.0f, 0.92f, 0.01f, 0.8f, 720.0f},
    }};

    ASSERT(idx < STUNT_PROPERTIES.size());

    properties = STUNT_PROPERTIES[idx];
    rotSpeed = properties.initRotSpeed;
    angle = 0.0f;
    decayRate = 1.0f;
}

} // namespace Kinoko::Kart
