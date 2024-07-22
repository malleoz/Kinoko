#include "KartHalfPipe.hh"

#include "game/kart/KartDynamics.hh"
#include "game/kart/KartMove.hh"
#include "game/kart/KartParam.hh"
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

    if (m_touchingZipper && state()->isAirStart()) {
        dynamics()->setExtVel(EGG::Vector3f::zero);
        state()->setOverZipper(true);

        EGG::Vector3f velNorm = velocity();
        velNorm.normalise();
        EGG::Vector3f rot = dynamics()->mainRot().rotateVectorInv(velNorm);
        m_rot.makeVectorRotation(rot, EGG::Vector3f::ez);
        m_prevPos = prevPos();

        calcLanding(false);
    } else {
        if (!state()->isOverZipper()) {
            if (state()->isHalfpipeRamp()) {
                calcLanding(true);
            }
        } else {
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

            calcLanding(false);
        }
    }

    m_timer = std::max(0, m_timer - 1);

    m_touchingZipper = bVar14;
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

/// @addr{0x805758E4}
void KartHalfPipe::end(bool boost) {
    if (state()->isOverZipper()) {
        if (state()->airtime() > 5 && boost) {
            move()->activateZipperBoost();
        }

        move()->setDir(mainRot().rotateVector(EGG::Vector3f::ez));
        move()->setVel1Dir(move()->dir());
    }

    state()->setOverZipper(false);
    state()->setZipperStick(false);
}

} // namespace Kart
