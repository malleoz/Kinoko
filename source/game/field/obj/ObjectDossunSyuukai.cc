#include "ObjectDossunSyuukai.hh"

namespace Field {

/// @addr{0x80760B20}
ObjectDossunSyuukai::ObjectDossunSyuukai(const System::MapdataGeoObj &params)
    : ObjectDossun(params) {}

/// @addr{0x80764B88}
ObjectDossunSyuukai::~ObjectDossunSyuukai() = default;

/// @addr{0x80760BD4}
void ObjectDossunSyuukai::init() {
    ObjectDossun::init();

    m_state = State::Moving;
    m_initRotY = m_rot.y;
    m_rotating = true;
}

/// @addr{0x80760C5C}
void ObjectDossunSyuukai::calc() {
    m_touchingGround = false;

    switch (m_state) {
    case State::Moving: {
        if (m_railInterpolator->calc() == RailInterpolator::Status::SegmentEnd) {
            m_state = State::RotatingBeforeStomp;
        }

        m_pos = m_railInterpolator->curPos();
        m_flags.setBit(eFlags::Position);
    } break;
    case State::RotatingBeforeStomp:
    case State::RotatingAfterStomp: {
        calcRotating();
    } break;
    case State::Stomping: {
        ObjectDossun::calcStomp();
    } break;
    default:
        break;
    }
}

/// @addr{0x8076139C}
void ObjectDossunSyuukai::startStill() {
    ObjectDossun::startStill();
    m_state = State::RotatingAfterStomp;
}

/// @addr{0x80760D8C}
void ObjectDossunSyuukai::calcRotating() {
    constexpr f32 ANG_VEL = 0.08726646f; /// Approximately 5 degrees

    m_flags.setBit(eFlags::Rotation);
    m_rot.y += ANG_VEL;

    if (m_state == State::RotatingBeforeStomp) {
        f32 dVar6 = m_initRotY;
        if (dVar6 < 0.0f) {
            dVar6 += F_TAU;
        } else if (dVar6 >= F_TAU) {
            dVar6 -= F_TAU;
        }

        if (dVar6 < m_rot.y) {
            m_flags.setBit(eFlags::Rotation);

            if (m_rotating) {
                m_rot.y -= F_TAU;
            } else {
                m_rot.y = m_initRotY;
                m_anmState = MoveState::BeforeFall;
                m_beforeFallTimer = 10;

                m_currRot = m_rot.y;
                if (m_currRot >= F_PI) {
                    m_currRot -= F_TAU;
                }

                m_cycleTimer = m_fullDuration;
                m_state = State::Stomping;
            }
        }

        m_rotating = false;
    } else if (m_state == State::RotatingAfterStomp) {
        const auto &curTan = m_railInterpolator->curTangentDir();
        f32 angle = FIDX2RAD * EGG::Mathf::Atan2FIdx(curTan.x, curTan.z);

        if (angle < 0.0f) {
            angle += F_TAU;
        } else if (angle >= F_TAU) {
            angle -= F_TAU;
        }

        if (angle < m_rot.y) {
            m_flags.setBit(eFlags::Rotation);
            m_rot.y = angle;
            m_state = State::Moving;
            m_rotating = true;
        }
    }
}

} // namespace Field
