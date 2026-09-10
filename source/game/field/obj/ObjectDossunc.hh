#pragma once

#include "game/field/obj/ObjectCollidable.hh"

namespace Kinoko::Field {

/// @brief Holder class for one of various Thwomp "modes".
class ObjectDossunc final : public ObjectCollidable {
public:
    ObjectDossunc(const System::MapdataGeoObj &params);

    /// @addr{0x80764B08}
    /// @brief Default virtual destructor
    ~ObjectDossunc() override = default;

    /// @addr{0x80764A38}
    void load() override;
};

} // namespace Kinoko::Field
