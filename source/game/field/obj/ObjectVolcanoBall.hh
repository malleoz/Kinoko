#pragma once

#include "game/field/StateManager.hh"
#include "game/field/obj/ObjectCollidable.hh"

namespace Kinoko::Field {

/// @brief Represents a fireball that is launched from the volcanoes on Grumble Volcano
/// @details Lifecycle and management of these objects is performed by @ref
/// ObjectVolcanoBallLauncher. The fireball is launched with a given initial velocity and has
/// constant acceleration. Once the ball reaches the end of its rail, it will transition to the
/// burning state. It will become intangible after the burning duration has elapsed.
class ObjectVolcanoBall final : public ObjectCollidable, private StateManager {
    /// @brief Grants the launcher class access to the fireball's state
    friend class ObjectVolcanoBallLauncher;

public:
    /// @addr{0x806E2904}
    /// @copybrief ObjectCollidable::ObjectCollidable(const System::MapdataGeoObj &)
    /// @param accel The acceleration of the ball
    /// @param finalVel The final velocity of the ball at impact
    /// @param endPosY The end position along the Y-axis
    /// @param params The parameters used to initialize the object
    /// @param vel The initial velocity of the ball
    /// @details Initializes @ref m_burnDuration based on param setting 4. Sets @ref m_accel, @ref
    /// m_finalVelSq, and @ref m_endPosY based on the `accel`, `finalVel`, and `endPosY` arguments
    /// respectively. Finally, computes the squared XZ-plane velocity and saves it to @ref
    /// m_sqVelXZ.
    ObjectVolcanoBall(f32 accel, f32 finalVel, f32 endPosY, const System::MapdataGeoObj &params,
            const EGG::Vector3f &vel)
        : ObjectCollidable(params),
          StateManager(this, STATE_ENTRIES),
          m_burnDuration(params.setting(3)),
          m_accel(accel),
          m_finalVelSq(finalVel),
          m_endPosY(endPosY),
          m_sqVelXZ(vel.x * vel.x + vel.z * vel.z) {}

    /// @addr{0x806E2BE0}
    /// @brief Default virtual destructor
    ~ObjectVolcanoBall() override = default;

    /// @addr{0x806E2C4C}
    /// @copybrief ObjectBase::init()
    /// @details Initializes the rail interpolator to the start of the rail and sets the fireball's
    /// position accordingly.
    void init() override {
        m_railInterpolator->init(0.0f, 0);
        setPos(m_railInterpolator->curPos());
    }

    /// @addr{0x806E2E08}
    /// @copybrief ObjectBase::calc()
    /// @details Simply evaluates the fireball's state machine.
    void calc() override {
        StateManager::calc();
    }

    /// @addr{0x806E3A7C}
    /// @copybrief ObjectBase::loadFlags()
    /// @return Returns @ref eLoadFlags::Calc, so that the object is calculated every frame.
    [[nodiscard]] LoadFlags loadFlags() const override {
        return LoadFlags(eLoadFlags::Calc);
    }

private:
    /// @addr{0x806E2F24}
    /// @brief Runs when the fireball is despawned/intangible.
    /// @details Simply calls @ref init() to reinitialize the fireball to its initial state.
    void enterDormant() {
        init();
    }

    /// @addr{0x806E2F38}
    /// @brief Runs when the fireball starts falling.
    /// @details Simply calls @ref init() to reinitialize the fireball to its initial state.
    /// @todo Since this is identical to the behavior of @ref enterDormant() and there is no
    /// `calcDormant()` function, we can likely omit this function for Kinoko.
    void enterFalling() {
        init();
    }

    /// @addr{0x806E3034}
    /// @brief Calculates the ball's velocity and updates its position along the rail
    /// @details Derives the ball's velocity based on the
    /// kinematic equation:
    /// \f[
    /// v^2 = v_f^2 - 2a(y - y_{end})
    /// \f]
    /// where \f$v_f^2\f$ is @ref m_finalVelSq, \f$a\f$ is @ref m_accel,
    /// \f$y\f$ is the Y-component of @ref pos(), and \f$y_{end}\f$ is @ref m_endPosY.
    /// If the fireball has reaches the end of its rail, then it transitions to the burning state.
    /// Otherwise, the fireball's transformation matrix and position are
    void calcFalling() {
        f32 sqVel = std::max(0.01f, m_finalVelSq - 2.0f * m_accel * (pos().y - m_endPosY));
        m_railInterpolator->setSpeed(EGG::Mathf::sqrt(m_sqVelXZ + sqVel));

        if (m_railInterpolator->calc() == RailInterpolator::Status::ChangingDirection) {
            m_nextStateId = 2;
        } else {
            EGG::Vector3f tangent = m_railInterpolator->curTangentDir();
            if (EGG::Mathf::abs(tangent.y) > 0.1f) {
                tangent.y = 0.01f;
            }

            tangent.normalise2();
            setMatrixFromOrthonormalBasisAndPos(tangent);

            setPos(m_railInterpolator->curPos());
        }
    }

    /// @addr{0x806E3324}
    /// @brief Runs while the fireball is on the ground and burning.
    /// @details Transitions the fireball to the dormant state once @ref m_burnDuration have
    /// elapsed.
    void calcBurning() {
        if (m_currentFrame >= m_burnDuration) {
            m_nextStateId = 0;
        }
    }

    const u16 m_burnDuration; ///< How long the ball burns for before disappearing
    const f32 m_accel;        ///< The acceleration of the ball along the rail when it is falling
    const f32 m_finalVelSq;   ///< Velocity of the ball at the moment of impact
    const f32 m_endPosY;      ///< Height of the ball at the end of its rail
    const f32 m_sqVelXZ;      ///< Squared X-Z plane velocity

    /// @brief The enter and calc functions for each @ref StateManager entry
    static constexpr std::array<StateManagerEntry, 3> STATE_ENTRIES = {{
            {StateEntry<ObjectVolcanoBall, &ObjectVolcanoBall::enterDormant, nullptr>(0)},
            {StateEntry<ObjectVolcanoBall, &ObjectVolcanoBall::enterFalling,
                    &ObjectVolcanoBall::calcFalling>(1)},
            {StateEntry<ObjectVolcanoBall, nullptr, &ObjectVolcanoBall::calcBurning>(2)},
    }};
};

} // namespace Kinoko::Field
