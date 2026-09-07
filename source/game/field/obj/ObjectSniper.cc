#include "ObjectSniper.hh"

#include "game/field/RailManager.hh"

namespace Kinoko::Field {

/// @addr{0x806DDA84}
/// @brief Constructor
ObjectSniper::ObjectSniper()
    : ObjectCollidable("MapObjSniper", EGG::Vector3f::zero, EGG::Vector3f::ez,
              EGG::Vector3f::unit) {}

/// @addr{0x806DDAF4}
/// @brief Default virtual destructor
ObjectSniper::~ObjectSniper() = default;

/// @addr{0x806DDB34}
/// @copybrief ObjectBase::init()
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

} // namespace Kinoko::Field
