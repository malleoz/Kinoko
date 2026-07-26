#pragma once

#include "game/field/obj/ObjectCollidable.hh"

namespace Kinoko::Field {

/// @brief The falling rocks on GCN DK Mountain
class ObjectRock : public ObjectCollidable {
public:
    ObjectRock(const System::MapdataGeoObj &params);
    ~ObjectRock() override;

    void init() override;
    void calc() override;

    /// @addr{0x8077037C}
    [[nodiscard]] u32 loadFlags() const override {
        return 1;
    }

    /// @addr{0x8077036C}
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
    void enterTangible();
    void checkSphereFull();
    void breakRock();

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
