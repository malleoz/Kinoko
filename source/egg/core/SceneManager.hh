#pragma once

#include "egg/core/Scene.hh"
#include "egg/core/SceneCreator.hh"

namespace Kinoko {

namespace Host {

class Context;

} // namespace Host

namespace EGG {

/// @brief Manages the scene stack and transitions between scenes.
class SceneManager {
    /// @brief Grants access to the singleton so a @ref Host::Context can restore the instance's
    /// state on context switch
    friend class Host::Context;

public:
    /*------------*
        Lifecycle
     *------------*/

    /// @addr{0x8023ADDC}
    SceneManager(SceneCreator *creator) {
        m_creator = creator;
        m_currentScene = nullptr;
    }

    virtual ~SceneManager() = default;

    /*----------*
        Virtual
     *----------*/

    /// @addr{0x8023AE60}
    virtual void calc() {
        calcCurrentScene();
    }

    /// @addr{0x8023B588}
    virtual void calcCurrentScene() {
        if (!m_currentScene) {
            return;
        }

        m_currentScene->calc();
    }

    /*----------*
        Methods
     *----------*/

    /// @addr{0x8023B248}
    void createChildScene(int id, Scene *parent) {
        outgoingParentScene(parent);
        setNextSceneId(id);
        setupNextSceneId();
        createScene(id, parent);
    }

    void createScene(int nextSceneId, Scene *parent);
    void destroyScene(Scene *scene);

    /// @addr{0x8023AF84}
    void changeScene(int nextSceneId) {
        while (m_currentScene) {
            destroyCurrentSceneNoIncoming(true);
        }

        changeSiblingScene(nextSceneId);
    }

    /// @addr{0x8023AFE0}
    void changeSiblingScene(int nextSceneId) {
        setNextSceneId(nextSceneId);
        changeSiblingScene();
    }

    void changeSiblingScene();

    /// @addr{0x8023B568}
    void incomingCurrentScene() {
        if (m_currentScene) {
            m_currentScene->incoming_childDestroy();
        }
    }

    /// @addr{0x8023B92C}
    void outgoingParentScene(Scene *parent) {
        parent->outgoing_childCreate();
    }

    /// @addr{0x8023AEF8}
    void reinitCurrentScene() {
        if (m_currentScene) {
            m_currentScene->reinit();
        }
    }

    bool destroyCurrentSceneNoIncoming(bool destroyRootIfNoParent);
    bool destroyToSelectSceneId(int nextSceneId);
    [[nodiscard]] Scene *findParentScene(int id) const;

    /// @addr{0x8023B910}
    void setupNextSceneId() {
        m_prevSceneId = m_currentSceneId;
        m_currentSceneId = m_nextSceneId;
        m_nextSceneId = -1;
    }

    /*----------*
        Getters
     *----------*/

    [[nodiscard]] Scene *currentScene() const {
        return m_currentScene;
    }

    [[nodiscard]] int currentSceneId() const {
        return m_currentSceneId;
    }

    [[nodiscard]] static Heap *heapForCreateScene() {
        return s_heapForCreateScene;
    }

    /*----------*
        Setters
     *----------*/

    void setNextSceneId(int id) {
        m_nextSceneId = id;
    }

    static void SetRootHeap(Heap *heap) {
        s_rootHeap = heap;
    }

private:
    /*----------*
        Members
     *----------*/

    SceneCreator *m_creator;
    Scene *m_currentScene;
    int m_nextSceneId;
    int m_currentSceneId;
    int m_prevSceneId;

    static Heap *s_heapForCreateScene;
    static u16 s_heapOptionFlg;

    static Heap *s_rootHeap;
};

} // namespace EGG

} // namespace Kinoko
