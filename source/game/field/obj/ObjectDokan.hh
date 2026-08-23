#pragma once

#include "game/field/obj/ObjectCollidable.hh"

namespace Kinoko::Field {

/// @brief Represents a pipe, like on SNES Mario Circuit 3
class ObjectDokan final : public ObjectCollidable {
public:
    ObjectDokan(const System::MapdataGeoObj &params);
    ~ObjectDokan() override;

    /// @addr{0x80778830}
    void init() override {
        m_isAirborne = false;
    }

    /// @addr{0x807788C8}
    void calc() override {
        if (!m_isAirborne) {
            return;
        }

        calcPos();
        calcFloor();
    }

    /// @addr{0x80778FE4}
    [[nodiscard]] u32 loadFlags() const override {
        return 1;
    }

    void calcCollisionTransform() override;

    Kart::Reaction onCollision(Kart::KartObject *kartObj, Kart::Reaction reactionOnKart,
            Kart::Reaction reactionOnObj, EGG::Vector3f &hitDepth) override;

private:
    /// @addr{0x8077894C}
    /// @brief Applies gravity to velocity and updates position accordingly
    void calcPos() {
        constexpr f32 ACCEL = 2.0f;

        m_velocity.y -= ACCEL;
        addPos(m_velocity);
    }

    void calcFloor();

    /// @addr{0x80778BA0}
    /// @brief If the pipe is not already airborne, induces upwards velocity
    void tryStartAirborne() {
        constexpr f32 INITIAL_VELOCITY = 100.0f;

        if (!m_isAirborne) {
            m_isAirborne = true;
            m_velocity = INITIAL_VELOCITY * EGG::Vector3f::ey;
        }
    }

    bool m_isAirborne;        ///< Whether the pipe is currently mid-air
    EGG::Vector3f m_velocity; ////< The current velocity of the pipe
};

} // namespace Kinoko::Field
