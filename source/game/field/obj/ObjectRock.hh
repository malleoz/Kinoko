#pragma once

#include "game/field/obj/ObjectCollidable.hh"

namespace Kinoko::Field {

/// @brief The falling rocks on GCN DK Mountain
class ObjectRock final : public ObjectCollidable {
public:
    ObjectRock(const System::MapdataGeoObj &params);
    ~ObjectRock() override;

    void init() override;
    void calc() override;

    /// @addr{0x8077037C}
    /// @copybrief ObjectBase::loadFlags()
    /// @return Returns @ref eLoadFlags::Calc, so that object is calculated every frame.
    [[nodiscard]] LoadFlags loadFlags() const override {
        return LoadFlags(eLoadFlags::Calc);
    }

    /// @addr{0x8077036C}
    /// @copydoc ObjectCollidable::getCollisionTranslation()
    [[nodiscard]] const EGG::Vector3f &getCollisionTranslation() const override {
        return m_colTranslate;
    }

    [[nodiscard]] Kart::Reaction onCollision(Kart::KartObject *kartObj,
            Kart::Reaction reactionOnKart, Kart::Reaction reactionOnObj,
            EGG::Vector3f &hitDepth) override;

private:
    /// @brief Distinguishes whether the rock is collidable/visible
    enum class State {
        Tangible = 0,   ///< The rock is rolling
        Intangible = 1, ///< The rock is despawned
    };

    void calcTangible();

    /// @addr{0x8076F868}
    /// @brief Runs every frame that the rock is intangible
    /// @details Waits for the cooldown timer to expire before making the rock tangible again.
    void calcIntangible() {
        if (m_cooldownTimer < 0) {
            enterTangible();
        }
    }

    void calcTangibleSub();

    /// @addr{0x8076FFC0}
    /// @brief Makes the rock tangible again after being intangible
    void enterTangible() {
        m_state = State::Tangible;
        m_angSpd = INITIAL_ANGULAR_SPEED;
        m_colTranslate.y = m_bounceFactor;
        m_cooldownTimer = m_cooldownDuration;
        enableCollision();
    }

    void checkSphereFull();

    /// @addr{0x8076FD90}
    /// @brief Runs when a collision occurs or when the rock reaches the end of its rail path
    void breakRock() {
        m_state = State::Intangible;
        m_railInterpolator->init(0.0f, 0);
        m_railInterpolator->setSpeed(m_railSpeed);
        setPos(EGG::Vector3f(pos().x, m_startYPos, pos().z));
        disableCollision();
    }

    State m_state;                ///< Current tangibility of the rock
    f32 m_startYPos;              ///< Starting y-axis position
    EGG::Vector3f m_colTranslate; ///< Current collision translation
    f32 m_angSpd;                 ///< Angular velocity
    s32 m_cooldownTimer;          ///< Cooldown timer before rock becomes tangible again
    f32 m_angRad;                 ///< Rotation from accumulated angular velocity

    const s32 m_cooldownDuration; ///< Duration of the cooldown before becoming tangible again
    const f32 m_railSpeed;        ///< Speed at which the rock moves along the rail
    const f32 m_bounceFactor; ///< Scales initial y position, gravity, and floor bounce dampening

    static constexpr f32 INITIAL_ANGULAR_SPEED = 3.0f;
};

} // namespace Kinoko::Field
