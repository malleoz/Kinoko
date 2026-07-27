#pragma once

#include "game/field/obj/ObjectBreakable.hh"

namespace Kinoko::Field {

/// @brief Represents a wooden box, like on DS Delfino Square
/// @details Normally these boxes are breakable via items or collisions, but in time trial mode,
/// these boxes become unbreakable "ironboxes".
class ObjectWoodbox : public ObjectBreakable {
public:
    ObjectWoodbox(const System::MapdataGeoObj &params);
    ~ObjectWoodbox() override;

    /// @addr{0x8077ED7C}
    [[nodiscard]] const char *getKclName() const override {
        return "ironbox"; // woodbox when not in TTs
    }

    void calcCollisionTransform() override;
};

} // namespace Kinoko::Field
