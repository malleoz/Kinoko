#pragma once

#include "game/field/obj/ObjectCollidable.hh"

namespace Kinoko::Field {

/// @brief Represents objects that can be broken by kart collision, such as the rDS/TF wooden boxes
/// @details This class effectively does nothing currently (except maintaining an "active" state)
/// and is implemented only to maintain the inheritance heirarchy used by @ref ObjectWoodbox.
class ObjectBreakable : public ObjectCollidable {
public:
    /// @addr{0x8076EBE0}
    ObjectBreakable(const System::MapdataGeoObj &params) : ObjectCollidable(params), m_state(0) {}

    /// @addr{0x8076EC28}
    ~ObjectBreakable() = default;

    /// @addr{0x807677E4}
    [[nodiscard]] u32 loadFlags() const override {
        return 1;
    }

    /// @addr{0x8076F0AC}
    Kart::Reaction onCollision(Kart::KartObject * /*kartObj*/, Kart::Reaction reactionOnKart,
            Kart::Reaction /*reactionOnObj*/, EGG::Vector3f & /*hitDepth*/) override {
        return reactionOnKart;
    }

    /// @addr{0x8076ED70}
    /// @brief Interface through which a spawner can make the object tangible again
    virtual void enableCollision() {
        m_state = 1;
    }

protected:
    u32 m_state; ///< 0 = inactive, 1 = active (collidable)
};

} // namespace Kinoko::Field
