#pragma once

#include "game/field/obj/ObjectCollidable.hh"

namespace Kinoko::Field {

class ObjectCrab final : public ObjectCollidable {
public:
    ObjectCrab(const System::MapdataGeoObj &params);
    ~ObjectCrab() override;

    void init() override;
    void calc() override;

    /// @addr{0x808864DC}
    [[nodiscard]] u32 loadFlags() const override {
        return 1;
    }

private:
    /// @brief Describes the current behavior of the crab
    enum class State {
        Walking = 0,
        Still = 1,
        Deactivated = 2,
        Resurfacing = 3,
    };

    /// @brief Describes the phase of the current state
    enum class StatePhase {
        Start = 0,
        Middle = 1,
        End = 2,
    };

    /// @todo Document this enum
    enum class StateResult {
        Walking = 0,
        Middle = 1,
        BeginWalking = 2,
    };

    /// @addr{0x80886438}
    /// @brief Sets the provided state and resets the phase
    void setState(State state) {
        m_state = state;
        m_statePhase = StatePhase::Start;
    }

    bool calcRail();
    StateResult calcState();

    /// @brief Sets rotation, factoring in the crab's backwards setting and rail direction
    void calcCurRot(const EGG::Vector3f &rot) {
        m_curRot = rot;
        m_curRot = m_backwards ? -m_curRot : m_curRot;
        m_curRot.y = m_railInterpolator->isMovementDirectionForward() ? m_curRot.y : -m_curRot.y;
    }

    void calcTransMat(const EGG::Vector3f &rot);

    const f32 m_vel;         ///< The speed at which the crab moves along the rail
    u32 m_stillDuration;     ///< Frames the crab will remain still at the current rail point
    u32 m_stillFrame;        ///< Frames the crab has been still at the current rail point
    bool m_still;            ///< Whether the crab is currently paused at the current rail point
    const bool m_backwards;  ///< Flips the crab's rotation
    EGG::Vector3f m_curRot;  ///< The current rotation of the crab
    State m_state;           ///< The current state of the crab: walking or still
    StatePhase m_statePhase; ///< Phase of the current state
    bool m_introCalc;        ///< Enforces only one calc call during the race intro timer

    static constexpr EGG::Vector3f INIT_ROT = EGG::Vector3f(0.0f, HALF_PI, 0.0f);
};

} // namespace Kinoko::Field
