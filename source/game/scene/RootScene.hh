#pragma once

#include <egg/core/ExpHeap.hh>
#include <egg/core/SceneManager.hh>

#include <host/SceneId.hh>

namespace Kinoko::Scene {

/// @brief The parent scene for all other scenes
class RootScene final : public EGG::Scene {
public:
    RootScene();
    ~RootScene() override;

    /// @addr{0x80543B84}
    /// @copybrief EGG::Scene::enter()
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
