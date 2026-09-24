#pragma once

#include "game/field/obj/ObjectCollidable.hh"

namespace Kinoko::Field {

/// @brief The falling rocks on GCN DK Mountain
/// @details Rocks move along a rail when they are tangible. When they collide with a floor, an
/// inelastic bounce occurs while increasng the rock's @ref m_angSpd. Rocks become intangible (aka
/// despawn) when they reach the end of the rail path, or, if the the rock's scale is less than
/// `1.0f`, the rock breaks and becomes intangible when a kart collides with it. Rocks with a scale
/// lesser than `1.0f` do not impart any reaction on the kart.
class ObjectRock final : public ObjectCollidable {
public:
    /// @addr{0x8076F2E0}
    /// @copybrief ObjectCollidable::ObjectCollidable(const System::MapdataGeoObj &)
    /// @param params The parameters used to initialize the object
    /// @details Initializes @ref m_cooldownDuration based on param setting 2, @ref m_railSpeed
    /// based on param setting 3, and @ref m_bounceFactor basd on param setting 4.
    ObjectRock(const System::MapdataGeoObj &params)
        : ObjectCollidable(params),
          m_cooldownDuration(params.setting(1)),
          m_railSpeed(static_cast<f32>(param.setting(2))),
          m_bounceFactor(static_cast<f32>(param.setting(3))) {}

    /// @addr{0x8076F344}
    /// @brief Default virtual destructor
    ~ObjectRock() override = default;

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
        return m_vel;
    }

    [[nodiscard]] Kart::Reaction onCollision(Kart::KartObject *kartObj,
            Kart::Reaction reactionOnKart, Kart::Reaction reactionOnObj,
            EGG::Vector3f &hitDepth) override;

private:
    /// @brief Distinguishes between whether the rock is collidable/visible
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
    /// @details Sets @ref m_state to @ref State::Tangible. Resets @ref m_angSpd to @ref
    /// INITIAL_ANGULAR_SPEED, sets @ref m_vel.y to @ref m_bounceFactor, and resets the
    /// cooldown timer. Finally, re-enables the rock's collision.
    void enterTangible() {
        m_state = State::Tangible;
        m_angSpd = INITIAL_ANGULAR_SPEED;
        m_vel.y = m_bounceFactor;
        m_cooldownTimer = m_cooldownDuration;
        enableCollision();
    }

    void checkSphereFull();

    /// @addr{0x8076FD90}
    /// @brief Runs when a collision occurs or when the rock reaches the end of its rail path
    /// @details Sets @ref m_state to @ref State::Intangible, reinitializes the rail interpolator to
    /// the start of the rail with a speed of @ref m_railSpeed, resets the rock's vertical position
    /// to @ref m_startYPos, and disables the rock's collision.
    void breakRock() {
        m_state = State::Intangible;
        m_railInterpolator->init(0.0f, 0);
        m_railInterpolator->setSpeed(m_railSpeed);
        setPos(EGG::Vector3f(pos().x, m_startYPos, pos().z));
        disableCollision();
    }

    State m_state;       ///< Current tangibility of the rock
    f32 m_startYPos;     ///< Starting y-axis position
    EGG::Vector3f m_vel; ///< Current velocity of the rock
    f32 m_angSpd;        ///< Angular velocity
    s32 m_cooldownTimer; ///< Cooldown timer before rock becomes tangible again
    f32 m_angRad;        ///< Rotation from accumulated angular velocity

    const s32 m_cooldownDuration; ///< Duration of the cooldown before becoming tangible again
    const f32 m_railSpeed;        ///< Speed at which the rock moves along the rail
    const f32 m_bounceFactor; ///< Scales initial y position, gravity, and floor bounce dampening

    static constexpr f32 INITIAL_ANGULAR_SPEED = 3.0f; ///< The initial angular velocity of the rock
};

} // namespace Kinoko::Field
