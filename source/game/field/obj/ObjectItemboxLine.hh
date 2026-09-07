#pragma once

#include "game/field/obj/ObjectCollidable.hh"

namespace Kinoko::Field {

class ObjectItemboxPress;

/// @brief Object which represents a line of itemboxes, a brick block which may be pressed into an
/// itembox, and a stomper.
/// @details The stomper is held and managed by @ref ObjectItemboxPress. Since we only implement the
/// components relevant to Time Trial mode, the itemboxes and bricks are not implemented - only the
/// stomper.
class ObjectItemboxLine final : public ObjectCollidable {
public:
    ObjectItemboxLine(const System::MapdataGeoObj &params);
    ~ObjectItemboxLine() override;

    void init() override;
    void calc() override;

    /// @addr{0x8076E9BC}
    /// @copybrief ObjectBase::loadFlags()
    /// @return Returns @ref eLoadFlags::Calc, so that object is calculated every frame.
    [[nodiscard]] LoadFlags loadFlags() const override {
        return LoadFlags(eLoadFlags::Calc);
    }

private:
    owning_span<ObjectItemboxPress *> m_press; ///< Pointer to the stomper manager objects
    u32 m_stompCooldown;                       ///< Number of frames until next stomper is activated
    u32 m_curPressIdx;                         ///< Index of the next stomper to activate
};

} // namespace Kinoko::Field
