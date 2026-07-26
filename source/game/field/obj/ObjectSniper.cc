#include "ObjectSniper.hh"

#include "game/field/RailManager.hh"

namespace Kinoko::Field {

/// @addr{0x806DDA84}
ObjectSniper::ObjectSniper()
    : ObjectCollidable("MapObjSniper", EGG::Vector3f::zero, EGG::Vector3f::ez,
              EGG::Vector3f::unit) {}

/// @addr{0x806DDAF4}
ObjectSniper::~ObjectSniper() = default;

/// @addr{0x806DDB34}
/// @details Constructs the m_pointIdxs mapping between projectile indices and the corresponding
/// rail point index to launch the projectile at.
void ObjectSniper::init() {
    const auto *launcherRailInterp = m_launcher->railInterpolator();

    for (auto &point : m_pointIdxs) {
        point = -1;
    }

    auto *launcherRail = RailManager::Instance()->rail(launcherRailInterp->railIdx());

    for (size_t i = 0; i < m_projectiles.size(); ++i) {
        auto *obj = m_projectiles[i];
        m_pointIdxs[obj->idx()] = i;
        obj->initProjectile(launcherRail->pointPos(obj->idx()));
    }
}

/// @addr{0x806DDC44}
/// @details Checks if the launcher is ready to launch a projectile and triggers the corresponding
/// projectile's launch.
void ObjectSniper::calc() {
    s32 idx = m_launcher->launchPointIdx();

    if (idx != -1) {
        m_projectiles[m_pointIdxs[idx]]->onLaunch();
    }
}

} // namespace Kinoko::Field
