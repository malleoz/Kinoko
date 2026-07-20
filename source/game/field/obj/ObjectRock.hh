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
    void calcIntangible();
    void calcTangibleSub();

    void checkSphereFull();
    void breakRock();

    State m_state;                ///< Current tangibility of the rock
    f32 m_startYPos;              ///< Starting y-axis position
    EGG::Vector3f m_colTranslate; ///< Current collision translation
    f32 m_angSpd;                 ///< Angular velocity
    s32 m_cooldownTimer;
    f32 m_angRad;

    static constexpr f32 INITIAL_ANGULAR_SPEED = 3.0f;
};

} // namespace Kinoko::Field
