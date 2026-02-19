#include "KDirectoryReplaySystem.hh"

#include "host/Option.hh"
#include "host/SceneCreatorDynamic.hh"

#include <abstract/File.hh>
#include <abstract/Filesystem.hh>

#include <egg/core/ExpHeap.hh>

#include <game/system/RaceManager.hh>

#include <chrono>

static auto rkgFilter = [](const std::filesystem::path &path) -> bool {
    return path.has_extension() && memcmp(path.extension().string().c_str(), ".rkg", 4) == 0;
};

void KDirectoryReplaySystem::init() {
    auto *sceneCreator = new Host::SceneCreatorDynamic;
    m_sceneMgr = new EGG::SceneManager(sceneCreator);

    System::RaceConfig::RegisterInitCallback(OnInit, nullptr);
    Abstract::File::Remove("results.txt");

    m_memorySpace = malloc(MEMORY_SPACE_SIZE);
    m_ghostHeap = EGG::ExpHeap::create(m_memorySpace, MEMORY_SPACE_SIZE, DEFAULT_OPT);
    m_ghostHeap->setName("GhostHeap");
}

void KDirectoryReplaySystem::calc() {
    m_sceneMgr->calc();
}

bool KDirectoryReplaySystem::run() {
    using std::chrono::duration;
    using std::chrono::duration_cast;
    using std::chrono::high_resolution_clock;
    using std::chrono::milliseconds;

    auto t1 = high_resolution_clock::now();

    // Switch to the ghost heap so that the generator consumes memory in the ghost heap
    auto *prevHeap = EGG::Heap::getCurrentHeap();
    m_ghostHeap->becomeCurrentHeap();

    for (auto file : *m_fileGenerator) {
        prevHeap->becomeCurrentHeap();
        m_ghostHeap->destroy();
        m_ghostHeap = EGG::ExpHeap::create(m_memorySpace, MEMORY_SPACE_SIZE, DEFAULT_OPT);

        m_currentGhostFile = std::move(file);

        runGhost();

        if (++m_replayCount % 100 == 0) {
            REPORT("Ghost #%zu", m_replayCount.load());
        }

        m_ghostHeap->becomeCurrentHeap();
    }

    auto t2 = high_resolution_clock::now();

    /* Getting number of milliseconds as an integer. */
    auto ms_int = duration_cast<milliseconds>(t2 - t1);

    REPORT("Analyzed %zu ghosts in %ld milliseconds", m_replayCount.load(), ms_int.count());

    return true;
}

/// @brief Parses non-generic command line options.
/// @details Expects a directory name
/// @param argc The number of arguments.
/// @param argv The arguments.
void KDirectoryReplaySystem::parseOptions(int argc, char **argv) {
    if (argc < 1) {
        PANIC("Expected directory argument");
    }

    m_fileGenerator = Abstract::Filesystem::iterate(*argv, rkgFilter);
}

KDirectoryReplaySystem *KDirectoryReplaySystem::CreateInstance() {
    ASSERT(!s_instance);
    s_instance = new KDirectoryReplaySystem;
    return static_cast<KDirectoryReplaySystem *>(s_instance);
}

void KDirectoryReplaySystem::DestroyInstance() {
    ASSERT(s_instance);
    auto *instance = s_instance;
    s_instance = nullptr;
    delete instance;
}

KDirectoryReplaySystem::KDirectoryReplaySystem() : m_replayCount(0) {}

KDirectoryReplaySystem::~KDirectoryReplaySystem() {
    if (s_instance) {
        s_instance = nullptr;
        WARN("KDirectoryReplaySystem instance not explicitly handled!");
    }
}

void KDirectoryReplaySystem::runGhost() {
    if (m_currentGhostFile.size() < System::RKG_HEADER_SIZE ||
            m_currentGhostFile.size() > sizeof(System::RawGhostFile)) {
        PANIC("File cannot be a ghost! Check the file size.");
    }

    // Creating the raw ghost file validates it
    System::RawGhostFile file =
            System::RawGhostFile(static_cast<const u8 *>(m_currentGhostFile.data()));

    m_currentGhost = System::GhostFile(file);

    // Has the root scene been created?
    if (!m_sceneMgr->currentScene()) {
        m_sceneMgr->changeScene(0);
    } else {
        m_sceneMgr->createScene(2, m_sceneMgr->currentScene());
    }

    auto *scene = m_sceneMgr->currentScene();
    scene->heap()->disableAllocation();

    while (!calcEnd()) {
        calc();
    }

    if (!success()) {
        WARN("DESYNC! TODO: Not sure how to communicate filepath");
    }

    scene->heap()->enableAllocation();
    m_sceneMgr->destroyScene(m_sceneMgr->currentScene());
}

bool KDirectoryReplaySystem::calcEnd() const {
    constexpr u16 MAX_MINUTE_COUNT = 10;

    const auto *raceManager = System::RaceManager::Instance();
    if (raceManager->stage() == System::RaceManager::Stage::FinishGlobal) {
        return true;
    }

    if (raceManager->timerManager().currentTimer().min >= MAX_MINUTE_COUNT) {
        return true;
    }

    return false;
}

/// @brief Reports failure to file.
/// @param msg The message to report.
void KDirectoryReplaySystem::reportFail(const std::string &msg) const {
    std::string report("TODO_FILENAME");
    report += "\n" + std::string(msg);
    Abstract::File::Append("results.txt", report.c_str(), report.size());
}

/// @brief Determines whether the simulation was a success or not.
/// @return Whether the simulation was a success or not.
bool KDirectoryReplaySystem::success() const {
    auto format = [](const System::Timer &timer) {
        std::ostringstream oss;
        oss << std::setw(2) << std::setfill('0') << timer.min << ":" << std::setw(2)
            << std::setfill('0') << timer.sec << "." << std::setw(3) << std::setfill('0')
            << timer.mil;
        return oss.str();
    };

    const auto *raceManager = System::RaceManager::Instance();
    if (raceManager->stage() != System::RaceManager::Stage::FinishGlobal) {
        m_sceneMgr->currentScene()->heap()->enableAllocation();
        reportFail("Race didn't finish");
        return false;
    }

    s32 desyncingTimerIdx = getDesyncingTimerIdx();
    if (desyncingTimerIdx != -1) {
        m_sceneMgr->currentScene()->heap()->enableAllocation();
        std::string msg;

        const auto [correct, incorrect] = getDesyncingTimer(desyncingTimerIdx);
        if (desyncingTimerIdx == 0) {
            msg = "Final timer desync!";
        } else {
            msg = "Lap " + std::to_string(desyncingTimerIdx) + " timer desync!";
        }

        msg += " Expected " + format(correct) + ", got " + format(incorrect);
        reportFail(msg);
        return false;
    }

    return true;
}

/// @brief Finds the desyncing timer index, if one exists.
/// @return -1 if there's no desync, 0 if the final timer desyncs, and 1+ if a lap timer desyncs.
s32 KDirectoryReplaySystem::getDesyncingTimerIdx() const {
    const auto &player = System::RaceManager::Instance()->player();
    if (m_currentGhost->raceTimer() != player.raceTimer()) {
        return 0;
    }

    for (size_t i = 0; i < 3; ++i) {
        if (m_currentGhost->lapTimer(i) != player.getLapSplit(i + 1)) {
            return i + 1;
        }
    }

    return -1;
}

/// @brief Gets the desyncing timer according to the index.
/// @param i Index to the desyncing timer. Cannot be -1.
/// @return The pair of timers. The first is the correct one, and the second is the incorrect one.
KDirectoryReplaySystem::DesyncingTimerPair KDirectoryReplaySystem::getDesyncingTimer(s32 i) const {
    auto cond = i <=> 0;
    ASSERT(cond != std::strong_ordering::less);

    if (cond == std::strong_ordering::equal) {
        const auto &correct = m_currentGhost->raceTimer();
        const auto &incorrect = System::RaceManager::Instance()->player().raceTimer();
        ASSERT(correct != incorrect);
        return DesyncingTimerPair(correct, incorrect);
    } else if (cond == std::strong_ordering::greater) {
        const auto &correct = m_currentGhost->lapTimer(i - 1);
        const auto &incorrect = System::RaceManager::Instance()->player().lapTimer(i - 1);
        ASSERT(correct != incorrect);
        return DesyncingTimerPair(correct, incorrect);
    }

    // This is unreachable
    return DesyncingTimerPair(System::Timer(), System::Timer());
}

/// @brief Initializes the race configuration as needed for replays.
/// @param config The race configuration instance.
/// @param arg Unused optional argument.
void KDirectoryReplaySystem::OnInit(System::RaceConfig *config, void * /* arg */) {
    config->setGhost(static_cast<const u8 *>(Instance()->m_currentGhostFile.data()));
    config->raceScenario().players[0].type = System::RaceConfig::Player::Type::Ghost;
}
