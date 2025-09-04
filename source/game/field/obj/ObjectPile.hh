#pragma once

#include "game/field/obj/ObjectCollidable.hh"

namespace Field {

class ObjectPile : public ObjectCollidable {
public:
    /// @addr{Inlined in 0x806E4224}
    ObjectPile(const EGG::Vector3f &pos, const EGG::Vector3f &rot, const EGG::Vector3f &scale)
        : ObjectCollidable("pile", pos, rot, scale) {}

    /// @addr{0x806E9568}
    ~ObjectPile() override = default;
};

} // namespace Field
