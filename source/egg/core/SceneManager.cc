#include "SceneManager.hh"

#include "egg/core/ExpHeap.hh"
#include "egg/core/Scene.hh"
#include "egg/core/SceneCreator.hh"

namespace Kinoko::EGG {

/// @addr{0x8023ADDC}
/// @brief Constructor
/// @param creator The scene creator used to instantiate new scenes
/// @details Sets @ref m_creator to the provided `creator` and initializes @ref m_currentScene to
/// `nullptr`.
SceneManager::SceneManager(SceneCreator *creator) : m_creator(creator) {
    m_currentScene = nullptr;
}

/// @brief Default virtual destructor
SceneManager::~SceneManager() = default;

/// @addr{0x8023AE60}
/// @brief Processes the current scene
/// @details Dispatches to @ref calcCurrentScene() to process the current scene.
void SceneManager::calc() {
    calcCurrentScene();
}

/// @addr{0x8023B588}
/// @brief Calculates the current scene if it exists
/// @details If @ref m_currentScene is not `nullptr`, then calls @ref Scene::calc().
void SceneManager::calcCurrentScene() {
    if (!m_currentScene) {
        return;
    }

    m_currentScene->calc();
}

/// @addr{0x8023B248}
/// @brief Creates a child scene of the provided parent scene
/// @param id The ID of the child scene to create
/// @param parent The parent scene
/// @details Sets @ref m_nextSceneId based on the provided `id` and creates a new scene as a child
/// of the specified parent.
void SceneManager::createChildScene(int id, Scene *parent) {
    setNextSceneId(id);
    setupNextSceneId();
    createScene(id, parent);
}

/// @addr{0x8023B0E4}
/// @brief Creates a new scene with the specified ID and parent
/// @param id The ID of the new scene to create
/// @param parent The parent scene for the new scene
/// @details Allocates a new heap for the scene, sets it as the current heap (or uses @ref sRootHeap
/// if `parent` is `nullptr`), and then uses @ref m_creator to instantiate the new scene. Sets up
/// the scene's ID, parent, and @ref SceneManager pointer, and calls @ref Scene::enter() on the new
/// scene.
void SceneManager::createScene(int id, Scene *parent) {
    Heap *parentHeap = parent ? parent->heap() : s_rootHeap;

    // We need to preserve the locked status to reinstate it later
    bool locked = parentHeap->tstDisableAllocation();
    if (locked) {
        parentHeap->enableAllocation();
    }

    ExpHeap *newHeap = ExpHeap::Create(-1, parentHeap, s_heapOptionFlg);
    s_heapForCreateScene = newHeap;

    if (locked) {
        parentHeap->disableAllocation();
    }

    newHeap->becomeCurrentHeap();
    newHeap->setName("DefaultSceneHeap");

    Scene *newScene = m_creator->create(id);

    if (parent) {
        parent->setChild(newScene);
    }

    m_currentScene = newScene;
    newScene->setId(id);
    newScene->setParent(parent);
    newScene->setSceneMgr(this);
    newScene->enter();
}

/// @addr{0x8023B3F0}
/// @brief Destroys the specified scene and its children
/// @param scene The scene to destroy
/// @details Calls @ref Scene::exit() on the scene, recursively destroys its children, and then
/// deletes the scene and its heap. Updates @ref m_currentScene accordingly and changes @ref
/// s_currentHeap to the parent heap (or @ref s_rootHeap if there is no parent)
void SceneManager::destroyScene(Scene *scene) {
    scene->exit();
    if (scene->child()) {
        destroyScene(scene->child());
    }

    Scene *parent = scene->parent();
    m_creator->destroy(scene->id());

    // ADDED: The original library does not actually delete the scene, but we run into leaks
    // So, we delete the scene that's provided to the function
    auto *heap = scene->heap();
    EGG::egg_delete(scene);

    m_currentScene = nullptr;

    if (parent) {
        parent->setChild(nullptr);
        m_currentScene = parent;
    }

    heap->destroy();
    Heap *parentHeap = parent ? parent->heap() : s_rootHeap;
    parentHeap->becomeCurrentHeap();
}

/// @addr{0x8023AF84}
/// @brief Destroys the current scene and changes to a new scene
/// @details Destroys the current scene and changes to a new scene with the specified ID
/// @param nextSceneId The ID of the next scene to change to
void SceneManager::changeScene(int nextSceneId) {
    while (m_currentScene) {
        destroyCurrentSceneNoIncoming(true);
    }

    changeSiblingScene(nextSceneId);
}

/// @addr{0x8023AFE0}
/// @brief Changes to a new scene with the specified ID
/// @param nextSceneId The ID of the next scene to change to
void SceneManager::changeSiblingScene(int nextSceneId) {
    setNextSceneId(nextSceneId);
    changeSiblingScene();
}

/// @addr{0x8023B064}
/// @brief Changes to another scene
/// @details Destroys the current scene and creates a new scene whose ID is @ref m_nextSceneId with
/// the same parent as the current scene.
void SceneManager::changeSiblingScene() {
    Scene *parentScene = m_currentScene ? m_currentScene->parent() : nullptr;

    if (m_currentScene) {
        destroyScene(m_currentScene);
        m_currentScene = nullptr;
    }

    int nextSceneId = m_nextSceneId;
    setupNextSceneId();
    createScene(nextSceneId, parentScene);
}

/// @addr{0x8023AEF8}
/// @brief Reinitializes the current scene
/// @details Calls @ref Scene::reinit() on the current scene if it exists
void SceneManager::reinitCurrentScene() {
    if (m_currentScene) {
        m_currentScene->reinit();
    }
}

/// @addr{0x8023B2AC}
/// @brief Destroys the current scene without considering incoming scenes
/// @param destroyRootIfNoParent Whether to destroy the root scene if it has no parent
/// @return `true` if a scene was destroyed, `false` otherwise
bool SceneManager::destroyCurrentSceneNoIncoming(bool destroyRootIfNoParent) {
    if (!m_currentScene) {
        return false;
    }

    Scene *parent = m_currentScene->parent();
    if (parent) {
        destroyScene(parent->child());
        setNextSceneId(parent->id());
        setupNextSceneId();
        return true;
    } else {
        if (destroyRootIfNoParent) {
            destroyScene(m_currentScene);
            setNextSceneId(-1);
            setupNextSceneId();
        }
        return false;
    }
}

/// @addr{0x8023B344}
/// @brief Destroys scenes until the specified scene ID is reached
/// @param id The ID of the scene to stop at
/// @return `true` if the parent scene with the specified ID was found and destruction occurred,
/// `false` otherwise
/// @todo This is unused in Kinoko - should we scrap?
bool SceneManager::destroyToSelectSceneId(int id) {
    Scene *parent = findParentScene(id);
    if (!parent) {
        return false;
    }

    while (parent->id() != currentSceneId()) {
        destroyCurrentSceneNoIncoming(false);
    }
    return true;
}

/// @addr{0x8023B940}
/// @brief Finds the parent scene of a given scene ID
/// @param id The ID of the scene whose parent is to be found
/// @return A pointer to the parent scene if found, `nullptr` otherwise
Scene *SceneManager::findParentScene(int id) const {
    Scene *scene = m_currentScene->parent();
    for (; scene; scene = scene->parent()) {
        if (scene->id() == id) {
            break;
        }
    }

    return scene;
}

/// @addr{0x8023B910}
/// @brief Sets up the next scene ID by updating the previous, current, and next scene IDs
/// accordingly
/// @details Updates the previous scene ID to the current scene ID, the current scene ID to the next
/// scene ID, and resets the next scene ID to -1.
void SceneManager::setupNextSceneId() {
    m_prevSceneId = m_currentSceneId;
    m_currentSceneId = m_nextSceneId;
    m_nextSceneId = -1;
}

/// @brief Gets the current scene
/// @return A pointer to the current scene, @ref m_currentScene
Scene *SceneManager::currentScene() const {
    return m_currentScene;
}

/// @brief Gets the ID of the current scene
/// @return The ID of the current scene, @ref m_currentSceneId
int SceneManager::currentSceneId() const {
    return m_currentSceneId;
}

/// @brief Gets the heap used for creating scenes
/// @return A pointer to the heap used for creating scenes, @ref s_heapForCreateScene
Heap *SceneManager::heapForCreateScene() {
    return s_heapForCreateScene;
}

/// @brief Sets the next scene ID
/// @param id The ID to set as the next scene ID
void SceneManager::setNextSceneId(int id) {
    m_nextSceneId = id;
}

/// @brief Sets the root heap for scene management
/// @param heap The heap to set as the root heap
void SceneManager::SetRootHeap(Heap *heap) {
    s_rootHeap = heap;
}

Heap *SceneManager::s_heapForCreateScene = nullptr;
u16 SceneManager::s_heapOptionFlg = Abstract::Memory::DEFAULT_OPT;

Heap *SceneManager::s_rootHeap = nullptr;

} // namespace Kinoko::EGG
