#pragma once

#include "game/field/obj/ObjectBase.hh"

namespace Kinoko::Field {

/// @brief Represents an object that is explicitly not implemented in Kinoko because it either does
/// not have collision or does not pertain to Time Trial mode
class ObjectNoImpl final : public ObjectBase {
public:
    ObjectNoImpl(const System::MapdataGeoObj &params);
    ~ObjectNoImpl() override;

    void load() override;
    void createCollision() override {}
    void calcCollisionTransform() override {}
};

} // namespace Kinoko::Field
