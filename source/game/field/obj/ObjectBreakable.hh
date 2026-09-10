#pragma once

#include "game/field/obj/ObjectCollidable.hh"

namespace Kinoko::Field {

/// @brief Represents objects that can be broken by kart collision, such as the rDS/TF wooden boxes
/// @details This class effectively does nothing currently (except maintaining an "active" state)
/// and is implemented only to maintain the inheritance heirarchy used by @ref ObjectWoodbox.
class ObjectBreakable : public ObjectCollidable {
public:
    /// @addr{0x8076EBE0}
    /// @brief Constructor
    /// @param params The parameters used to initialize the object
    ObjectBreakable(const System::MapdataGeoObj &params) : ObjectCollidable(params), m_state(0) {}

    /// @addr{0x8076EC28}
    /// @brief Default virtual destructor
    ~ObjectBreakable() override = default;

    /// @addr{0x807677E4}
    /// @copybrief ObjectBase::loadFlags()
    /// @return Returns @ref eLoadFlags::Calc, so that object is calculated every frame.
    [[nodiscard]] LoadFlags loadFlags() const override {
        return LoadFlags(eLoadFlags::Calc);
    }

    /// @addr{0x8076ED70}
    /// @brief Enables a spawner to make the object tangible again
    virtual void enableCollision() {
        m_state = 1;
    }

protected:
    u32 m_state; ///< 0 = inactive, 1 = active (collidable)
};

} // namespace Kinoko::Field
