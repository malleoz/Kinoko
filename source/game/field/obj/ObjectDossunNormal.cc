#include "ObjectDossunNormal.hh"

namespace Kinoko::Field {

/// @addr{0x80760188}
/// @brief Constructor
/// @param params The parameters used to initialize the object
ObjectDossunNormal::ObjectDossunNormal(const System::MapdataGeoObj &params)
    : ObjectDossun(params) {}

/// @addr{0x80760188}
/// @brief Default virtual destructor
ObjectDossunNormal::~ObjectDossunNormal() = default;

/// @addr{0x8076023C}
/// @copybrief ObjectBase::init()
void ObjectDossunNormal::init() {
    ObjectDossun::init();

    m_stompState = StompState::Inactive;
    m_currYaw = rot().y;

    if (m_currYaw <= F_PI) {
        m_currYaw -= F_TAU;
    }
}

/// @addr{0x807602E0}
/// @copybrief ObjectBase::calc()
void ObjectDossunNormal::calc() {
    m_touchingGround = false;

    switch (m_stompState) {
    case StompState::Inactive:
        calcInactive();
        break;
    case StompState::Active:
        calcStomp();
        break;
    default:
        break;
    }
}

/// @addr{0x80760820}
void ObjectDossunNormal::startStill() {
    m_anmState = AnmState::Still;
    m_shakePhase = 0;
    m_vel = 0.0f;
    setRot(EGG::Vector3f(rot().x, m_currYaw, rot().z));
    m_stompState = StompState::Inactive;
    m_stillTimer = static_cast<s32>(m_mapObj->setting(3));
}

/// @addr{0x80760490}
/// @brief Runs once per frame when the Thwomp is not stomping down or resetting
/// @details Causes the Thwomp to shake for 30 frames before stomping down.
void ObjectDossunNormal::calcInactive() {
    constexpr s32 SHAKE_DURATION = 30;
    constexpr s32 SHAKE_PHASE_CHANGE = 30;
    constexpr f32 SHAKE_AMPLITUDE = 30.0f;

    if (--m_stillTimer == 0) {
        startBeforeFall();
    }

    if (m_stillTimer <= SHAKE_DURATION) {
        m_shakePhase += SHAKE_PHASE_CHANGE;
        f32 posY = m_initialPosY +
                SHAKE_AMPLITUDE * EGG::Mathf::sin(static_cast<f32>(m_shakePhase) * DEG2RAD);
        setPos(EGG::Vector3f(pos().x, posY, pos().z));
    }
}

} // namespace Kinoko::Field
