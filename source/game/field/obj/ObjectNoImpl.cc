#include "ObjectNoImpl.hh"

#include "game/field/ObjectDirector.hh"

namespace Kinoko::Field {

/// @copybrief ObjectBase::load()
/// @details Registers the object to the ObjectDirector without any implementation.
void ObjectNoImpl::load() {
    ObjectDirector::Instance()->addObjectNoImpl(this);
}

} // namespace Kinoko::Field
