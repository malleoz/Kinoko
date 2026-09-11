#pragma once

#include "game/field/obj/ObjectDossun.hh"

namespace Kinoko::Field {

/// @brief A Thwomp that remains in one position and stomps downward, like on GBA Bowser Castle 3
class ObjectDossunNormal final : public ObjectDossun {
public:
    /// @addr{0x80760188}
    /// @brief Constructor
    /// @param params The parameters used to initialize the object
    /// @details Caches the Thwomp's "still" duration to @ref m_stillDuration based on param
    /// setting 4.
    ObjectDossunNormal(const System::MapdataGeoObj &params)
        : ObjectDossun(params),
          m_stillDuration(static_cast<s32>(params.setting(3))) {}

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
    /// @details Clears the @ref m_touchingGround flag. Then, based on whether the Thwomp is
    /// currently stomping or not, either calls @ref calcStomp() or @ref calcInactive().
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
    /// @copybrief ObjectDossun::startStill()
    /// @details Calls @ref ObjectDossun::startStill() to initialize the Thwomp at the beginning of
    /// the still state. Also resets the stomp state to @ref StompState::Inactive and sets the @ref
    /// m_stillTimer to @ref m_stillDuration.
    void startStill() override {
        ObjectDossun::startStill();
        m_stompState = StompState::Inactive;
        m_stillTimer = m_stillDuration;
    }

    /// @addr{0x80760964}
    /// @brief Runs once when the Thwomp begins rising before stomping down
    /// @details Sets the stomp state to @ref StompState::Active. Sets the animation state to @ref
    /// AnmState::BeforeFall. Initializes @ref m_beforeFallTimer to @ref BEFORE_FALL_DURATION, and
    /// sets @ref m_stompDuration to @ref m_fullDuration.
    void startBeforeFall() {
        m_stompState = StompState::Active;
        m_anmState = AnmState::BeforeFall;
        m_beforeFallTimer = static_cast<s32>(BEFORE_FALL_DURATION);
        m_stompDuration = static_cast<s32>(m_fullDuration);
    }

private:
    /// @addr{0x80760490}
    /// @brief Runs once per frame when the Thwomp is not stomping down or resetting
    /// @details Causes the Thwomp to shake for 30 frames with an amplitude of `30.0f` before
    /// stomping down.
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

    /// @brief Number of frames the Thwomp remains still for
    /// @details This member does not exist in the base game, but we cache it here to avoid
    /// repeatedly fetching the param settings.
    const s32 m_stillDuration;
};

} // namespace Kinoko::Field
