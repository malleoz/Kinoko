#pragma once

#include "game/field/obj/ObjectCollidable.hh"

namespace Kinoko::Field {

/// @brief Represents the rising water on GCN Peach Beach and Delfino Pier battle stage.
/// @details In the base game, there are 5 subclasses owned by ObjectPsea. In terms of collision,
/// the water's height is only a function of one of them. We can simplify in Kinoko and remove that
/// subclass entirely since its position can be easily represented without an instance of the class.
/// This object is used in an unusual way - the @ref ObjectDirector keeps a pointer to it and uses
/// it to determine the height of the rising water. @ref Kart::KartMove will then use that height to
/// determine whether the kart is submerged and should be slowed down.
class ObjectPsea final : public ObjectCollidable {
public:
    ObjectPsea(const System::MapdataGeoObj &params);
    ~ObjectPsea() override;

    void calc() override;

    /// @addr{0x8082C888}
    /// @copybrief ObjectBase::loadFlags()
    /// @return Returns @ref eLoadFlags::Calc, so that object is calculated every frame.
    [[nodiscard]] LoadFlags loadFlags() const override {
        return LoadFlags(eLoadFlags::Calc);
    }

    /// @addr{0x8082C884}
    /// @copybrief ObjectBase::loadGraphics()
    /// @details This is a no-op in the base game.
    void loadGraphics() override {}

    /// @copybrief ObjectBase::createCollision()
    /// @details no-op because collision is managed via @ref Kart::KartMove::calcRisingWater().
    void createCollision() override {}

private:
    u16 m_frame;        ///< Frame within the sinusoidal cycle of the rising water
    const f32 m_period; ///< Period of the sinusoidal cycle
    f32 m_initPosY;     ///< Initial position after applying hard-coded offsets

    static constexpr s16 CYCLE_DURATION = 1200; ///< Framecount of full cycle
};

} // namespace Kinoko::Field
