#include "GameScene.hh"

#include "game/system/ResourceManager.hh"

#include "game/render/KartCamera.hh"
namespace Kinoko::Scene {

/// @addr{0x8051A1E0}
/// @brief Constructor
GameScene::GameScene() {
    m_heap->setName("DefaultGameSceneHeap");
    m_nextSceneId = -1;

#ifdef BUILD_DEBUG
    m_totalMemoryUsed = 0;
    EGG::ExpHeap *heap = EGG::Heap::dynamicCastToExp(m_heap);
    ASSERT(heap);

    heap->calcGroupSize(&m_groupSizeRecord);
    for (u16 groupID = 0; groupID < m_groupSizeRecord.size(); ++groupID) {
        m_totalMemoryUsed += m_groupSizeRecord.getGroupSize(groupID);
    }
#endif // BUILD_DEBUG
}

/// @addr{0x8051A3C0}
/// @brief Virtual destructor
GameScene::~GameScene() {
    m_resources.clear();

#ifdef BUILD_DEBUG
    EGG::ExpHeap *heap = EGG::Heap::dynamicCastToExp(m_heap);
    ASSERT(heap);

    heap->calcGroupSize(&m_groupSizeRecord);
    size_t sum = 0;
    for (u16 groupID = 0; groupID < m_groupSizeRecord.size(); ++groupID) {
        sum += m_groupSizeRecord.getGroupSize(groupID);
    }

    if (sum > m_totalMemoryUsed) {
        WARN("MEMORY LEAK DETECTED: %zu bytes", sum - m_totalMemoryUsed);
        getMemoryLeakTags();
    }
#endif // BUILD_DEBUG
}

/// @addr{0x805A1A8C}
/// @brief Initializes the camera
void GameScene::initCamera() {
    Render::KartCamera::Instance()->init();
}

/// @addr{0x805A1AF0}
/// @brief Every frame, updates the cameras' positions and orientations
void GameScene::calcCamera() {
    Render::KartCamera::Instance()->calc();
}

/// @addr{Inlined in 0x8051AA58}
/// @brief Constructor
GameScene::Resource::Resource(System::MultiDvdArchive *archive, s32 id)
    : archive(archive), id(id) {}

/// @addr{0x8051AAE8}
/// @brief Decrements the refcounter for each managed archive and clears the resource list
void GameScene::unmountResources() {
    auto *resourceManager = System::ResourceManager::Instance();
    for (auto iter = m_resources.begin(); iter != m_resources.end();) {
        Resource *resource = *iter;
        resourceManager->unmount(resource->archive);
        iter = m_resources.erase(iter);
        EGG::egg_delete(resource);
    }
}

#ifdef BUILD_DEBUG
void GameScene::checkMemory() {
    EGG::ExpHeap *heap = EGG::Heap::dynamicCastToExp(m_heap);
    ASSERT(heap);

    heap->calcGroupSize(&m_groupSizeRecord);
    size_t defaultSize = m_groupSizeRecord.getGroupSize(static_cast<u16>(GroupID::None));

    // Because we're working with a class that can be inherited (but not doubly-inherited),
    // it's possible that derived classes can have a different size compared to GameScene
    // We don't need to know the exact derived class, though, as we have the memory block head
    // The scene is the first, always group ID 0 allocation to happen in the scene's heap
    Abstract::Memory::MEMiExpBlockHead *blockHead =
            static_cast<Abstract::Memory::MEMiExpBlockHead *>(
                    SubOffset(this, sizeof(Abstract::Memory::MEMiExpBlockHead)));
    size_t initialAllocSize = blockHead->m_size;
    ASSERT(defaultSize >= initialAllocSize);

    defaultSize -= initialAllocSize;
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

void GameScene::getMemoryLeakTags() {
    size_t count = getMemoryLeakTagCount();
    if (count == 0) {
        return;
    }

    owning_span<u32> tags = owning_span<u32>(count);
    for (auto &tag : tags) {
        tag = 0; // empty tag
    }

    EGG::Heap::dynamicCastToExp(m_heap)->dynamicCastHandleToExp()->visitAllocated(ViewTags,
            GetAddrNum(&tags));

    DEBUG("TAGGED MEMORY BLOCKS:");
    printf("[%d", tags[0]);
    for (u32 i = 1; i < tags.size(); ++i) {
        printf(", %d", tags[i]);
    }
    printf("]\n");
}

size_t GameScene::getMemoryLeakTagCount() {
    size_t count = 0;
    EGG::Heap::dynamicCastToExp(m_heap)->dynamicCastHandleToExp()->visitAllocated(IncreaseTagCount,
            GetAddrNum(&count));
    return count;
}

void GameScene::ViewTags(void *block, Abstract::Memory::MEMiHeapHead * /*heap*/, uintptr_t param) {
    Abstract::Memory::MEMiExpBlockHead *blockHead =
            static_cast<Abstract::Memory::MEMiExpBlockHead *>(
                    SubOffset(block, sizeof(Abstract::Memory::MEMiExpBlockHead)));
    owning_span<u32> *span = reinterpret_cast<owning_span<u32> *>(param);
    for (auto &tag : *span) {
        if (tag == 0) {
            tag = blockHead->m_tag;
            break;
        }
    }
}

void GameScene::IncreaseTagCount(void * /*block*/, Abstract::Memory::MEMiHeapHead * /*heap*/,
        uintptr_t param) {
    size_t &count = *reinterpret_cast<size_t *>(param);
    ++count;
}
#endif // BUILD_DEBUG

} // namespace Kinoko::Scene
