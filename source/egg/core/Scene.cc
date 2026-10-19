#include "Scene.hh"

#include "egg/core/SceneManager.hh"

namespace Kinoko::EGG {

/// @addr{0x8023AD10}
/// @brief Constructor
/// @details Initializes @ref m_heap using @ref SceneManager::heapForCreateScene(). Sets @ref
/// m_parent, @ref m_child, and @ref m_sceneMgr to `nullptr` and @ref m_id to `-1`.
Scene::Scene() : m_heap(SceneManager::heapForCreateScene()) {
    m_parent = nullptr;
    m_child = nullptr;
    m_id = -1;
    m_sceneMgr = nullptr;
}

/// @addr{0x8023AD84}
/// @brief Default virtual destructor
Scene::~Scene() = default;

/// @brief Responsible for performing per-frame updates for the scene
/// @details This is a no-op since Kinoko does not create an instance of the base class.
void Scene::calc() {}

/// @brief Called when the scene is entered, performing necessary setup
/// @details This is a no-op since Kinoko does not create an instance of the base class.
void Scene::enter() {}

/// @brief Called when the scene is exited, responsible for performing necessary cleanup
/// @details This is a no-op since Kinoko does not create an instance of the base class.
void Scene::exit() {}

/// @brief Responsible for reinitializing the scene
/// @details This is a no-op since Kinoko does not create an instance of the base class.
void Scene::reinit() {}

/// @brief Sets the parent for this scene
/// @param parent The parent scene to set
void Scene::setParent(Scene *parent) {
    m_parent = parent;
}

/// @brief Sets the child for this scene
/// @param child The child scene to set
void Scene::setChild(Scene *child) {
    m_child = child;
}

/// @brief Sets the ID for this scene
/// @param id The ID to set
void Scene::setId(int id) {
    m_id = id;
}

/// @brief Sets the scene manager for this scene
/// @param sceneMgr The @ref SceneManager to set
void Scene::setSceneMgr(SceneManager *sceneMgr) {
    m_sceneMgr = sceneMgr;
}

/// @brief Gets a pointer to the heap for this scene
/// @return A pointer to the @ref Heap used by this scene, @ref m_heap
Heap *Scene::heap() const {
    return m_heap;
}

/// @brief Gets the parent of this scene
/// @return A pointer to the parent scene, @ref m_parent
Scene *Scene::parent() const {
    return m_parent;
}

/// @brief Gets the child of this scene
/// @return A pointer to the child scene, @ref m_child
Scene *Scene::child() const {
    return m_child;
}

/// @brief Gets the ID of this scene
/// @return The ID of the scene, @ref m_id
int Scene::id() const {
    return m_id;
}

/// @brief Gets the scene manager for this scene
/// @return A pointer to the @ref SceneManager managing this scene, @ref m_sceneMgr
SceneManager *Scene::sceneMgr() const {
    return m_sceneMgr;
}

} // namespace Kinoko::EGG
