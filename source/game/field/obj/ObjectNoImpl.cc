#include "ObjectNoImpl.hh"

#include "game/field/ObjectDirector.hh"

namespace Kinoko::Field {

/// @brief Constructor
/// @param params The parameters used to initialize the object
ObjectNoImpl::ObjectNoImpl(const System::MapdataGeoObj &params) : ObjectBase(params) {}

/// @brief Default virtual destructor
ObjectNoImpl::~ObjectNoImpl() = default;

/// @copybrief ObjectBase::load()
/// @details Registers the object to the ObjectDirector without any implementation.
void ObjectNoImpl::load() {
    ObjectDirector::Instance()->addObjectNoImpl(this);
}

} // namespace Kinoko::Field
