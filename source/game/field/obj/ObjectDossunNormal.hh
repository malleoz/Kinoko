#pragma once

#include "game/field/obj/ObjectDossun.hh"

namespace Kinoko::Field {

/// @brief A Thwomp that remains in one position and stomps downward, like on GBA Bowser Castle 3
class ObjectDossunNormal final : public ObjectDossun {
public:
    /// @addr{0x80760188}
    /// @brief Constructor
    /// @param params The parameters used to initialize the object
    ObjectDossunNormal(const System::MapdataGeoObj &params) : ObjectDossun(params) {}

    /// @addr{0x80760188}
    /// @brief Default virtual destructor
    ~ObjectDossunNormal() override = default;

    /// @addr{0x8076023C}
    /// @copybrief ObjectBase::init()
    /// @details Initializes the Thwomp's state, including its stomp state and rotation.
    void init() override {
        ObjectDossun::init();

        m_stompState = StompState::Inactive;
        m_currYaw = rot().y;

        if (m_currYaw <= F_PI) {
            m_currYaw -= F_TAU;
        }
    }

    /// @addr{0x807602E0}
    /// @copybrief ObjectBase::calc()
    void calc() override {
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
    void startStill() override {
        m_anmState = AnmState::Still;
        m_shakePhase = 0;
        m_vel = 0.0f;
        setRot(EGG::Vector3f(rot().x, m_currYaw, rot().z));
        m_stompState = StompState::Inactive;
        m_stillTimer = static_cast<s32>(m_mapObj->setting(3));
    }

    /// @addr{0x80760964}
    /// @brief Runs once when the Thwomp begins rising before stomping down
    void startBeforeFall() {
        m_stompState = StompState::Active;
        m_anmState = AnmState::BeforeFall;
        m_beforeFallTimer = static_cast<s32>(BEFORE_FALL_DURATION);
        m_stompDuration = static_cast<s32>(m_fullDuration);
    }

private:
    /// @addr{0x80760490}
    /// @brief Runs once per frame when the Thwomp is not stomping down or resetting
    /// @details Causes the Thwomp to shake for 30 frames before stomping down with an amplitude of
    /// `30.0f`.
    void calcInactive() {
        constexpr s32 SHAKE_DURATION = 30;
        constexpr f32 SHAKE_AMPLITUDE = 30.0f;

        if (--m_stillTimer == 0) {
            startBeforeFall();
        }

        if (m_stillTimer <= SHAKE_DURATION) {
            m_shakePhase += SHAKE_DURATION;
            f32 posY = m_initialPosY +
                    SHAKE_AMPLITUDE * EGG::Mathf::sin(static_cast<f32>(m_shakePhase) * DEG2RAD);
            setPos(EGG::Vector3f(pos().x, posY, pos().z));
        }
    }
};

} // namespace Kinoko::Field
