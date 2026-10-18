#include "Scene.hh"

#include "egg/core/SceneManager.hh"

namespace Kinoko::EGG {

/// @addr{0x8023AD10}
Scene::Scene() {
    m_heap = SceneManager::heapForCreateScene();
    m_parent = nullptr;
    m_child = nullptr;
    m_id = -1;
    m_sceneMgr = nullptr;
}

/// @addr{0x8023AD84}
Scene::~Scene() = default;

/// @brief Responsible for performing per-frame updates for the scene
void Scene::calc() {}

/// @brief Called when the scene is entered, performing necessary setup
void Scene::enter() {}

/// @brief Called when the scene is exited, responsible for performing necessary cleanup
void Scene::exit() {}

/// @brief Responsible for reinitializing the scene
void Scene::reinit() {}

void Scene::incoming_childDestroy() {}
void Scene::outgoing_childCreate() {}

void Scene::setParent(Scene *parent) {
    m_parent = parent;
}

void Scene::setChild(Scene *child) {
    m_child = child;
}

void Scene::setId(int id) {
    m_id = id;
}

void Scene::setSceneMgr(SceneManager *sceneMgr) {
    m_sceneMgr = sceneMgr;
}

Heap *Scene::heap() const {
    return m_heap;
}

Scene *Scene::parent() const {
    return m_parent;
}

Scene *Scene::child() const {
    return m_child;
}

int Scene::id() const {
    return m_id;
}

SceneManager *Scene::sceneMgr() const {
    return m_sceneMgr;
}

} // namespace Kinoko::EGG
