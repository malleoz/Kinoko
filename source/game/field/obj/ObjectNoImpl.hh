#pragma once

#include "game/field/obj/ObjectBase.hh"

namespace Kinoko::Field {

/// @brief Represents an object that is explicitly not implemented in Kinoko because it either does
/// not have collision or does not pertain to Time Trial mode
class ObjectNoImpl final : public ObjectBase {
public:
    /// @copybrief ObjectBase::ObjectBase(const System::MapdataGeoObj &)
    /// @param params The parameters used to initialize the object
    ObjectNoImpl(const System::MapdataGeoObj &params) : ObjectBase(params) {}

    /// @brief Default virtual destructor
    ~ObjectNoImpl() override = default;

    void load() override;

    /// @copybrief ObjectBase::createCollision()
    /// @details no-op since these objects are not implemented.
    void createCollision() override {}

    /// @copybrief ObjectBase::calcCollisionTransform()
    /// @details no-op since these objects are not implemented.
    void calcCollisionTransform() override {}
};

} // namespace Kinoko::Field
