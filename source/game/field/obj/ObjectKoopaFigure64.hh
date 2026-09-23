#pragma once

#include "game/field/obj/ObjectCollidable.hh"

namespace Kinoko::Field {

/// @brief The Bowser statues on N64 Bowser's Castle.
/// @details The big statue by the first turn has collision while the mini ones do not.
/// The big statue will breathe fire for a set duration, then stop for a cooldown period.
class ObjectKoopaFigure64 final : public ObjectCollidable {
public:
    /// @addr{0x806DA914}
    /// @copybrief ObjectCollidable::ObjectCollidable(const System::MapdataGeoObj &)
    /// @param params The parameters used to initialize the object
    /// @details Determines if the statue is large based on param setting 2. Initializes @ref
    /// m_startDelay based on param setting 3.
    ObjectKoopaFigure64(const System::MapdataGeoObj &params)
        : ObjectCollidable(params),
          m_isBigStatue(params.setting(1) == 1),
          m_startDelay(static_cast<u32>(params.setting(2))) {}

    /// @addr{0x806DB114}
    /// @brief Default virtual destructor
    ~ObjectKoopaFigure64() override = default;

    void init() override;
    void calc() override;

    /// @addr{0x806C0854}
    /// @details The game defines this in a GeoObjectSmoke base class, but we don't implement it.
    /// @copybrief ObjectBase::loadFlags()
    /// @return Returns @ref eLoadFlags::Calc, so that object is calculated every frame.
    [[nodiscard]] LoadFlags loadFlags() const override {
        return LoadFlags(eLoadFlags::Calc);
    }

    /// @addr{0x806DB154}
    /// @copybrief ObjectBase::createCollision()
    /// @details Collision is only created for the large statue, not the mini ones.
    void createCollision() override {
        if (m_isBigStatue) {
            ObjectCollidable::createCollision();
        }
    }

    void calcCollisionTransform() override;

    /// @addr{0x806DB168}
    /// @copybrief ObjectCollidable::onCollision()
    /// @param reactionOnKart The reaction that should be applied to the kart upon collision
    /// @param hitDepth The depth of the collision along each axis
    /// @return @ref Kart::Reaction::FireSpin
    /// @details This is only called as a result of the player colliding with the fire that the
    /// statue breathes. Also sets the hit depth to zero.
    Kart::Reaction onCollision(Kart::KartObject * /*kartObj*/, Kart::Reaction reactionOnKart,
            Kart::Reaction /*reactionOnObj*/, EGG::Vector3f &hitDepth) override {
        hitDepth.setZero();
        return reactionOnKart;
    }

private:
    const bool m_isBigStatue; ///< Differentiates the first rBC turn statue from tiny statues
    const u32 m_startDelay;   ///< Frame delay before the statue starts shooting fire
    u32 m_cycleFrame;         ///< Tracks the current frame in the fire/cooldown cycle

    static constexpr u32 FIRE_DURATION = 300; ///< How long the statue shoots fire for in a cycle
    static constexpr u32 COOLDOWN_DURATION = 180; ///< How long fire is disabled for in a cycle

    /// @brief Framecount of the entire cycle duration, including fire and cooldown
    static constexpr u32 CYCLE_DURATION = FIRE_DURATION + COOLDOWN_DURATION;
};

} // namespace Kinoko::Field
