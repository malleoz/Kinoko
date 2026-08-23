#pragma once

#include "game/system/MultiDvdArchive.hh"

#include "game/system/KPadDirector.hh"

#include <egg/core/ExpHeap.hh>
#include <egg/core/SceneManager.hh>

/// @brief Pertains to scene handling.
namespace Kinoko::Scene {

/// @brief Interface for menu and race scenes.
class GameScene : public EGG::Scene {
public:
    GameScene();
    ~GameScene() override;

    /// @addr{0x8051B3C8}
    void calc() final {
        System::KPadDirector::Instance()->calc();
        calcEngines();
        calcCamera();
    }

    /// @addr{0x8051AB58}
    void enter() final {
        configure();
        initScene();
    }

    /// @addr{0x8051B250}
    void exit() final {
        deinitScene();
        unmountResources();
    }

    /// @addr{0x8051B7B0}
    void reinit() final {
        exit();
        if (m_nextSceneId < 0) {
            onReinit();
            initScene();
        } else {
            m_sceneMgr->changeSiblingScene(m_nextSceneId);
        }
    }

    virtual void createEngines() = 0;
    virtual void initEngines() = 0;
    virtual void calcEngines() = 0;
    virtual void destroyEngines() = 0;
    virtual void configure() = 0;
    virtual void onReinit() {}

    static void initCamera();
    static void calcCamera();

protected:
    /// @addr{0x8051AA58}
    void appendResource(System::MultiDvdArchive *archive, s32 id) {
        m_resources.push_back(EGG::egg_new<Resource>(archive, id));
    }

private:
    struct Resource {
        Resource(System::MultiDvdArchive *archive, s32 id);

        System::MultiDvdArchive *archive;
        s32 id;
    };

    /// @addr{0x8051A4DC}
    void initScene() {
        createEngines();
        System::KPadDirector::Instance()->reset();
        initEngines();
#ifdef BUILD_DEBUG
        checkMemory();
#endif // BUILD_DEBUG
    }

    /// @addr{0x8051B0F4}
    void deinitScene() {
        if (m_nextSceneId >= 0) {
            return;
        }

        destroyEngines();
        System::KPadDirector::Instance()->clear();
    }

    void unmountResources();

#ifdef BUILD_DEBUG
    void checkMemory();
    void getMemoryLeakTags();
    size_t getMemoryLeakTagCount();

    static void ViewTags(void *block, Abstract::Memory::MEMiHeapHead *heap, uintptr_t param);
    static void IncreaseTagCount(void *block, Abstract::Memory::MEMiHeapHead *heap,
            uintptr_t param);
#endif // BUILD_DEBUG

    EGG::ExpHeap::GroupSizeRecord m_groupSizeRecord;
    alloc_list<Resource *> m_resources; ///< List of all active resources in the scene.
    int m_nextSceneId;

    [[maybe_unused]] size_t m_totalMemoryUsed;
};

} // namespace Kinoko::Scene
