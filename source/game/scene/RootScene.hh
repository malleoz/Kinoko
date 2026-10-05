#pragma once

#include <egg/core/ExpHeap.hh>
#include <egg/core/SceneManager.hh>

#include <host/SceneId.hh>

namespace Kinoko::Scene {

/// @brief The parent scene for all other scenes
class RootScene final : public EGG::Scene {
public:
    /// @addr{0x80542878}
    /// @brief Constructor
    /// @details Sets the @ref m_heap name to `"RootSceneHeap"` for debugging/logging purposes.
    RootScene() {
        m_heap->setName("RootSceneHeap");
    }

    /// @addr{0x805429A8}
    /// @brief Default virtual destructor
    ~RootScene() override = default;

    /// @addr{0x80543B84}
    /// @copybrief EGG::Scene::enter()
    /// @details Allocates and initializes the root singletons via @ref allocate() and @ref init().
    /// Finally, creates the child race scene (@ref RaceScene).
    void enter() override {
        allocate();
        init();
#ifdef BUILD_DEBUG
        checkMemory();
#endif // BUILD_DEBUG
        m_sceneMgr->createChildScene(static_cast<int>(Host::SceneId::Race), this);
    }

private:
    void allocate();
    void init();

#ifdef BUILD_DEBUG
    void checkMemory();

    EGG::ExpHeap::GroupSizeRecord m_groupSizeRecord;
#endif // BUILD_DEBUG
};

} // namespace Kinoko::Scene
