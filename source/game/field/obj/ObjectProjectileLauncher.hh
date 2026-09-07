#pragma once

#include "game/field/obj/ObjectCollidable.hh"

namespace Kinoko::Field {

/// @brief %Abstract class that moves along its own rail and throws projectiles.
/// @note This class must be constructed before the projectiles, otherwise @ref ObjectFireSnake will
/// not register itself as a managed object since it only registers itself if the managed object
/// vector is non-empty.
class ObjectProjectileLauncher : public ObjectCollidable {
public:
    /// @addr{Inlined in 0x806DDDD8} @addr{Inlined in 0x806D18FC}
    /// @brief Constructor
    /// @param params The parameters used to initialize the object
    ObjectProjectileLauncher(const System::MapdataGeoObj &params) : ObjectCollidable(params) {
        registerManagedObject();
    }

    /// @addr{0x806D1998}
    /// @brief Default virtual destructor
    ~ObjectProjectileLauncher() override = default;

    /// @copybrief ObjectBase::createCollision()
    /// @details Not overridden in the base game, but has collision mode 0.
    void createCollision() override {}

    /// @brief Used by @ref ObjectSniper to check which object index (if any) should be thrown
    virtual s16 launchPointIdx() = 0;
};

} // namespace Kinoko::Field
