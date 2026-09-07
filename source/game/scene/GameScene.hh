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
    /// @brief Performs per-frame updates for the scene, including input handling, updating all
    /// karts' physics, and updating the cameras
    void calc() final {
        System::KPadDirector::Instance()->calc();
        calcEngines();
        calcCamera();
    }

    /// @addr{0x8051AB58}
    /// @copybrief EGG::Scene::enter()
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
    /// @brief Reinitializes the scene, either by reentering it or switching to the next scene
    void reinit() final {
        exit();
        if (m_nextSceneId < 0) {
            onReinit();
            initScene();
        } else {
            m_sceneMgr->changeSiblingScene(m_nextSceneId);
        }
    }

    /// @brief Pure virtual function responsible for creating all singleton instances necessary for
    /// the scene to function
    virtual void createEngines() = 0;

    /// @brief Pure virtual function that initializes all singletons and their subsystems
    virtual void initEngines() = 0;

    /// @brief Pure virtual function that performs per-frame updates for all singletons and their
    /// subsystems
    virtual void calcEngines() = 0;

    /// @brief Pure virtual function that destroys all singletons and their subsystems
    virtual void destroyEngines() = 0;

    /// @brief Pure virtual function that performs initial setup for the scene
    virtual void configure() = 0;

    /// @brief Called when the scene is reinitialized, allowing for custom behavior during reentry
    virtual void onReinit() {}

    static void initCamera();
    static void calcCamera();

protected:
    /// @addr{0x8051AA58}
    /// @brief Appends a resource to the scene's resource list so that the underlying archive can be
    /// properly destroyed when the refcount is 0
    /// @param archive Pointer to the archive containing the resource to be appended
    /// @param id The ID of the resource within the archive
    void appendResource(System::MultiDvdArchive *archive, s32 id) {
        m_resources.push_back(EGG::egg_new<Resource>(archive, id));
    }

private:
    /// @brief Represents a resource managed by the scene, including its archive and a unique ID
    struct Resource {
        Resource(System::MultiDvdArchive *archive, s32 id);

        /// @brief Default destructor
        ~Resource() = default;

        System::MultiDvdArchive *archive; ///< Pointer to the archive containing the resource
        s32 id;                           ///< Unique ID of the resource within the archive
    };

    /// @addr{0x8051A4DC}
    /// @brief Initializes the scene by creating and initializing all singletons and resetting the
    /// player's input state
    void initScene() {
        createEngines();
        System::KPadDirector::Instance()->reset();
        initEngines();
#ifdef BUILD_DEBUG
        checkMemory();
#endif // BUILD_DEBUG
    }

    /// @addr{0x8051B0F4}
    /// @brief If not transitioning to another scene, destroys all singletons and clears the
    /// player's input state
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

    alloc_list<Resource *> m_resources; ///< List of all active resources in the scene
    int m_nextSceneId; ///< ID of the next scene to transition to, or -1 if no transition

#ifdef BUILD_DEBUG
    EGG::ExpHeap::GroupSizeRecord m_groupSizeRecord;
    size_t m_totalMemoryUsed;
#endif // BUILD_DEBUG
};

} // namespace Kinoko::Scene
