#pragma once

#include "game/field/obj/ObjectCollidable.hh"

namespace Kinoko::Field {

/// @brief The beach umbrellas on GBA Shy Guy Beach
/// @details Simply acts as a wall for collision purposes.
class ObjectParasolR final : public ObjectCollidable {
public:
    /// @addr{0x8077902C}
    /// @brief Constructor
    /// @param params The parameters used to initialize the object
    ObjectParasolR(const System::MapdataGeoObj &params) : ObjectCollidable(params) {}

    /// @addr{0x80779EFC}
    /// @brief Default virtual destructor
    ~ObjectParasolR() override = default;

    /// @addr{0x80779EF4}
    [[nodiscard]] u32 loadFlags() const override {
        return 1;
    }
};

} // namespace Kinoko::Field
