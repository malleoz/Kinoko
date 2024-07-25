#include "KartHalfPipe.hh"

#include "game/kart/KartDynamics.hh"
#include "game/kart/KartMove.hh"
#include "game/kart/KartParam.hh"
#include "game/kart/KartPhysics.hh"
#include "game/kart/KartState.hh"

#include "game/field/CourseColMgr.hh"

#include <egg/math/Math.hh>

namespace Kart {

/// @addr{0x80574114}
KartHalfPipe::KartHalfPipe() = default;

/// @addr{0x80574170}
KartHalfPipe::~KartHalfPipe() = default;

/// @addr{0x805741B0}
void KartHalfPipe::reset() {
    m_type = TrickType::None;
    m_touchingZipper = false;
    m_timer = 0;
}

/// @addr{0x80574340}
void KartHalfPipe::calc() {
    constexpr s16 LANDING_BOOST_DELAY = 3;

    if (state()->airtime() > 0xF && state()->isOverZipper()) {
        m_timer = LANDING_BOOST_DELAY;
    }

    bool bVar14 = state()->isHalfpipeRamp() && m_timer == 0;

    calcTrick();

    if (m_touchingZipper && state()->isAirStart()) {
        dynamics()->setExtVel(EGG::Vector3f::zero);
        state()->setOverZipper(true);

        EGG::Vector3f upXZ = move()->dir();
        upXZ.y = 0.0f;
        upXZ.normalise();
        EGG::Vector3f up = move()->dir().perpInPlane(upXZ, true);

        m_prevPos = up.cross(bodyUp().perpInPlane(up, true));
        m_nextSign = m_prevPos.dot(EGG::Vector3f::ey) > 0.0f ? 1.0f : -1.0f;

        EGG::Vector3f velNorm = velocity();
        velNorm.normalise();
        EGG::Vector3f rot = dynamics()->mainRot().rotateVectorInv(velNorm);

        m_rot.makeVectorRotation(rot, EGG::Vector3f::ez);
        m_prevPos = prevPos();

        calcLanding(false);

        f32 scaledDir = std::min(65.0f, move()->dir().y * move()->speed());
        m_attemptedTrickTimer = std::max<s32>(0, scaledDir * 2.0f / 1.3f - 1.0f);
    } else {
        if (state()->isOverZipper()) {
            dynamics()->setGravity(-1.3f);

            EGG::Vector3f side = mainRot().rotateVector(EGG::Vector3f::ez);
            EGG::Vector3f velNorm = velocity();
            velNorm.normalise();

            EGG::Quatf sideRot;
            sideRot.makeVectorRotation(side, velNorm);
            sideRot = sideRot.multSwap(mainRot()).multSwap(m_rot);

            f32 t = move()->calcSlerpRate(DEG2RAD360, mainRot(), sideRot);
            EGG::Quatf slerp = mainRot().slerpTo(sideRot, t);
            dynamics()->setFullRot(slerp);
            dynamics()->setMainRot(slerp);

            --m_attemptedTrickTimer;

            calcRot();
            calcLanding(false);
        } else {
            if (state()->isHalfpipeRamp()) {
                calcLanding(true);
            }
        }
    }

    m_timer = std::max(0, m_timer - 1);

    m_touchingZipper = bVar14;
}

/// @addr{0x80574C90}
void KartHalfPipe::calcTrick() {
    constexpr s16 TRICK_COOLDOWN = 10;

    auto &trick = inputs()->currentState().trick;

    if (trick != System::Trick::None) {
        m_nextTimer = TRICK_COOLDOWN;
        m_trick = trick;
    }

    if (state()->isOverZipper()) {
        if (!state()->isZipperTrick() && m_nextTimer > 0 && state()->airtime() > 3 &&
                state()->airtime() < 10) {
            activateTrick(m_attemptedTrickTimer, m_trick);
        }
    }

    m_nextTimer = std::max(0, m_nextTimer - 1);
}

/// @addr{0x805750CC}
void KartHalfPipe::calcRot() {
    if (m_type == TrickType::None) {
        return;
    }

    if (m_sub.finalAngle * m_sub.properties.finalAngleScalar < m_sub.angle) {
        m_sub.angleDelta =
                std::max(m_sub.properties.angleDeltaMin, m_sub.angleDelta * m_sub.angleDeltaFactor);
        m_sub.angleDeltaFactor = std::max(m_sub.properties.angleDeltaFactorMin,
                m_sub.angleDeltaFactor - m_sub.properties.angleDeltaFactorDecr);
    }

    m_sub.angle = std::min(m_sub.finalAngle, m_sub.angle + m_sub.angleDelta);

    switch (m_type) {
    case TrickType::Side360:
    case TrickType::Side720:
        m_stuntRot.setRPY(EGG::Vector3f(0.0f, m_rotSign * m_sub.angle * DEG2RAD, 0.0f));
        break;
    case TrickType::Backside: {
        EGG::Quatf rpy;
        rpy.setRPY(EGG::Vector3f(0.0f, -m_rotSign * 0.25f * m_sub.angle * DEG2RAD, 0.0f));
        EGG::Vector3f rot = rpy.rotateVector(EGG::Vector3f::ez);
        m_stuntRot.setAxisRotation(m_rotSign * m_sub.angle * DEG2RAD, rot);
    } break;
    case TrickType::Frontside: {
        EGG::Quatf rpy;
        rpy.setRPY(EGG::Vector3f(0.0f, 0.0f, -m_rotSign * 0.2f * m_sub.angle * DEG2RAD));
        EGG::Vector3f rot = rpy.rotateVector(EGG::Vector3f::ez);
        m_stuntRot.setAxisRotation(m_rotSign * m_sub.angle * DEG2RAD, rot);
    } break;
    case TrickType::Frontflip:
        m_stuntRot.setRPY(EGG::Vector3f(m_rotSign * m_rotSign * m_sub.angle * DEG2RAD, 0.0f, 0.0f));
        break;
    case TrickType::Backflip:
        m_stuntRot.setRPY(
                EGG::Vector3f(-m_rotSign * m_rotSign * m_sub.angle * DEG2RAD, 0.0f, 0.0f));
        break;
    default:
        break;
    }

    physics()->composeStuntRot(m_stuntRot);
}

/// @addr{0x805752E8}
void KartHalfPipe::calcLanding(bool) {
    constexpr f32 LANDING_RADIUS = 150.0f;
    constexpr f32 PREVIOUS_RADIUS = 200.0f;
    constexpr f32 MIDAIR_RADIUS = 50.0f;
    constexpr f32 IDK_RADIUS = 100.0f;

    Field::CourseColMgr::CollisionInfo colInfo;
    Field::CourseColMgr::CollisionInfo colInfo2;
    Field::KCLTypeMask maskOut;
    EGG::Vector3f pos;
    EGG::Vector3f upLocal;

    Field::KCLTypeMask mask = state()->isOverZipper() ?
            KCL_TYPE_ANY_INVISIBLE_WALL :
            KCL_TYPE_BIT(COL_TYPE_HALFPIPE_INVISIBLE_WALL);
    EGG::Vector3f up = mainRot().rotateVector(EGG::Vector3f::ey);
    EGG::Vector3f prevPos = m_prevPos + EGG::Vector3f::ey * PREVIOUS_RADIUS;

    bool hasDriverFloorCollision = move()->calcZipperCollision(LANDING_RADIUS, bsp().initialYPos,
            pos, upLocal, prevPos, &colInfo, &maskOut, KCL_TYPE_DRIVER_FLOOR);

    if (state()->isOverZipper()) {
        prevPos = hasDriverFloorCollision ? EGG::Vector3f::inf : prevPos;

        if (!move()->calcZipperCollision(MIDAIR_RADIUS, bsp().initialYPos, pos, upLocal, prevPos,
                    &colInfo2, &maskOut, mask)) {
            mask |= KCL_TYPE_DRIVER_WALL;
        }
    }

    if (move()->calcZipperCollision(IDK_RADIUS, bsp().initialYPos, pos, upLocal, prevPos, &colInfo2,
                &maskOut, mask)) {
        if (!(maskOut & ~KCL_TYPE_BIT(COL_TYPE_SPECIAL_WALL))) {}

        EGG::Vector3f up = move()->up();
        move()->setUp(up + (colInfo2.wallNrm - up) * 0.2f);
        move()->setSmoothedUp(move()->up());

        f32 yScale = bsp().initialYPos * scale().y;
        EGG::Vector3f newPos =
                pos + colInfo2.tangentOff + (-IDK_RADIUS * colInfo2.wallNrm) + yScale * upLocal;
        newPos.y += move()->hopPosY();

        dynamics()->setPos(newPos);
        move()->setDir(move()->dir().perpInPlane(move()->up(), true));
        move()->setVel1Dir(move()->dir());

        if (state()->isOverZipper()) {
            state()->setZipperStick(true);
        }

        m_prevPos = newPos;
    } else {
        if (state()->isOverZipper()) {
            state()->setZipperStick(false);
        }
    }

    if (!hasDriverFloorCollision || state()->airtime() <= 5) {
        return;
    }

    if (colInfo.floorNrm.dot(EGG::Vector3f::ey) <= 0.707f) {
        return;
    }

    if (state()->isOverZipper()) {
        state()->setZipperStick(false);
    }
}

/// @addr{0x80574E60}
void KartHalfPipe::activateTrick(s32 param_2, System::Trick trick) {
    static constexpr std::array<TrickProperties, 6> TRICK_PROPERTIES = {{
            {6.0f, 2.5f, 0.955f, 0.01f, 0.7f, 360.0f},
            {7.0f, 3.0f, 0.955f, 0.01f, 0.7f, 360.0f},
            {7.0f, 3.0f, 0.95f, 0.01f, 0.7f, 360.0f},
            {12.0f, 2.5f, 0.955f, 0.01f, 0.0f, 360.0f},
            {4.0f, 4.0f, 0.98f, 0.01f, 0.0f, 360.0f},
            {9.0f, 3.0f, 0.92f, 0.01f, 0.8f, 720.0f},
    }};

    if (param_2 < 0x33 || trick == System::Trick::None) {
        m_type = TrickType::None;
    } else {
        m_rotSign = m_nextSign;
        bool timerThreshold = param_2 > 70;

        switch (trick) {
        case System::Trick::Up:
            m_type = timerThreshold ? TrickType::Backside : TrickType::Backflip;
            break;
        case System::Trick::Down:
            m_type = timerThreshold ? TrickType::Frontside : TrickType::Frontflip;
            break;
        case System::Trick::Left:
        case System::Trick::Right:
            m_type = timerThreshold ? TrickType::Side720 : TrickType::Side360;
            m_rotSign = -1.0f;
            break;
        default:
            break;
        }

        m_sub.properties = TRICK_PROPERTIES[static_cast<s32>(m_type)];
        m_sub.finalAngle = m_sub.properties.finalAngle;
        m_sub.angleDelta = m_sub.properties.angleDelta;
        m_sub.angleDeltaFactorDecr = m_sub.properties.angleDeltaFactorDecr;
        m_sub.angle = 0.0f;
        m_sub.angleDeltaFactor = 1.0f;

        state()->setZipperTrick(true);
    }

    m_stuntRot = EGG::Quatf::ident;
}

/// @addr{0x805758E4}
void KartHalfPipe::end(bool boost) {
    if (state()->isOverZipper() && state()->airtime() > 5 && boost) {
        move()->activateZipperBoost();
    }

    if (state()->isZipperTrick()) {
        physics()->composeDecayingRot(m_stuntRot);
    }

    if (state()->isOverZipper()) {
        move()->setDir(mainRot().rotateVector(EGG::Vector3f::ez));
        move()->setVel1Dir(move()->dir());
    }

    state()->setOverZipper(false);
    state()->setZipperTrick(false);
    state()->setZipperStick(false);

    m_type = TrickType::None;
}

} // namespace Kart
