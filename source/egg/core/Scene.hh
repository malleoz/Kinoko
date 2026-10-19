#pragma once

#include <Common.hh>

#include "egg/core/Disposer.hh"

namespace Kinoko::EGG {

class SceneManager;

/// @brief Base class for all scenes.
/// @details A scene compartmentalizes certain elements of the game, such as the menu, the race, the
/// credits, etc. This allows for easy cleanup when transitioning to a different scene, as scenes
/// destroy instances of classes used only during the scene lifetime. While this is not an abstract
/// class, you'll never see an instance of this base class.
class Scene : Disposer {
public:
    Scene();
    virtual ~Scene();

    virtual void calc();
    virtual void enter();
    virtual void exit();
    virtual void reinit();

    /// @beginSetters

    void setParent(Scene *parent);
    void setChild(Scene *child);
    void setId(int id);
    void setSceneMgr(SceneManager *sceneMgr);

    /// @endSetters

    /// @beginGetters

    [[nodiscard]] Heap *heap() const;
    [[nodiscard]] Scene *parent() const;
    [[nodiscard]] Scene *child() const;
    [[nodiscard]] int id() const;
    [[nodiscard]] SceneManager *sceneMgr() const;

    /// @endGetters

protected:
    Heap *const m_heap;       ///< The heap associated with this scene
    Scene *m_parent;          ///< The parent scene, or `nullptr` if this is the root scene
    Scene *m_child;           ///< The child scene, or `nullptr` if there is no child
    int m_id;                 ///< The ID of the scene
    SceneManager *m_sceneMgr; ///< The scene manager associated with this scene
};

} // namespace Kinoko::EGG
