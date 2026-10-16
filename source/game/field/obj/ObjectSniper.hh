#pragma once

#include "game/field/RailManager.hh"
#include "game/field/obj/ObjectProjectile.hh"
#include "game/field/obj/ObjectProjectileLauncher.hh"

namespace Kinoko::Field {

/// @brief The base class for a manager object which is responsible for synchronizing a set of
/// projectiles and a projectile launcher.
/// @details Every frame, this class checks if the launcher is ready to spawn a projectile, and it
/// maps between the launcher's current rail point and the corresponding projectile to launch. It is
/// expected that the derived class sets the member pointers and @ref m_pointIdxs.
class ObjectSniper : public ObjectCollidable {
public:
    ObjectSniper();
    ~ObjectSniper() override;

    void init() override;
    void calc() override;
    void loadGraphics() override;
    void createCollision() override;
    void loadRail() override;
    [[nodiscard]] LoadFlags loadFlags() const override;

protected:
    owning_span<ObjectProjectile *> m_projectiles; ///< Pointers to the managed projectiles
    ObjectProjectileLauncher *m_launcher;          ///< The object launching the projectiles
    owning_span<s16> m_pointIdxs; ///< Indices along the rail that each projectile should launch at
};

} // namespace Kinoko::Field
