#pragma once

#include "game/field/ObjectCollisionCylinder.hh"
#include "game/field/StateManager.hh"
#include "game/field/obj/ObjectCollidable.hh"

namespace Kinoko::Field {

/// @brief Oscillating cars on Coconut Mall
/// @details These cars move along a rail and have a trapezoidal motion profile; they accelerate to
/// a set velocity, drive at that velocity for a set amount of time, and then decelerate to a stop.
class ObjectCarA : public ObjectCollidable, private StateManager {
public:
    ObjectCarA(const System::MapdataGeoObj &params);
    ~ObjectCarA() override;

    void init() override;

    /// @addr{0x806B82CC}
    void calc() override {
        StateManager::calc();
        calcRail();
        calcPos();
    }

    /// @addr{0x806B8F44}
    [[nodiscard]] u32 loadFlags() const override {
        return 1;
    }

    /// @addr{0x806B7B44}
    void createCollision() override {
        constexpr f32 RADIUS = 210.0f;
        constexpr f32 HEIGHT = 200.0f;

        m_collision = EGG::egg_new<ObjectCollisionCylinder>(RADIUS, HEIGHT, collisionCenter());
    }

    void calcCollisionTransform() override;

    Kart::Reaction onCollision(Kart::KartObject *kartObj, Kart::Reaction reactionOnKart,
            Kart::Reaction reactionOnObj, EGG::Vector3f &hitDepth) override;

private:
    // Trapezoidal motion profile
    enum class MotionState {
        Accelerating = 0,
        Cruising = 1,
        Decelerating = 2,
    };

    /// @addr{0x806B8CCC}
    /// @brief Updates the rail and checks if the car is changing direction
    void calcRail() {
        m_railInterpolator->setCurrVel(m_currVel);

        auto status = m_railInterpolator->calc();
        m_changingDir = (status == RailInterpolator::Status::ChangingDirection);
    }

    /// @addr{0x806B8D3C}
    /// @brief Helper function that updates the car's position based on the rail interpolator
    void calcPos() {
        m_currUp = Interpolate(0.1f, m_currUp, EGG::Vector3f::ey);
        m_currUp.normalise2();
        setMatrixTangentTo(m_currUp, m_currTangent);
        setPos(m_railInterpolator->curPos());
    }

    /// @addr{0x806B84FC}
    /// @brief Runs once when the car has entered the stop state
    void enterStop() {
        m_currVel = 0.0f;
    }

    /// @brief Runs once when the car starts accelerating
    void enterAccel() {}

    /// @addr{0x806B8838}
    /// @brief Runs once when the car has entered the cruising state
    void enterCruising() {
        m_currVel = m_finalVel;
    }

    /// @addr{0x806B8588}
    /// @brief Runs once per frame when the car is in the stop state
    void calcStop() {
        if (m_currentFrame > m_stopTime) {
            m_motionState = MotionState::Accelerating;
            m_currentStateId = 1;
        }
    }

    void calcAccel();

    /// @addr{0x806B8844}
    /// @brief Runs once per frame when the car is in the cruising state
    void calcCruising() {
        // We might've had decimals, better to undershoot the cruising time and handle it in decel
        if (static_cast<f32>(m_currentFrame) > m_cruiseTime - 1.0f) {
            m_motionState = MotionState::Decelerating;
            m_nextStateId = 1;
        }
    }

    const f32 m_finalVel;        ///< Target velocity after accelerating
    const f32 m_accel;           ///< Acceleration and deceleration rate
    const u32 m_stopTime;        ///< How long to spend at 0 velocity before accelerating.
    f32 m_cruiseTime;            ///< How long to spend at cruising speed before decelerating.
    EGG::Vector3f m_currTangent; ///< It's EGG::Vector3f::ey unless it flies up in the air.
    EGG::Vector3f m_currUp;      ///< It's EGG::Vector3f::ey unless it flies up in the air.
    f32 m_currVel;               ///< Current velocity of the car this frame
    MotionState m_motionState;   ///< The current motion state of the car
    bool m_changingDir;          ///< Triggers the deceleration-to-stop logic

    static constexpr std::array<StateManagerEntry, 3> STATE_ENTRIES = {{
            {StateEntry<ObjectCarA, &ObjectCarA::enterStop, &ObjectCarA::calcStop>(0)},
            {StateEntry<ObjectCarA, &ObjectCarA::enterAccel, &ObjectCarA::calcAccel>(1)},
            {StateEntry<ObjectCarA, &ObjectCarA::enterCruising, &ObjectCarA::calcCruising>(2)},
    }};
};

} // namespace Kinoko::Field
