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
    ObjectVolcanoBall(f32 accel, f32 finalVel, f32 endPosY, const System::MapdataGeoObj &params,
            const EGG::Vector3f &vel);
    ~ObjectVolcanoBall() override;

    /// @addr{0x806E2C4C}
    /// @copybrief ObjectBase::init()
    void init() override {
        m_railInterpolator->init(0.0f, 0);
        setPos(m_railInterpolator->curPos());
    }

    /// @addr{0x806E2E08}
    /// @copybrief ObjectBase::calc()
    void calc() override {
        StateManager::calc();
    }

    /// @addr{0x806E3A7C}
    /// @copybrief ObjectBase::loadFlags()
    /// @return Returns @ref eLoadFlags::Calc, so that object is calculated every frame.
    [[nodiscard]] LoadFlags loadFlags() const override {
        return LoadFlags(eLoadFlags::Calc);
    }

private:
    /// @addr{0x806E2F24}
    void enterDormant() {
        init();
    }

    /// @addr{0x806E2F38}
    void enterFalling() {
        init();
    }

    void calcFalling();

    /// @addr{0x806E3324}
    void calcBurning() {
        if (m_currentFrame >= m_burnDuration) {
            m_nextStateId = 0;
        }
    }

    const u16 m_burnDuration; ///< How long the ball burns for before disappearing
    const f32 m_accel;
    const f32 m_finalVelSq; ///< Velocity of the ball at the moment of impact
    const f32 m_endPosY;    ///< Height of the ball at the end of its rail
    const f32 m_sqVelXZ;    ///< Squared X-Z plane velocity

    /// @brief The enter and calc functions for each @ref StateManager entry
    static constexpr std::array<StateManagerEntry, 3> STATE_ENTRIES = {{
            {StateEntry<ObjectVolcanoBall, &ObjectVolcanoBall::enterDormant,
                    nullptr>(0)},
            {StateEntry<ObjectVolcanoBall, &ObjectVolcanoBall::enterFalling,
                    &ObjectVolcanoBall::calcFalling>(1)},
            {StateEntry<ObjectVolcanoBall, nullptr,
                    &ObjectVolcanoBall::calcBurning>(2)},
    }};
};

} // namespace Kinoko::Field
