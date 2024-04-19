#include "Scene.hh"

namespace EGG {

/// @addr{0x8023AD10}
Scene::Scene() {
    m_parent = nullptr;
    m_child = nullptr;
    m_id = -1;
    m_sceneMgr = nullptr;
}

Scene::~Scene() = default;

} // namespace EGG
