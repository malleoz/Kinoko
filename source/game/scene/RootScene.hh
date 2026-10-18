#pragma once

#include <egg/core/Scene.hh>

#include <host/SceneId.hh>

#ifdef BUILD_DEBUG
#include <egg/core/ExpHeap.hh>
#endif

namespace Kinoko::Scene {

/// @brief The parent scene for all other scenes
class RootScene final : public EGG::Scene {
public:
    RootScene();
    ~RootScene() override;

    void enter() override;

private:
    void allocate();
    void init();

#ifdef BUILD_DEBUG
    void checkMemory();

    EGG::ExpHeap::GroupSizeRecord m_groupSizeRecord;
#endif // BUILD_DEBUG
};

} // namespace Kinoko::Scene
