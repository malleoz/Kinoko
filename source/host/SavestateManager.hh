#pragma once

#include "Common.hh"

#include <game/system/KPadController.hh>

#include <filesystem>
#include <functional>
#include <list>

class KSystem;

namespace Abstract::Memory {
class MEMList;
}

namespace EGG {
class Archive;
class Heap;
class SceneManager;
} // namespace EGG

namespace Field {
class BoxColManager;
class CollisionDirector;
class CourseColMgr;
class ObjectCollisionBase;
class ObjectDirector;
class ObjectDrivableDirector;
class RailManager;
} // namespace Field

namespace Item {
class ItemDirector;
}

namespace Kart {
class KartObjectManager;
class KartParamFileManager;
} // namespace Kart

namespace System {
class CourseMap;
class KPadDirector;
class KPadHostController;
class RaceConfig;
typedef std::function<void(RaceConfig *, void *)> InitCallback;
class RaceInputState;
class RaceManager;
class ResourceManager;
class KSystem;
} // namespace System

/// @brief Manages the serialization and deserialization of input sequences.
class SavestateManager {
private:
    static constexpr size_t SLOTS = 10;
    static constexpr u32 KSAV_SIGNATURE = 0x4b534156; // KSAV

    static constexpr size_t CORE_BLOCK_IDX = SLOTS + 0;
    static constexpr size_t AUX_BLOCK_IDX = SLOTS + 1;

public:
    SavestateManager();
    ~SavestateManager();

    void calcState();
    void saveState(size_t slot);
    void loadState(size_t slot);

    void saveContext(size_t slot);
    void loadContext(size_t slot);

    void setRaceConfigFromParams(u32 params);

    void setSceneMgr(EGG::SceneManager *sceneMgr) {
        m_sceneMgr = sceneMgr;
    }

    void setController(System::KPadHostController *controller) {
        m_controller = controller;
    }

private:
    struct Context {
        std::optional<Abstract::Memory::MEMList> m_rootList;
        std::optional<Abstract::Memory::MEMList> m_archiveList;
        std::optional<Abstract::Memory::MEMList> m_heapList;
        EGG::Heap *m_currentHeap;
        // EGG::Heap *m_allocatableHeap; // Unused
        EGG::Heap *m_heapForCreateScene;
        Field::BoxColManager *m_boxColMgr;
        Field::CollisionDirector *m_colDir;
        Field::CourseColMgr *m_courseColMgr;
        Field::ObjectDirector *m_objDir;
        Field::ObjectDrivableDirector *m_objDrivableDir;
        Field::RailManager *m_railMgr;
        Item::ItemDirector *m_itemDir;
        Kart::KartObjectManager *m_kartObjMgr;
        Kart::KartParamFileManager *m_paramFileMgr;
        System::CourseMap *m_courseMap;
        System::KPadDirector *m_padDir;
        System::RaceConfig *m_raceConfig;
        System::InitCallback m_onInitCallback;
        void *m_onInitCallbackArg;
        System::RaceManager *m_raceMgr;
        System::ResourceManager *m_resMgr;
    };

    void flushBufferToFile();

    void initMemory();

    std::optional<SavestateManager::Context> &getContext(size_t slot);
    void setContext(size_t slot);

    std::array<u16, 128> m_bufferedState;
    size_t m_statesBuffered;

    std::array<std::filesystem::path, SLOTS> m_files;
    System::KPadHostController *m_controller;
    EGG::SceneManager *m_sceneMgr;

    void *m_coreBlock;
    void *m_auxBlock;
    std::array<void *, SLOTS> m_contextBlocks;

    std::array<std::optional<Context>, SLOTS + 2> m_contexts;

    const char *STATE_FILE = "StateBuffer.dat";

    static std::filesystem::path GetFilePathForSlot(size_t slot);
    static u32 GetParamsFromRaceConfig();
};
