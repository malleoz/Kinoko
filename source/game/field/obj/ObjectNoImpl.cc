#include "ObjectNoImpl.hh"

#include "game/field/ObjectDirector.hh"

namespace Kinoko::Field {

/// @copydoc ObjectBase::ObjectBase(const System::MapdataGeoObj &)
ObjectNoImpl::ObjectNoImpl(const System::MapdataGeoObj &params) : ObjectBase(params) {}

/// @brief Default virtual destructor
ObjectNoImpl::~ObjectNoImpl() = default;

/// @copybrief ObjectBase::load()
/// @details Registers the object to the ObjectDirector without any implementation.
void ObjectNoImpl::load() {
    ObjectDirector::Instance()->addObjectNoImpl(this);
}

} // namespace Kinoko::Field
