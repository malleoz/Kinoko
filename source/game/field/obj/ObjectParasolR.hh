#pragma once

#include "game/field/obj/ObjectCollidable.hh"

namespace Kinoko::Field {

/// @brief The beach umbrellas on GBA Shy Guy Beach
/// @details Simply acts as a wall for collision purposes.
class ObjectParasolR : public ObjectCollidable {
public:
    /// @addr{0x8077902C}
    ObjectParasolR(const System::MapdataGeoObj &params) : ObjectCollidable(params) {}

    /// @addr{0x80779EFC}
    ~ObjectParasolR() override = default;

    /// @addr{0x80779EF4}
    [[nodiscard]] u32 loadFlags() const override {
        return 1;
    }
};

} // namespace Kinoko::Field
