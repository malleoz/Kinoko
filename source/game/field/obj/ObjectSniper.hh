#pragma once

#include "game/field/RailManager.hh"
#include "game/field/obj/ObjectProjectile.hh"
#include "game/field/obj/ObjectProjectileLauncher.hh"

namespace Field {

/// @brief The base class for a manager object which is responsible for synchronizing a set of
/// projectiles and a projectile launcher.
/// @details In the base game, this is used for:
/// - DS Desert Hills: The sun and FireSnakes
/// - GBA Shy Guy Beach: The ship and the bombs
/// Every frame, this class checks if the launcher is ready to spawn a projectile, and it maps
/// between the launcher's current rail point and the corresponding projectile to begin launching
/// it.
class ObjectSniper : public ObjectCollidable {
public:
    /// @addr{0x806DDA84}
    ObjectSniper()
        : ObjectCollidable("MapObjSniper", EGG::Vector3f::zero, EGG::Vector3f::ez,
                  EGG::Vector3f::unit) {}

    /// @addr{0x806DDAF4}
    ~ObjectSniper() override {
        delete[] m_projectiles.data();
        delete[] m_pointIdxs.data();
    }

    /// @addr{0x806DDB34}
    void init() override {
        const auto *srcRailInterp = m_launcher->railInterpolator();

        for (auto &point : m_pointIdxs) {
            point = -1;
        }

        for (size_t i = 0; i < m_projectiles.size(); ++i) {
            auto *obj = m_projectiles[i];
            m_pointIdxs[obj->idx()] = i;
        }

        auto *sunRail = RailManager::Instance()->rail(srcRailInterp->railIdx());

        for (auto *&obj : m_projectiles) {
            const EGG::Vector3f &pos = sunRail->pointPos(obj->idx());
            obj->initFromSniper(pos);
        }
    }

    /// @addr{0x806DDC44}
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
    void loadGraphics() override {}

    /// @addr{0x806D28F4}
    void createCollision() override {}

    /// @addr{0x806D28F8}
    void loadRail() override {}

protected:
    std::span<ObjectProjectile *> m_projectiles;
    ObjectProjectileLauncher *m_launcher; // The rDH sun or the RSGB ship
    std::span<s16> m_pointIdxs;
};

} // namespace Field
