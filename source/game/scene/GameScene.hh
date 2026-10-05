#pragma once

#include "game/render/KartCamera.hh"

#include "game/system/KPadDirector.hh"
#include "game/system/MultiDvdArchive.hh"

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
    /// @copybrief EGG::Scene::calc()
    /// @details Manages input handling, updates all karts' physics, and updates the cameras.
    void calc() final {
        System::KPadDirector::Instance()->calc();
        calcEngines();
        calcCamera();
    }

    /// @addr{0x8051AB58}
    /// @copybrief EGG::Scene::enter()
    /// @details Calls @ref configure() and @ref initScene() to initialize the race.
    void enter() final {
        configure();
        initScene();
    }

    /// @addr{0x8051B250}
    /// @copybrief EGG::Scene::exit()
    /// @details Calls @ref deinitScene() and @ref unmountResources() to properly clean up at the
    /// end of a race.
    void exit() final {
        deinitScene();
        unmountResources();
    }

    /// @addr{0x8051B7B0}
    /// @copybrief EGG::Scene::reinit()
    /// @details Calls @ref exit() and reinitializes the scene, either by reentering it via @ref
    /// onReinit() or switching to the next scene.
    void reinit() final {
        exit();
        if (m_nextSceneId < 0) {
            onReinit();
            initScene();
        } else {
            m_sceneMgr->changeSiblingScene(m_nextSceneId);
        }
    }

    /// @brief Responsible for creating all singleton instances necessary for
    /// the scene to function
    virtual void createEngines() = 0;

    /// @brief Initializes all singletons and their subsystems
    virtual void initEngines() = 0;

    /// @brief Performs per-frame updates for all singletons and their
    /// subsystems
    virtual void calcEngines() = 0;

    /// @brief Destroys all singletons and their subsystems
    virtual void destroyEngines() = 0;

    /// @brief Performs initial setup for the scene
    virtual void configure() = 0;

    /// @brief Called when the scene is reinitialized, allowing for custom behavior during reentry
    virtual void onReinit() {}

    /// @addr{0x805A1A8C}
    /// @brief Initializes the camera
    /// @details In the base game, this function normally has to iterate over the cameras for all
    /// players. For Kinoko, we simplify for one player by exposing @ref Render::KartCamera as a
    /// singleton.
    static void initCamera() {
        Render::KartCamera::Instance()->init();
    }

    /// @addr{0x805A1AF0}
    /// @brief Every frame, updates the cameras' positions and orientations
    /// @details In the base game, this function normally has to iterate over the cameras for all
    /// players. For Kinoko, we simplify for one player by exposing @ref Render::KartCamera as a
    /// singleton.
    static void calcCamera() {
        Render::KartCamera::Instance()->calc();
    }

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
        /// @addr{Inlined in 0x8051AA58}
        /// @brief Constructor
        Resource(System::MultiDvdArchive *archive, s32 id) : archive(archive), id(id) {}

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
    /// @brief Called upon exiting the scene in order to clean up singletons used mid-race
    /// @details If not transitioning to another scene, destroys all singletons and clears the
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
