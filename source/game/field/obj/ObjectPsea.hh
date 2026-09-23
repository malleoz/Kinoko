#pragma once

#include "game/field/obj/ObjectCollidable.hh"

namespace Kinoko::Field {

/// @brief Represents the rising water on GCN Peach Beach and Delfino Pier battle stage.
/// @details In the base game, there are 5 subclasses owned by ObjectPsea. In terms of collision,
/// the water's height is only a function of one of them. We can simplify in Kinoko and remove that
/// subclass entirely since its position can be easily represented without an instance of the class.
/// @note This object is used in an unusual way - the @ref ObjectDirector keeps a pointer to it and
/// uses it to determine the height of the rising water. @ref Kart::KartMove will then use that
/// height to determine whether the kart is submerged and should be slowed down. Because @ref
/// ObjectDirector holds a single pointer to @ref ObjectPsea, if there are multiple instances of
/// this object, then only the most-recently-constructed one will have an effect on the player's
/// speed or out-of-bounds trigger.
class ObjectPsea final : public ObjectCollidable {
public:
    ObjectPsea(const System::MapdataGeoObj &params);

    /// @addr{0x8082C890}
    /// @brief Default virtual destructor
    ~ObjectPsea() override = default;

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
    /// @brief Helper function to initialize @ref m_initPosY
    /// @return The initial Y position of the rising water after applying hard-coded offsets
    [[nodiscard]] f32 calcInitPosY() const {
        constexpr EGG::Vector3f POS_OFFSET = EGG::Vector3f(0.0f, 6.75f, 0.0f) * 96.0f;
        constexpr EGG::Vector3f EXTRA_OFFSET = EGG::Vector3f(0.0f, 1.0f, 0.0f);

        return (pos() + POS_OFFSET + EXTRA_OFFSET).y;
    }

    u16 m_frame;          ///< Frame within the sinusoidal cycle of the rising water
    const f32 m_initPosY; ///< Initial position after applying hard-coded offsets

    static constexpr s16 CYCLE_DURATION = 1200; ///< Framecount of full cycle

    /// @brief The period of the sinusoidal cycle
    static constexpr f32 PERIOD = F_TAU / static_cast<f32>(CYCLE_DURATION);
};

} // namespace Kinoko::Field
