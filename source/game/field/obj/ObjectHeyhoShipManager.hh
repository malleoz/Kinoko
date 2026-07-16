#pragma once

#include "game/field/obj/ObjectSniper.hh"

namespace Kinoko::Field {

/// @brief Managed the synchronization between the @ref ObjectHeyhoBall and the @ref ObjectHeyhoShip
class ObjectHeyhoShipManager final : public ObjectSniper {
public:
    ObjectHeyhoShipManager();
    ~ObjectHeyhoShipManager() override;

    void init() override;
    void calc() override;
};

} // namespace Kinoko::Field
