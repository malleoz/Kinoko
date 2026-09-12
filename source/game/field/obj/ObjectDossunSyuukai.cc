#include "ObjectDossunSyuukai.hh"

namespace Kinoko::Field {

/// @addr{0x80760C5C}
/// @copybrief ObjectBase::calc()
/// @details Calls the appropriate helper function (@ref calcMoving(), @ref calcRotating(), or @ref
/// ObjectDossun::calcStomp()) based on the Thwomp's current motion state.
void ObjectDossunSyuukai::calc() {
    m_touchingGround = false;

    switch (m_state) {
    case State::Moving:
        calcMoving();
        break;
    case State::RotatingBeforeStomp:
    case State::RotatingBeforeMoving:
        calcRotating();
        break;
    case State::Stomping:
        ObjectDossun::calcStomp();
        break;
    default:
        break;
    }
}

/// @addr{0x80760D8C}
/// @brief Runs once per frame while the Thwomp is rotating
/// @details Adds 5 degrees to the Thwomp's rotation about the Y-axis each frame. The Thwomp rotates
/// every frame until it reaches its target rotation. If the Thwomp is rotating before stomping and
/// reaches its target rotation, it will transition to the @ref State::Stomping state. Otherwise, if
/// the Thwomp is rotating before beginning to move and reaches its target rotation, tit will
/// transition to the @ref State::Moving state.
void ObjectDossunSyuukai::calcRotating() {
    constexpr f32 ANG_VEL = 0.08726646f; // Approximately 5 degrees
    constexpr f32 BEFORE_FALL_FRAMES = 10;

    addRot(EGG::Vector3f(0.0f, ANG_VEL, 0.0f));

    if (m_state == State::RotatingBeforeStomp) {
        f32 targetRot = m_initYaw;
        if (targetRot < 0.0f) {
            targetRot += F_TAU;
        } else if (targetRot >= F_TAU) {
            targetRot -= F_TAU;
        }

        if (targetRot < rot().y) {
            if (m_rotating) {
                subRot(EGG::Vector3f(0.0f, F_TAU, 0.0f));
            } else {
                setRot(EGG::Vector3f(rot().x, m_initYaw, rot().z));
                m_anmState = AnmState::BeforeFall;
                m_beforeFallTimer = BEFORE_FALL_FRAMES;

                m_currYaw = rot().y;
                if (m_currYaw >= F_PI) {
                    m_currYaw -= F_TAU;
                }

                m_stompDuration = m_fullDuration;
                m_state = State::Stomping;
            }
        }

        m_rotating = false;
    } else if (m_state == State::RotatingBeforeMoving) {
        const auto &curTan = m_railInterpolator->curTangentDir();
        f32 targetRot = FIDX2RAD * EGG::Mathf::Atan2FIdx(curTan.x, curTan.z);

        if (targetRot < 0.0f) {
            targetRot += F_TAU;
        } else if (targetRot >= F_TAU) {
            targetRot -= F_TAU;
        }

        if (targetRot < rot().y) {
            setRot(EGG::Vector3f(rot().x, targetRot, rot().z));
            m_state = State::Moving;
            m_rotating = true;
        }
    }
}

} // namespace Kinoko::Field
