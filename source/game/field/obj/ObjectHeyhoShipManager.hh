#pragma once

#include "game/field/obj/ObjectSniper.hh"

namespace Kinoko::Field {

/// @brief Managed the synchronization between the @ref ObjectHeyhoBall and the @ref ObjectHeyhoShip
/// @details Communicates the ship's position to all cannonballs so they can initialize their launch
/// parameters accordingly. In @ref calc(), this manager reacts when the @ref ObjectHeyhoShip has
/// reached the end of a rail segment, checking to see if any cannonballs should be launched.
class ObjectHeyhoShipManager final : public ObjectSniper {
public:
    ObjectHeyhoShipManager();

    /// @addr{0x806D2514}
    /// @brief Default virtual destructor
    ~ObjectHeyhoShipManager() override = default;

    void init() override;
    void calc() override;
};

} // namespace Kinoko::Field
