#pragma once

#include "game/field/obj/ObjectCollidable.hh"

namespace Kinoko::Field {

/// @brief Represents a pipe, like on SNES Mario Circuit 3
class ObjectDokan final : public ObjectCollidable {
public:
    ObjectDokan(const System::MapdataGeoObj &params);
    ~ObjectDokan() override;

    void init() override;
    void calc() override;

    /// @addr{0x80778FE4}
    [[nodiscard]] u32 loadFlags() const override {
        return 1;
    }

    void calcCollisionTransform() override;

    Kart::Reaction onCollision(Kart::KartObject *kartObj, Kart::Reaction reactionOnKart,
            Kart::Reaction reactionOnObj, EGG::Vector3f &hitDepth) override;

private:
    void calcFloor();

    bool m_isAirborne;        ///< Whether the pipe is currently mid-air
    EGG::Vector3f m_velocity; ////< The current velocity of the pipe
};

} // namespace Kinoko::Field
