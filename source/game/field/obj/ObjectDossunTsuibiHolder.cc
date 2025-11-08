#include "ObjectDossunTsuibiHolder.hh"

#include "game/field/obj/ObjectDossunTsuibi.hh"

namespace Field {

/// @addr{0x807614D0}
ObjectDossunTsuibiHolder::ObjectDossunTsuibiHolder(const System::MapdataGeoObj &params)
    : ObjectCollidable(params) {
    for (auto &dossun : m_dossuns) {
        dossun = new ObjectDossunTsuibi(params, this);
        dossun->load();
    }

    m_stillTimer = static_cast<u32>(params.setting(2));
    m_facingBackwards = false;
}

/// @addr{0x80764BC8}
ObjectDossunTsuibiHolder::~ObjectDossunTsuibiHolder() = default;

/// @addr{0x80761744}
void ObjectDossunTsuibiHolder::init() {
    for (auto *&dossun : m_dossuns) {
        dossun->init();
    }

    m_initPos = m_pos;
    m_initRotY = m_rot.y;
    m_state = State::Still;
    m_railInterpolator->init(0.0f, 0);
    m_vel = m_railInterpolator->currVel();
    m_forwardTimer = 0;
    m_movingForward = false;
    m_movingSideways = false;

    m_flags.setBit(eFlags::Position);
    m_dossuns[0]->setPos(EGG::Vector3f(m_pos.x, m_pos.y, m_pos.z + DOSSUN_POS_OFFSET));
    m_dossuns[1]->setPos(EGG::Vector3f(m_pos.x, m_pos.y, m_pos.z - DOSSUN_POS_OFFSET));
    m_lastStompZ = m_pos.z;
    m_flipSideways = 1;
}

/// @addr{0x8076198C}
void ObjectDossunTsuibiHolder::calc() {
    for (auto &dossun : m_dossuns) {
        dossun->calc();
    }

    switch (m_state) {
    case State::Still:
        if (--m_stillTimer == 0) {
            m_state = State::Forward;
            m_movingSideways = false;
        }

        break;
    case State::Forward:
        FUN_807624F0();
        FUN_807625F0();
        break;
    case State::StartStomp: {
        for (auto *&dossun : m_dossuns) {
            // TODO: Check if this is an inline
            dossun->m_anmState = ObjectDossun::MoveState::BeforeFall;
            dossun->m_beforeFallTimer = 10;
            f32 rot = dossun->m_rot.y;
            dossun->m_currRot = rot;

            if (rot >= F_PI) {
                dossun->m_currRot = rot - F_TAU;
            }

            dossun->m_cycleTimer = dossun->m_fullDuration;
        }

        m_state = State::Stomping;
    } break;
    case State::Stomping:
        for (auto *&dossun : m_dossuns) {
            dossun->calcStomp();
        }

        FUN_807625F0();

        break;
    case State::Backward:
        if (m_railInterpolator->calc() == RailInterpolator::Status::ChangingDirection) {
            m_resetZVel = (m_lastStompZ - m_railInterpolator->curPos().z) /
                    static_cast<f32>(HOME_RESET_FRAMES);

            m_resetAngVel = m_facingBackwards ? 5.0f : 10.0f;
            m_backwardsCounter = 0;
            m_state = State::SillRotating;
            m_facingBackwards = false;
            m_railInterpolator->init(0.0f, 0);
        }

        m_flags.setBit(eFlags::Position);
        m_pos.z = m_lastStompZ;

        m_dossuns[0]->setPos(EGG::Vector3f(m_pos.x, m_pos.y, m_pos.z + DOSSUN_POS_OFFSET));
        m_dossuns[1]->setPos(EGG::Vector3f(m_pos.x, m_pos.y, m_pos.z - DOSSUN_POS_OFFSET));

        FUN_807625F0();

        break;
    case State::SillRotating:
        m_flags.setBit(eFlags::Position);
        m_pos.z -= m_resetZVel;

        m_dossuns[0]->setPos(EGG::Vector3f(m_pos.x, m_pos.y, m_pos.z + DOSSUN_POS_OFFSET));
        m_dossuns[1]->setPos(EGG::Vector3f(m_pos.x, m_pos.y, m_pos.z - DOSSUN_POS_OFFSET));

        FUN_807625F0();

        break;
    }
}

/// @addr{0x807624F0}
void ObjectDossunTsuibiHolder::FUN_807624F0() {
    ++m_forwardTimer;

    if (m_movingForward) {
        FUN_8076321C();
    } else if (m_forwardTimer == 45) {
        m_movingForward = true;
    }

    if (m_movingSideways) {
        FUN_807634C0();
    }

    if (m_railInterpolator->curPoint().setting[1] == 1 && !m_facingBackwards) {
        m_facingBackwards = true;
        m_backwardsCounter = 0;
    }
}

/// @addr{0x807625F0}
void ObjectDossunTsuibiHolder::FUN_807625F0() {
    constexpr f32 DEGREES_5_RAD = DEG2RAD * 5.0f;
    STATIC_ASSERT(DEGREES_5_RAD == 0.08726646f);

    if (m_facingBackwards) {
        ++m_backwardsCounter;
        if (m_backwardsCounter == HOME_RESET_FRAMES) {
            m_flags.setBit(eFlags::Rotation);
            m_rot.y += F_PI;

            m_dossuns[0]->m_flags.setBit(eFlags::Rotation);
            m_dossuns[0]->m_rot.y = m_rot.y;
            m_dossuns[1]->m_flags.setBit(eFlags::Rotation);
            m_dossuns[1]->m_rot.y = m_rot.y;

            m_movingSideways = true;
            m_sidewaysPhase = 0;

            if (m_flipSideways == 0) {
                m_flipSideways = 1;
            } else {
                m_flipSideways = 0;
            }
        } else if (m_backwardsCounter < HOME_RESET_FRAMES) {
            m_flags.setBit(eFlags::Rotation);
            m_rot.y += DEGREES_5_RAD;

            m_dossuns[0]->m_flags.setBit(eFlags::Rotation);
            m_dossuns[0]->m_rot.y = m_rot.y;
            m_dossuns[1]->m_flags.setBit(eFlags::Rotation);
            m_dossuns[1]->m_rot.y = m_rot.y;
        }
    } else if (m_state == State::SillRotating) {
        m_flags.setBit(eFlags::Rotation);
        m_rot.y += m_resetAngVel * DEG2RAD;

        m_dossuns[0]->m_flags.setBit(eFlags::Rotation);
        m_dossuns[0]->m_rot.y = m_rot.y;
        m_dossuns[1]->m_flags.setBit(eFlags::Rotation);
        m_dossuns[1]->m_rot.y = m_rot.y;

        if (++m_backwardsCounter == HOME_RESET_FRAMES) {
            m_flags.setBit(eFlags::Rotation);
            m_rot.y = m_initRotY;

            m_dossuns[0]->m_flags.setBit(eFlags::Rotation);
            m_dossuns[0]->m_rot.y = m_rot.y;
            m_dossuns[1]->m_flags.setBit(eFlags::Rotation);
            m_dossuns[1]->m_rot.y = m_rot.y;

            m_state = State::Still;
            m_stillTimer = static_cast<u32>(m_mapObj->setting(3));
        }
    }
}

/// @addr{0x8076321C}
void ObjectDossunTsuibiHolder::FUN_8076321C() {
    m_railInterpolator->setCurrVel(static_cast<f32>(m_mapObj->setting(0)));

    if (m_railInterpolator->calc() == RailInterpolator::Status::ChangingDirection) {
        m_state = State::StartStomp;
        m_forwardTimer = 0;
        m_movingForward = false;
        m_lastStompZ = m_pos.z;
    }

    m_pos = m_railInterpolator->curPos();
    m_flags.setBit(eFlags::Position);
    m_dossuns[0]->setPos(EGG::Vector3f(m_pos.x, m_pos.y, m_pos.z + DOSSUN_POS_OFFSET));
    m_dossuns[1]->setPos(EGG::Vector3f(m_pos.x, m_pos.y, m_pos.z - DOSSUN_POS_OFFSET));
}

/// @addr{0x807634C0}
void ObjectDossunTsuibiHolder::FUN_807634C0() {
    constexpr f32 AMPLITUDE = 1500.0f;

    m_sidewaysPhase += 2;

    m_flags.setBit(eFlags::Position);
    u32 phase = m_flipSideways ? m_sidewaysPhase + 180 : m_sidewaysPhase;
    m_pos.z += AMPLITUDE * EGG::Mathf::sin(static_cast<f32>(phase) * DEG2RAD);

    m_dossuns[0]->setPos(EGG::Vector3f(m_pos.x, m_pos.y, m_pos.z + DOSSUN_POS_OFFSET));
    m_dossuns[1]->setPos(EGG::Vector3f(m_pos.x, m_pos.y, m_pos.z - DOSSUN_POS_OFFSET));

    if (m_sidewaysPhase == 170) {
        if (m_state != State::StartStomp) {
            m_railInterpolator->reverseDirection();
        }

        m_state = State::StartStomp;
        m_forwardTimer = 0;
        m_movingForward = false;
        m_lastStompZ = m_pos.z;
    }
}

} // namespace Field
