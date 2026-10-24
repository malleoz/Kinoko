#pragma once

#include <Types.hh>

#include "egg/core/Scene.hh"

#ifdef BUILD_DEBUG
#include <egg/core/ExpHeap.hh>
#endif

/// @brief Pertains to scene handling.
namespace Kinoko {

namespace System {

class MultiDvdArchive;

} // namespace System

namespace Scene {

/// @brief Interface for menu and race scenes.
class GameScene : public EGG::Scene {
public:
    GameScene();
    ~GameScene() override;

    void calc() final;
    void enter() final;
    void exit() final;
    void reinit() final;

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

    virtual void onReinit();

    static void InitCamera();
    static void CalcCamera();

protected:
    void appendResource(System::MultiDvdArchive *archive, s32 id);

private:
    /// @brief Represents a resource managed by the scene, including its archive and a unique ID
    struct Resource {
        System::MultiDvdArchive *archive; ///< Pointer to the archive containing the resource
        s32 id;                           ///< Unique ID of the resource within the archive

        /// @addr{Inlined in 0x8051AA58}
        /// @brief Constructor
        Resource(System::MultiDvdArchive *archive, s32 id) : archive(archive), id(id) {}

        /// @brief Default destructor
        ~Resource() = default;
    };

    void initScene();
    void deinitScene();
    void unmountResources();

#ifdef BUILD_DEBUG
    void checkMemory();
    void getMemoryLeakTags();
    [[nodiscard]] size_t getMemoryLeakTagCount();

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

} // namespace Scene

} // namespace Kinoko
