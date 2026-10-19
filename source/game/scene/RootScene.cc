#include "RootScene.hh"

#include "game/system/KPadDirector.hh"
#include "game/system/RaceConfig.hh"
#include "game/system/ResourceManager.hh"

#include <egg/core/SceneManager.hh>

#include <ScopeLock.hh>

namespace Kinoko::Scene {

/// @addr{0x80542878}
/// @brief Constructor
/// @details Sets the @ref m_heap name to `"RootSceneHeap"` for debugging/logging purposes.
RootScene::RootScene() {
    m_heap->setName("RootSceneHeap");
}

/// @addr{0x805429A8}
/// @brief Default virtual destructor
RootScene::~RootScene() = default;

/// @addr{0x80543B84}
/// @copybrief EGG::Scene::enter()
/// @details Allocates and initializes the root singletons via @ref allocate() and @ref init().
/// Finally, creates the child race scene (@ref RaceScene).
void RootScene::enter() {
    allocate();
    init();
#ifdef BUILD_DEBUG
    checkMemory();
#endif // BUILD_DEBUG
    m_sceneMgr->createChildScene(static_cast<int>(Host::SceneId::Race), this);
}

/// @addr{0x80542D4C}
/// @brief Creates singleton instances for @ref System::ResourceManager, @ref System::KPadDirector,
/// and @ref System::RaceConfig
/// @details Singleton creation is grouped using a `ScopeLock` for each relevant @ref GroupID
/// to make it easier to track memory allocation for debugging purposes.
void RootScene::allocate() {
    {
        ScopeLock<GroupID> lock(GroupID::Resource);
        System::ResourceManager::CreateInstance();
    }

    {
        ScopeLock<GroupID> lock(GroupID::Race);
        System::KPadDirector::CreateInstance();
        System::RaceConfig::CreateInstance();
    }
}

/// @addr{0x805438B4}
/// @brief Initializes the @ref System::RaceConfig singleton instance
/// @details Singleton initialization is grouped using a `ScopeLock` for each relevant @ref GroupID
/// to make it easier to track memory allocation for debugging purposes.
void RootScene::init() {
    {
        ScopeLock<GroupID> lock(GroupID::Race);
        System::RaceConfig::Instance()->init();
    }
}

#ifdef BUILD_DEBUG
void RootScene::checkMemory() {
    EGG::ExpHeap *heap = EGG::Heap::DynamicCastToExp(m_heap);
    ASSERT(heap);

    heap->calcGroupSize(&m_groupSizeRecord);
    size_t defaultSize = m_groupSizeRecord.getGroupSize(static_cast<u16>(GroupID::None));

    // The scene is the first, always group ID 0 allocation to happen in the scene's heap
    ASSERT(defaultSize >= sizeof(RootScene));
    defaultSize -= sizeof(RootScene);
    if (defaultSize > 0) {
        WARN("Default memory usage found! %zu bytes", defaultSize);
    }

    for (u16 groupID = 1; groupID < m_groupSizeRecord.size(); ++groupID) {
        size_t size = m_groupSizeRecord.getGroupSize(groupID);
        if (size == 0) {
            continue;
        }

        DEBUG("Group ID %d: Allocated %zu bytes", groupID, size);
    }
}
#endif // BUILD_DEBUG

} // namespace Kinoko::Scene
