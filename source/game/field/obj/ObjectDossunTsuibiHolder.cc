#include "ObjectDossunTsuibiHolder.hh"

#include "game/field/obj/ObjectDossunTsuibi.hh"

namespace Kinoko::Field {

/// @addr{0x807614D0}
ObjectDossunTsuibiHolder::ObjectDossunTsuibiHolder(const System::MapdataGeoObj &params)
    : ObjectCollidable(params), m_stillTimer(static_cast<u32>(params.setting(2))),
      m_facingBackwards(false), m_forwardVel(static_cast<f32>(m_mapObj->setting(0))),
      m_stillDuration(static_cast<u32>(m_mapObj->setting(3))) {
    for (auto *&dossun : m_dossuns) {
        dossun = EGG::egg_new<ObjectDossunTsuibi>(params, this);
        dossun->load();
    }
}

/// @addr{0x80764BC8}
ObjectDossunTsuibiHolder::~ObjectDossunTsuibiHolder() = default;

/// @addr{0x80761744}
void ObjectDossunTsuibiHolder::init() {
    for (auto *&dossun : m_dossuns) {
        dossun->init();
    }

    m_initPos = pos();
    m_initYaw = rot().y;
    m_state = State::Still;
    m_railInterpolator->init(0.0f, 0);
    m_vel = m_railInterpolator->currVel();
    m_forwardTimer = 0;
    m_movingForward = false;
    m_movingSideways = false;

    updatePos(pos());

    m_lastStompZ = pos().z;
    m_flipSideways = 1;
}

/// @addr{0x8076198C}
void ObjectDossunTsuibiHolder::calc() {
    for (auto &dossun : m_dossuns) {
        dossun->calc();
    }

    switch (m_state) {
    case State::Still:
        calcStill();
        break;
    case State::Forward:
        calcForward();
        calcRot();
        break;
    case State::StartStomp:
        calcStartStomp();
        break;
    case State::Stomping:
        calcStomp();
        calcRot();

        break;
    case State::Backward:
        calcBackwards();
        calcRot();

        break;
    case State::StillRotating:
        calcResetZ();
        calcRot();

        break;
    }
}

/// @addr{0x807624F0}
/// @brief Runs every frame while the Thwomps are moving forward down the hallway
void ObjectDossunTsuibiHolder::calcForward() {
    constexpr u32 FORWARD_DELAY_FRAMES = 45;

    ++m_forwardTimer;

    if (m_movingForward) {
        calcForwardRail();
    } else if (m_forwardTimer == FORWARD_DELAY_FRAMES) {
        m_movingForward = true;
    }

    if (m_movingSideways) {
        calcForwardOscillation();
    }

    if (m_railInterpolator->curPoint().setting[1] == 1 && !m_facingBackwards) {
        m_facingBackwards = true;
        m_backwardsCounter = 0;
    }
}

/// @brief Runs once when the Thwomps begin to stomp
void ObjectDossunTsuibiHolder::calcStartStomp() {
    for (auto *&dossun : m_dossuns) {
        dossun->m_anmState = ObjectDossun::AnmState::BeforeFall;
        dossun->m_beforeFallTimer = 10;
        f32 rot = dossun->rot().y;
        dossun->m_currYaw = rot;

        if (rot >= F_PI) {
            dossun->m_currYaw = rot - F_TAU;
        }

        dossun->m_stompDuration = dossun->m_fullDuration;
    }

    m_state = State::Stomping;
}

/// @brief Runs every frame while the Thwomps are stomping downwards
void ObjectDossunTsuibiHolder::calcStomp() {
    for (auto *&dossun : m_dossuns) {
        dossun->calcStomp();
    }
}

/// @addr{0x80762EEC}
/// @brief Runs every frame while the Thwomps are moving backwards up the hallway towards home
void ObjectDossunTsuibiHolder::calcBackwards() {
    if (m_railInterpolator->calc() == RailInterpolator::Status::ChangingDirection) {
        m_resetZVel = (m_lastStompZ - m_railInterpolator->curPos().z) /
                static_cast<f32>(HOME_RESET_FRAMES);

        m_resetAngVel = m_facingBackwards ? 5.0f : 10.0f;
        m_backwardsCounter = 0;
        m_state = State::StillRotating;
        m_facingBackwards = false;
        m_railInterpolator->init(0.0f, 0);
    }

    updatePos(EGG::Vector3f(pos().x, pos().y, m_lastStompZ));
}

/// @addr{0x807625F0}
/// @brief Runs every frame in order to update the Thwomps' rotation
void ObjectDossunTsuibiHolder::calcRot() {
    constexpr f32 DEGREES_5_RAD = DEG2RAD * 5.0f;
    STATIC_ASSERT(DEGREES_5_RAD == 0.08726646f);

    if (m_facingBackwards) {
        if (++m_backwardsCounter == HOME_RESET_FRAMES) {
            updateRot(rot().y + F_PI);

            m_movingSideways = true;
            m_sidewaysPhase = 0;

            if (m_flipSideways == 0) {
                m_flipSideways = 1;
            } else {
                m_flipSideways = 0;
            }
        } else if (m_backwardsCounter < HOME_RESET_FRAMES) {
            updateRot(rot().y + DEGREES_5_RAD);
        }
    } else if (m_state == State::StillRotating) {
        updateRot(rot().y + m_resetAngVel * DEG2RAD);

        if (++m_backwardsCounter == HOME_RESET_FRAMES) {
            updateRot(m_initYaw);

            m_state = State::Still;
            m_stillTimer = m_stillDuration;
        }
    }
}

/// @addr{0x8076321C}
/// @brief Updates position from the rail every frame while the Thwomps are moving forward
void ObjectDossunTsuibiHolder::calcForwardRail() {
    m_railInterpolator->setCurrVel(m_forwardVel);

    if (m_railInterpolator->calc() == RailInterpolator::Status::ChangingDirection) {
        m_state = State::StartStomp;
        m_forwardTimer = 0;
        m_movingForward = false;
        m_lastStompZ = pos().z;
    }

    updatePos(m_railInterpolator->curPos());
}

/// @addr{0x807634C0}
/// @brief Calculates sideways oscillation of the Thwomps while moving forward down the hallway
void ObjectDossunTsuibiHolder::calcForwardOscillation() {
    constexpr f32 AMPLITUDE = 1500.0f;
    constexpr f32 STOMP_PHASE = 170.0f;

    m_sidewaysPhase += 2;

    u32 phase = m_flipSideways ? m_sidewaysPhase + 180 : m_sidewaysPhase;
    f32 posOffsetZ = AMPLITUDE * EGG::Mathf::sin(static_cast<f32>(phase) * DEG2RAD);
    updatePos(EGG::Vector3f(pos().x, pos().y, pos().z + posOffsetZ));

    if (m_sidewaysPhase == STOMP_PHASE) {
        if (m_state != State::StartStomp) {
            m_railInterpolator->reverseDirection();
        }

        m_state = State::StartStomp;
        m_forwardTimer = 0;
        m_movingForward = false;
        m_lastStompZ = pos().z;
    }
}

/// @addr{0x80762054}
/// @brief Synchronizes the position of the Thwomps based off the provided position
void ObjectDossunTsuibiHolder::updatePos(const EGG::Vector3f &pos) {
    setPos(pos);
    m_dossuns[0]->setPos(EGG::Vector3f(pos.x, pos.y, pos.z + DOSSUN_POS_OFFSET));
    m_dossuns[1]->setPos(EGG::Vector3f(pos.x, pos.y, pos.z - DOSSUN_POS_OFFSET));
}

/// @addr{0x80762190}
/// @brief Synchronizes the rotation of the Thwomps based off the provided yaw
void ObjectDossunTsuibiHolder::updateRot(f32 yaw) {
    setRot(EGG::Vector3f(rot().x, yaw, rot().z));

    const auto &firstRot = m_dossuns[0]->rot();
    const auto &secondRot = m_dossuns[1]->rot();
    m_dossuns[0]->setRot(EGG::Vector3f(firstRot.x, rot().y, firstRot.z));
    m_dossuns[1]->setRot(EGG::Vector3f(secondRot.x, rot().y, secondRot.z));
}

} // namespace Kinoko::Field
