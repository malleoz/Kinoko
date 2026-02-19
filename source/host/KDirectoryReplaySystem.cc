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

// Entrypoint for each thread
void KDirectoryReplaySystem::startThread() {
    // Each thread must have its own root heap
    static thread_local std::unique_ptr<void, void (*)(void *)> s_memorySpace(
            malloc(MEMORY_SPACE_SIZE), free);
    static thread_local EGG::ExpHeap *s_rootHeap =
            EGG::ExpHeap::create(s_memorySpace.get(), MEMORY_SPACE_SIZE, DEFAULT_OPT);
    char name[32];
    snprintf(name, sizeof(name), "ReplayRootThread%zu",
            std::hash<std::thread::id>{}(std::this_thread::get_id()));
    s_rootHeap->setName(name);
    s_rootHeap->becomeCurrentHeap();

    EGG::SceneManager::SetRootHeap(s_rootHeap);

    static thread_local auto sceneCreator = std::make_unique<Host::SceneCreatorDynamic>();
    m_sceneMgr = std::make_unique<EGG::SceneManager>(sceneCreator.get());

    m_startLatch->wait();

    REPORT("Thread started!");

    auto *prevHeap = EGG::Heap::getCurrentHeap();
    m_ghostHeap = EGG::ExpHeap::create(0x100000, EGG::SceneManager::rootHeap(), DEFAULT_OPT);
    m_ghostHeap->setName("GhostHeap");
    m_ghostHeap->becomeCurrentHeap();

    while (true) {
        auto nextGhost = getNextGhost();

        prevHeap->becomeCurrentHeap();

        if (!nextGhost) {
            break;
        }

        m_currentGhostFile = std::move(*nextGhost);

        runGhost();

        if (++m_replayCount % 100 == 0) {
            REPORT("Ghost #%zu", m_replayCount.load());
        }

        m_ghostHeap->becomeCurrentHeap();
    }

    REPORT("No more ghosts! Thread exiting");
}

/// @brief Provides thread-safe access to the std::generator
std::optional<Abstract::DVDFile> KDirectoryReplaySystem::getNextGhost() {
    std::lock_guard<std::mutex> lock(m_generatorMutex);

    if (!m_fileGenerator || !m_generatorIt) {
        return std::nullopt;
    }

    auto &it = *m_generatorIt;
    if (it == m_fileGenerator->end()) {
        return std::nullopt;
    }

    auto file = *it;
    ++it;

    return file;
}

void KDirectoryReplaySystem::init() {
    // The init function probably should be registered thread_local as well, however static
    // thread_local storage means that the std::function will be destroyed after the ExpHeap is
    // destroyed, causing a segfault.
    System::RaceConfig::RegisterInitCallback(OnInit, nullptr);

    // Create and initialize m_threadCount threads
    m_startLatch = std::make_unique<std::latch>(1);
    m_threads = std::span<std::thread>(new std::thread[m_threadCount], m_threadCount);
    for (auto &thread : m_threads) {
        thread = std::thread(startThread, this);
    }
}

bool KDirectoryReplaySystem::run() {
    // Tell each thread to start
    m_startLatch->count_down();

    for (auto &thread : m_threads) {
        thread.join();
    }

    return true;
}

void KDirectoryReplaySystem::calc() {
    m_sceneMgr->calc();
}

/// @brief Parses non-generic command line options.
/// @details Expects a directory name
/// @param argc The number of arguments.
/// @param argv The arguments.
void KDirectoryReplaySystem::parseOptions(int argc, char **argv) {
    if (argc-- < 1) {
        PANIC("Expected directory argument");
    }

    m_fileGenerator = Abstract::Filesystem::iterate(*argv++, rkgFilter);
    m_generatorIt = m_fileGenerator->begin();

    if (argc++ > 0) {
        // Check for thread count
        std::optional<Host::EOption> flag = Host::Option::CheckFlag(*argv++);
        if (flag == Host::EOption::Threads) {
            // Expect thread count following
            if (argc == 0) {
                PANIC("Expected count after thread argument");
            }

            int count = atoi(*argv);

            // Check for reasonable thread count
            if (count < 1 || count > 64) {
                PANIC("Invalid thread count of %d", count);
            }

            m_threadCount = static_cast<u16>(count);
        }
    }
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

KDirectoryReplaySystem::KDirectoryReplaySystem() : m_replayCount(0), m_threadCount(1) {}

KDirectoryReplaySystem::~KDirectoryReplaySystem() {
    if (s_instance) {
        s_instance = nullptr;
        WARN("KDirectoryReplaySystem instance not explicitly handled!");
    }

    delete[] m_threads.data();
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

thread_local std::unique_ptr<EGG::SceneManager> KDirectoryReplaySystem::m_sceneMgr;
thread_local Abstract::DVDFile KDirectoryReplaySystem::m_currentGhostFile;
thread_local std::optional<System::GhostFile> KDirectoryReplaySystem::m_currentGhost;
thread_local EGG::ExpHeap *KDirectoryReplaySystem::m_ghostHeap = nullptr;
