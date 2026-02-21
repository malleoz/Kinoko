#include "KDirectoryReplaySystem.hh"

#include "host/Option.hh"
#include "host/SceneCreatorDynamic.hh"

#include <abstract/File.hh>
#include <abstract/Filesystem.hh>

#include <egg/core/ExpHeap.hh>

#include <game/kart/KartObjectProxy.hh>
#include <game/system/RaceManager.hh>

#include <chrono>

static auto rkgFilter = [](const std::filesystem::path &path) -> bool {
    return path.has_extension() && memcmp(path.extension().string().c_str(), ".rkg", 4) == 0;
};

RKGFileGenerator::RKGFileGenerator(const char *path) : m_doneProducing(false) {
    m_fileGenerator = Abstract::Filesystem::iterate(path, rkgFilter);
}

void RKGFileGenerator::produce() {
    // Producer thread needs its own root heap
    void *memorySpace = malloc(MEMORY_SPACE_SIZE);
    EGG::ExpHeap *rootHeap = EGG::ExpHeap::create(memorySpace, MEMORY_SPACE_SIZE, DEFAULT_OPT);
    rootHeap->setName("GeneratorThread");
    rootHeap->becomeCurrentHeap();

    EGG::SceneManager::SetRootHeap(rootHeap);

    // Start producing files, pausing when the queue is full
    for (auto file : *m_fileGenerator) {
        std::unique_lock<std::mutex> lock(m_queueMutex);
        m_cvProducer.wait(lock, [this]() { return m_queuedFiles.size() < MAX_QUEUE_SIZE; });

        m_queuedFiles.push(std::move(file));
        m_cvConsumer.notify_one();

        // Reclaim any returned files
        while (!m_reclaimQueue.empty()) {
            m_reclaimQueue.pop(); // Destroys the file
        }
    }

    {
        std::unique_lock<std::mutex> lock(m_queueMutex);
        m_doneProducing = true;
        REPORT("Done producing!");
    }

    // Even if we're done producing, if the queue is not empty, we need to wait for all consumers to
    // take the files away
    {
        std::unique_lock<std::mutex> lock(m_queueMutex);
        m_cvProducer.wait(lock,
                [this]() { return m_queuedFiles.empty() && m_reclaimQueue.empty(); });
        m_reclaimQueue.pop();
    }

    REPORT("Queue is now empty");

    m_cvConsumer.notify_all();

    // Intentionally leak the rootHeap so that statics in other TUs are torn down before the heap
    // is!
}

/// @brief Accessed by consumer threads to get the next queued file. Also signals to the producer
/// thread to fill the queue back up
std::optional<Abstract::DVDFile> RKGFileGenerator::consume() {
    std::unique_lock<std::mutex> lock(m_queueMutex);
    m_cvConsumer.wait(lock, [this]() { return (!m_queuedFiles.empty()) || m_doneProducing; });

    if (m_queuedFiles.empty()) {
        m_cvProducer.notify_all();
        return std::nullopt;
    }

    // Copy constructor, so that the file's data is allocated on the consuming thread's heap
    Abstract::DVDFile file = m_queuedFiles.front();
    m_reclaimQueue.push(std::move(m_queuedFiles.front()));
    m_queuedFiles.pop();

    // Notify producer
    m_cvProducer.notify_one();

    return file;
}

// Entrypoint for each thread
void KDirectoryReplaySystem::startThread() {
    // Each thread must have its own root heap
    void *memorySpace = malloc(MEMORY_SPACE_SIZE);
    EGG::ExpHeap *rootHeap = EGG::ExpHeap::create(memorySpace, MEMORY_SPACE_SIZE, DEFAULT_OPT);
    char name[32];
    snprintf(name, sizeof(name), "ReplayRootThread%zu",
            std::hash<std::thread::id>{}(std::this_thread::get_id()));
    rootHeap->setName(name);
    rootHeap->becomeCurrentHeap();

    EGG::SceneManager::SetRootHeap(rootHeap);

    auto sceneCreator = new Host::SceneCreatorDynamic;
    m_sceneMgr = new EGG::SceneManager(sceneCreator);

    m_startLatch->wait();

    REPORT("Thread started!");

    while (true) {
        auto file = m_generator->consume();
        if (!file.has_value()) {
            break;
        }

        m_currentGhostFile = std::move(*file);

        runGhost();

        if (++m_replayCount % 100 == 0) {
            REPORT("Ghost #%zu", m_replayCount.load());
        }
    }

    REPORT("No more ghosts! Thread exiting");

    // Intentionally leak the rootHeap so that statics in other TUs are torn down before the heap
    // is! also We have to explicitly delete the nodes in the KartObjectManager::s_proxyList to make
    // sure that the heap-allocated nodes in the linked list are destroyed on the right thread.
    Kart::KartObjectProxy::clearProxyList();

    // TODO, on thread exit, save the objects that need to be deleted so we can delete once we leave
    // thread scope?
}

void KDirectoryReplaySystem::init() {
    // Before we do anything, kick off a thread that will start iterating files
    m_producerThread = std::thread(RKGFileGenerator::produce, &m_generator.value());

    // The init function probably should be registered thread_local as well, however static
    // thread_local storage means that the std::function will be destroyed after the ExpHeap is
    // destroyed, causing a segfault.
    System::RaceConfig::RegisterInitCallback(OnInit, nullptr);

    // Create and initialize m_threadCount threads
    m_startLatch = std::make_unique<std::latch>(1);
    m_threads = std::span<std::thread>(new std::thread[m_threadCount], m_threadCount);

    for (auto &thread : m_threads) {
        thread = std::thread(&KDirectoryReplaySystem::startThread, this);
    }
}

bool KDirectoryReplaySystem::run() {
    using std::chrono::duration;
    using std::chrono::duration_cast;
    using std::chrono::high_resolution_clock;
    using std::chrono::milliseconds;

    auto t1 = high_resolution_clock::now();

    // Tell each thread to start
    m_startLatch->count_down();

    m_producerThread.join();

    for (auto &thread : m_threads) {
        thread.join();
    }

    auto t2 = high_resolution_clock::now();

    /* Getting number of milliseconds as an integer. */
    auto ms_int = duration_cast<milliseconds>(t2 - t1);

    REPORT("Analyzed %zu ghosts in %ld seconds", m_replayCount.load(), ms_int.count() / 1000);

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

    m_generator.emplace(*argv++);

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
        WARN("DESYNC! %s", m_currentGhostFile.path().string().c_str());
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
    std::string report(m_currentGhostFile.path().string().c_str());
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

thread_local EGG::SceneManager *KDirectoryReplaySystem::m_sceneMgr;
thread_local Abstract::DVDFile KDirectoryReplaySystem::m_currentGhostFile;
thread_local std::optional<System::GhostFile> KDirectoryReplaySystem::m_currentGhost;
thread_local EGG::ExpHeap *KDirectoryReplaySystem::m_ghostHeap = nullptr;
