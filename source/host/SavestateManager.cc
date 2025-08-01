#include "SavestateManager.hh"

#include <abstract/memory/HeapCommon.hh>

#include <egg/core/Archive.hh>
#include <egg/core/Heap.hh>
#include <egg/core/SceneManager.hh>

#include <game/field/BoxColManager.hh>
#include <game/field/CollisionDirector.hh>
#include <game/field/ObjectDirector.hh>
#include <game/field/ObjectDrivableDirector.hh>
#include <game/field/RailManager.hh>

#include <game/item/ItemDirector.hh>

#include <game/kart/KartObjectManager.hh>
#include <game/kart/KartParamFileManager.hh>

#include <game/system/CourseMap.hh>
#include <game/system/KPadDirector.hh>
#include <game/system/RaceConfig.hh>
#include <game/system/RaceManager.hh>

#include <fstream>

SavestateManager::SavestateManager()
    : m_bufferedState{}, m_statesBuffered(0), m_controller(nullptr), m_sceneMgr(nullptr),
      m_contexts({}) {
    // Populate filename array
    for (size_t i = 0; i < m_files.size(); ++i) {
        m_files[i] = GetFilePathForSlot(i);
    }

    // Flush the state file so we aren't appending to an older session's file.
    std::ofstream outFile(STATE_FILE, std::ios::trunc);
    if (!outFile) {
        PANIC("Could not open %s!", STATE_FILE);
    }

    initMemory();
}

SavestateManager::~SavestateManager() = default;

/// @brief Saves the current input state to a buffer. When full, flushes the buffer to file.
void SavestateManager::calcState() {
    const auto &state = m_controller->raceInputState();

    u16 face = state.buttons & 0xf;
    u16 trick = static_cast<u8>(state.trick);
    u8 x = System::StateToRawStick(state.stick.x);
    u8 y = System::StateToRawStick(state.stick.y);

    u16 packedState = (face << 12) | (trick << 8) | (x << 4) | y;

    m_bufferedState[m_statesBuffered++] = parse(packedState);

    if (m_statesBuffered == m_bufferedState.size()) {
        flushBufferToFile();
    }
}

void SavestateManager::saveState(size_t slot) {
    ASSERT(slot < SLOTS);
    flushBufferToFile();

    // Seek to end immediately so we can get framecount
    std::ifstream inFile(STATE_FILE, std::ios::ate);
    if (!inFile) {
        PANIC("Could not open %s!", STATE_FILE);
    }

    std::ofstream outFile(m_files[slot], std::ios::binary);
    if (!outFile) {
        PANIC("Could not open %s!", m_files[slot]);
    }

    // Write signature, params, framecount, and inputs to savestate file
    u32 sig = parse(KSAV_SIGNATURE);
    u32 params = parse(GetParamsFromRaceConfig());
    u32 framecount = parse(static_cast<u32>(inFile.tellg() / sizeof(u16)));

    outFile.write(reinterpret_cast<const char *>(&sig), sizeof(sig));
    outFile.write(reinterpret_cast<const char *>(&params), sizeof(params));
    outFile.write(reinterpret_cast<const char *>(&framecount), sizeof(framecount));

    inFile.clear();
    inFile.seekg(0, std::ios::beg);
    outFile << inFile.rdbuf();
}

void SavestateManager::loadState(size_t slot) {
    std::ifstream inFile(m_files[slot], std::ios::binary);
    if (!inFile) {
        PANIC("Failed to load state %s!", m_files[slot]);
    }

    u32 sig;
    inFile.read(reinterpret_cast<char *>(&sig), sizeof(sig));
    ASSERT(parse(sig) == KSAV_SIGNATURE);

    ASSERT(m_controller);
    ASSERT(m_sceneMgr);

    m_sceneMgr->destroyScene(m_sceneMgr->currentScene());

    u32 packedScenario;
    if (!inFile.read(reinterpret_cast<char *>(&packedScenario), sizeof(packedScenario)).good()) {
        PANIC("Failed to read scenario data! Your savestate is corrupted.");
    }

    setRaceConfigFromParams(parse(packedScenario));

    m_sceneMgr->createScene(2, m_sceneMgr->currentScene());

    u32 framecount;
    if (!inFile.read(reinterpret_cast<char *>(&framecount), sizeof(framecount)).good()) {
        PANIC("Failed to read framecount! Your savestate is corrupted.");
    }

    framecount = parse(framecount);

    // Fast-forward to load the state
    for (u32 i = 0; i < framecount; ++i) {
        u16 frameData;
        if (!inFile.read(reinterpret_cast<char *>(&frameData), sizeof(frameData)).good()) {
            PANIC("Error on frame %u: Failed to read framedata! Your savestate is corrupted.", i);
        }

        frameData = parse(frameData);

        u16 face = frameData >> 12;
        System::Trick trick = static_cast<System::Trick>((frameData >> 8) & 0xf);
        u8 x = (frameData >> 4) & 0xf;
        u8 y = frameData & 0xf;

        m_controller->setInputsRawStick(face, x, y, trick);

        m_sceneMgr->calc();
    }

    const auto &pos = Kart::KartObjectManager::Instance()->object(0)->pos();
    REPORT("X: %f | Y: %f | Z: %f", static_cast<double>(pos.x), static_cast<double>(pos.y),
            static_cast<double>(pos.z));
}

void SavestateManager::saveContext(size_t slot) {
    // TODO: This is copied from main.cc. Should we store this in SceneManager?
    constexpr size_t MEMORY_SPACE_SIZE = 0x1000000;

    ASSERT(slot < SLOTS);
    getContext(slot);

    ASSERT(m_contextBlocks[slot]);
    memcpy(m_contextBlocks[slot], EGG::SceneManager::s_rootHeap, MEMORY_SPACE_SIZE);
}

void SavestateManager::loadContext(size_t slot) {
    // TODO: This is copied from main.cc. Should we store this in SceneManager?
    constexpr size_t MEMORY_SPACE_SIZE = 0x1000000;

    ASSERT(m_contextBlocks[slot]);
    memcpy(reinterpret_cast<void *>(EGG::SceneManager::s_rootHeap), m_contextBlocks[slot],
            MEMORY_SPACE_SIZE);

    setContext(slot);
}

void SavestateManager::flushBufferToFile() {
    if (m_statesBuffered == 0) {
        return;
    }

    std::ofstream outFile(STATE_FILE, std::ios::app | std::ios::binary);
    if (!outFile) {
        PANIC("Could not open %s!", STATE_FILE);
    }

    outFile.write(reinterpret_cast<const char *>(m_bufferedState.data()),
            m_statesBuffered * sizeof(u16));

    m_bufferedState = {};
    m_statesBuffered = 0;
}

void SavestateManager::initMemory() {
    constexpr size_t MEMORY_SPACE_SIZE = 0x1000000;

    m_coreBlock = EGG::SceneManager::s_rootHeap;

    Abstract::Memory::MEMiHeapHead::OptFlag opt;
    opt.setBit(Abstract::Memory::MEMiHeapHead::eOptFlag::ZeroFillAlloc);

#ifdef BUILD_DEBUG
    opt.setBit(Abstract::Memory::MEMiHeapHead::eOptFlag::DebugFillAlloc);
#endif

    m_auxBlock = malloc(MEMORY_SPACE_SIZE);

    for (auto *&block : m_contextBlocks) {
        block = malloc(MEMORY_SPACE_SIZE);
    }
}

std::optional<SavestateManager::Context> &SavestateManager::getContext(size_t slot) {
    ASSERT(slot < SLOTS);

    auto &context = m_contexts[slot];

    context = Context();
    context->m_rootList = Abstract::Memory::MEMiHeapHead::s_rootList;
    context->m_archiveList = EGG::Archive::s_archiveList;
    context->m_heapList = EGG::Heap::s_heapList;
    context->m_currentHeap = EGG::Heap::s_currentHeap;
    context->m_heapForCreateScene = EGG::SceneManager::s_heapForCreateScene;
    context->m_boxColMgr = Field::BoxColManager::s_instance;
    context->m_colDir = Field::CollisionDirector::s_instance;
    context->m_courseColMgr = Field::CourseColMgr::s_instance;
    context->m_objDir = Field::ObjectDirector::s_instance;
    context->m_objDrivableDir = Field::ObjectDrivableDirector::s_instance;
    context->m_railMgr = Field::RailManager::s_instance;
    context->m_itemDir = Item::ItemDirector::s_instance;
    context->m_kartObjMgr = Kart::KartObjectManager::s_instance;
    context->m_paramFileMgr = Kart::KartParamFileManager::s_instance;
    context->m_courseMap = System::CourseMap::s_instance;
    context->m_padDir = System::KPadDirector::s_instance;
    context->m_raceConfig = System::RaceConfig::s_instance;
    context->m_onInitCallback = System::RaceConfig::s_onInitCallback;
    context->m_onInitCallbackArg = System::RaceConfig::s_onInitCallbackArg;
    context->m_raceMgr = System::RaceManager::s_instance;
    context->m_resMgr = System::ResourceManager::s_instance;

    return context;
}

void SavestateManager::setContext(size_t slot) {
    ASSERT(slot < SLOTS);

    auto &context = m_contexts[slot];
    ASSERT(context);

    Abstract::Memory::MEMiHeapHead::s_rootList = *context->m_rootList;
    EGG::Archive::s_archiveList = *context->m_archiveList;
    EGG::Heap::s_heapList = *context->m_heapList;
    EGG::Heap::s_currentHeap = context->m_currentHeap;
    EGG::SceneManager::s_heapForCreateScene = context->m_heapForCreateScene;
    Field::BoxColManager::s_instance = context->m_boxColMgr;
    Field::CollisionDirector::s_instance = context->m_colDir;
    Field::CourseColMgr::s_instance = context->m_courseColMgr;
    Field::ObjectDirector::s_instance = context->m_objDir;
    Field::ObjectDrivableDirector::s_instance = context->m_objDrivableDir;
    Field::RailManager::s_instance = context->m_railMgr;
    Item::ItemDirector::s_instance = context->m_itemDir;
    Kart::KartObjectManager::s_instance = context->m_kartObjMgr;
    Kart::KartParamFileManager::s_instance = context->m_paramFileMgr;
    System::CourseMap::s_instance = context->m_courseMap;
    System::KPadDirector::s_instance = context->m_padDir;
    System::RaceConfig::s_instance = context->m_raceConfig;
    System::RaceConfig::s_onInitCallback = context->m_onInitCallback;
    System::RaceConfig::s_onInitCallbackArg = context->m_onInitCallbackArg;
    System::RaceManager::s_instance = context->m_raceMgr;
    System::ResourceManager::s_instance = context->m_resMgr;
}

void SavestateManager::setRaceConfigFromParams(u32 params) {
    auto &scenario = System::RaceConfig::Instance()->raceScenario();
    auto &player = scenario.players[0];
    scenario.course = static_cast<Course>(params >> 24);
    player.character = static_cast<Character>((params >> 16) & 0x1f);
    player.vehicle = static_cast<Vehicle>((params >> 8) & 0x3f);
    player.driftIsAuto = static_cast<bool>(params & 0x1);
    m_controller->setDriftIsAuto(static_cast<bool>(params & 0x1));
}

std::filesystem::path SavestateManager::GetFilePathForSlot(size_t slot) {
    char filename[16];
    snprintf(filename, sizeof(filename), "kinoko.s%02zu", slot);

    auto path = std::filesystem::current_path();
    path.append(filename);

    return path;
}

u32 SavestateManager::GetParamsFromRaceConfig() {
    const auto &scenario = System::RaceConfig::Instance()->raceScenario();
    const auto &player = scenario.players[0];

    u32 course = static_cast<u32>(scenario.course);
    u32 character = static_cast<u32>(player.character);
    u32 vehicle = static_cast<u32>(player.vehicle);
    u32 driftIsAuto = static_cast<u32>(player.driftIsAuto);

    return (course << 24) | (character << 16) | (vehicle << 8) | driftIsAuto;
}
