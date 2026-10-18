#pragma once

#include <Common.hh>

namespace Kinoko {

namespace Host {

class Context;

} // namespace Host

namespace EGG {

class Heap;
class Scene;
class SceneCreator;

/// @brief Manages the scene stack and transitions between scenes.
class SceneManager {
    /// @brief Grants access to the singleton so a @ref Host::Context can restore the instance's
    /// state on context switch
    friend Host::Context;

public:
    SceneManager(SceneCreator *creator);
    virtual ~SceneManager();

    virtual void calc();
    virtual void calcCurrentScene();

    void createChildScene(int id, Scene *parent);
    void createScene(int nextSceneId, Scene *parent);
    void destroyScene(Scene *scene);
    void changeScene(int nextSceneId);
    void changeSiblingScene(int nextSceneId);
    void changeSiblingScene();
    void incomingCurrentScene();
    void outgoingParentScene(Scene *parent);
    void reinitCurrentScene();
    bool destroyCurrentSceneNoIncoming(bool destroyRootIfNoParent);
    bool destroyToSelectSceneId(int nextSceneId);
    [[nodiscard]] Scene *findParentScene(int id) const;
    void setupNextSceneId();

    /// @beginGetters

    [[nodiscard]] Scene *currentScene() const;
    [[nodiscard]] int currentSceneId() const;
    [[nodiscard]] static Heap *heapForCreateScene();

    /// @endGetters

    /// @beginSetters

    void setNextSceneId(int id);
    static void SetRootHeap(Heap *heap);

    /// @endSetters

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
