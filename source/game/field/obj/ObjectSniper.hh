#pragma once

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

    /// @addr{0x806DDC44}
    /// @details Checks if the launcher is ready to launch a projectile and triggers the
    /// corresponding projectile's launch.
    void calc() override {
        s32 idx = m_launcher->launchPointIdx();

        if (idx != -1) {
            m_projectiles[m_pointIdxs[idx]]->onLaunch();
        }
    }

    /// @addr{0x806D2900}
    [[nodiscard]] u32 loadFlags() const override {
        return 1;
    }

    /// @addr{0x806D28FC}
    /// @details no-op because the manager object does not have any graphics to load.
    void loadGraphics() override {}

    /// @addr{0x806D28F4}
    /// @details no-op because the manager object does not have any collision to load. The launcher
    /// object independently loads its own collision.
    void createCollision() override {}

    /// @addr{0x806D28F8}
    /// @details no-op because the manager object does not have any rail to load. The launcher
    /// object independently manages its own position.
    void loadRail() override {}

protected:
    owning_span<ObjectProjectile *> m_projectiles; ///< Pointers to the managed projectiles
    ObjectProjectileLauncher *m_launcher;          ///< The object launching the projectiles
    owning_span<s16> m_pointIdxs; ///< Indices along the rail that each projectile should launch at
};

} // namespace Kinoko::Field
