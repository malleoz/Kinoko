#include "KReplaySystem.hh"

#include "host/Option.hh"
#include "host/SceneCreatorDynamic.hh"
#include "host/SceneId.hh"

#include <abstract/File.hh>
#include <egg/core/Heap.hh>

#include <game/system/RaceManager.hh>

#include <cstdio>
#include <cstring>

namespace Kinoko {

/// @brief Initializes the system.
void KReplaySystem::init() {
    auto *sceneCreator = EGG::egg_new<Host::SceneCreatorDynamic>();
    m_sceneMgr = EGG::egg_new<EGG::SceneManager>(sceneCreator);

    System::RaceConfig::RegisterInitCallback(OnInit, nullptr);

    // In worker mode, the root scene is created once the first ghost has been received
    if (m_workerMode) {
        return;
    }

    ASSERT(m_currentGhostFileName);
    ASSERT(m_currentGhost);

    Abstract::File::Remove("results.txt");

    m_sceneMgr->changeScene(0);
}

/// @brief Executes a frame.
void KReplaySystem::calc() {
    m_sceneMgr->calc();
}

/// @brief Executes a run.
/// @details A run consists of replaying a ghost, or every ghost received over stdin in worker mode.
/// @return Whether the run was successful or not.
bool KReplaySystem::run() {
    if (m_workerMode) {
        return runWorker();
    }

    while (!calcEnd()) {
        calc();
    }

    if (checkResult() == Result::Sync) {
        return true;
    }

    m_sceneMgr->currentScene()->heap()->enableAllocation();
    reportFail(m_failMsg.data());
    return false;
}

/// @brief Parses non-generic command line options.
/// @details Accepts either the ghost flag (followed by a ghost path) or the worker flag.
/// @param argc The number of arguments.
/// @param argv The arguments.
void KReplaySystem::parseOptions(int argc, char **argv) {
    if (argc < 1) {
        PANIC("Expected ghost or worker argument!");
    }

    for (int i = 0; i < argc; ++i) {
        std::optional<Host::EOption> flag = Host::Option::CheckFlag(argv[i]);
        if (!flag || *flag == Host::EOption::Invalid) {
            WARN("Expected a flag! Got: %s", argv[i]);
            continue;
        }

        switch (*flag) {
        case Host::EOption::Ghost: {
            ASSERT(i + 1 < argc);

            m_currentGhostFileName = argv[++i];

            const char *err = nullptr;
            if (!loadGhost(m_currentGhostFileName, err)) {
                PANIC("%s: %s", err, m_currentGhostFileName);
            }
        } break;
        case Host::EOption::Worker: {
            m_workerMode = true;
        } break;
        case Host::EOption::Invalid:
        default:
            PANIC("Invalid flag!");
            break;
        }
    }

    if (m_workerMode && m_currentGhostFileName) {
        PANIC("The ghost and worker flags are mutually exclusive!");
    }

    if (!m_workerMode && !m_currentGhostFileName) {
        PANIC("Expected ghost argument!");
    }

    if (m_workerMode) {
        // Results must reach the parent process immediately, and any PANIC output must not be lost
        // in a buffer when the process aborts.
        setvbuf(stdout, nullptr, _IONBF, 0);
    }
}

KReplaySystem *KReplaySystem::CreateInstance() {
    ASSERT(!s_instance);
    s_instance = EGG::egg_new<KReplaySystem>();
    return static_cast<KReplaySystem *>(s_instance);
}

void KReplaySystem::DestroyInstance() {
    ASSERT(s_instance);
    auto *instance = s_instance;
    s_instance = nullptr;
    EGG::egg_delete(instance);
}

KReplaySystem::KReplaySystem()
    : m_sceneMgr(nullptr), m_workerMode(false), m_currentGhostFileName(nullptr), m_rawGhost{},
      m_workerGhostPath{}, m_failMsg{} {}

KReplaySystem::~KReplaySystem() {
    if (s_instance) {
        s_instance = nullptr;
        WARN("KReplaySystem instance not explicitly handled!");
    }

    EGG::egg_delete(m_sceneMgr);
}

/// @brief Replays every ghost path received over stdin until EOF.
/// @details Each ghost replay creates a fresh race scene under the persistent root scene, which is
/// destroyed once the replay finishes. One result line is reported per ghost.
/// @return Whether every ghost synchronized.
bool KReplaySystem::runWorker() {
    bool allSynced = true;

    while (readWorkerGhostPath()) {
        const char *path = m_workerGhostPath.data();
        if (path[0] == '\0') {
            continue;
        }

        m_currentGhostFileName = path;

        const char *err = nullptr;
        if (!loadGhost(path, err)) {
            reportWorkerResult("ERROR", err);
            allSynced = false;
            continue;
        }

        if (!m_sceneMgr->currentScene()) {
            // Creates the root scene, which then creates the race scene
            m_sceneMgr->changeScene(0);
        } else {
            m_sceneMgr->createScene(static_cast<int>(Host::SceneId::Race),
                    m_sceneMgr->currentScene());
        }

        while (!calcEnd()) {
            calc();
        }

        switch (checkResult()) {
        case Result::Sync:
            reportWorkerResult("SYNC", "");
            break;
        case Result::Desync:
            reportWorkerResult("DESYNC", m_failMsg.data());
            allSynced = false;
            break;
        case Result::NoFinish:
            reportWorkerResult("NOFINISH", m_failMsg.data());
            allSynced = false;
            break;
        }

        // Tear down the race scene, returning to the root scene for the next ghost
        m_sceneMgr->currentScene()->heap()->enableAllocation();
        m_sceneMgr->destroyScene(m_sceneMgr->currentScene());

        m_currentGhost.reset();
        m_currentGhostFileName = nullptr;
    }

    return allSynced;
}

/// @brief Reads the next newline-terminated ghost path from stdin.
/// @details Paths which do not fit in the buffer are discarded and reported as an error.
/// @return False on EOF, otherwise true.
bool KReplaySystem::readWorkerGhostPath() {
    char *buf = m_workerGhostPath.data();

    while (fgets(buf, static_cast<int>(m_workerGhostPath.size()), stdin)) {
        size_t len = strlen(buf);
        bool complete = len > 0 && buf[len - 1] == '\n';

        if (!complete && !feof(stdin)) {
            // Discard the remainder of the oversized line
            int c;
            do {
                c = fgetc(stdin);
            } while (c != '\n' && c != EOF);

            m_currentGhostFileName = "";
            reportWorkerResult("ERROR", "Ghost path is too long");
            m_currentGhostFileName = nullptr;
            continue;
        }

        buf[strcspn(buf, "\r\n")] = '\0';
        return true;
    }

    return false;
}

/// @brief Writes a single machine-readable result line to stdout.
/// @param status The status of the replay.
/// @param msg Additional details about the result.
void KReplaySystem::reportWorkerResult(const char *status, const char *msg) const {
    printf("KINOKO_RESULT\t%s\t%s\t%s\n", status,
            m_currentGhostFileName ? m_currentGhostFileName : "", msg);
}

/// @brief Loads and validates a ghost into the raw ghost buffer.
/// @details Some malformed ghosts can still trigger a PANIC during deeper validation.
/// @param path The path to the ghost file.
/// @param err Set to a description of the problem if the ghost could not be loaded.
/// @return Whether the ghost was successfully loaded.
bool KReplaySystem::loadGhost(const char *path, const char *&err) {
    m_currentGhost.reset();

    FILE *file = fopen(path, "rb");
    if (!file) {
        err = "Failed to open file";
        return false;
    }

    // Zero-fill so that ghosts smaller than the maximum size are not read out of bounds
    m_rawGhost.fill(0);
    size_t size = fread(m_rawGhost.data(), 1, m_rawGhost.size(), file);
    bool tooLarge = fgetc(file) != EOF;
    fclose(file);

    if (size < System::RKG_HEADER_SIZE || tooLarge) {
        err = "File cannot be a ghost! Check the file size.";
        return false;
    }

    if (strncmp(reinterpret_cast<const char *>(m_rawGhost.data()), "RKGD", 4) != 0) {
        err = "RKG header malformed";
        return false;
    }

    System::RawGhostFile raw;
    if (!raw.isValid(m_rawGhost.data())) {
        err = "Invalid RKG header";
        return false;
    }

    // Initializing the raw ghost file decompresses the inputs, if necessary
    raw = m_rawGhost.data();
    m_currentGhost.emplace(raw);

    return true;
}

/// @brief Determines whether or not the ghost simulation should end.
/// @return Whether the ghost should end or not.
bool KReplaySystem::calcEnd() const {
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
void KReplaySystem::reportFail(const std::string &msg) const {
    std::string report(m_currentGhostFileName);
    report += "\n" + std::string(msg);
    Abstract::File::Append("results.txt", report.c_str(), report.size());
}

/// @brief Determines whether the simulation was a success or not.
/// @details On failure, a description of the failure is written to the failure message buffer.
/// @return The result of the simulation.
KReplaySystem::Result KReplaySystem::checkResult() {
    m_failMsg[0] = '\0';

    const auto *raceManager = System::RaceManager::Instance();
    if (raceManager->stage() != System::RaceManager::Stage::FinishGlobal) {
        snprintf(m_failMsg.data(), m_failMsg.size(), "Race didn't finish");
        return Result::NoFinish;
    }

    s32 desyncingTimerIdx = getDesyncingTimerIdx();
    if (desyncingTimerIdx == -1) {
        return Result::Sync;
    }

    const auto [correct, incorrect] = getDesyncingTimer(desyncingTimerIdx);
    constexpr const char *TIMER_FMT = "Expected %02u:%02u.%03u, got %02u:%02u.%03u";
    char timers[64];
    snprintf(timers, sizeof(timers), TIMER_FMT, static_cast<u32>(correct.min),
            static_cast<u32>(correct.sec), static_cast<u32>(correct.mil),
            static_cast<u32>(incorrect.min), static_cast<u32>(incorrect.sec),
            static_cast<u32>(incorrect.mil));

    if (desyncingTimerIdx == 0) {
        snprintf(m_failMsg.data(), m_failMsg.size(), "Final timer desync! %s", timers);
    } else {
        snprintf(m_failMsg.data(), m_failMsg.size(), "Lap %d timer desync! %s", desyncingTimerIdx,
                timers);
    }

    return Result::Desync;
}

/// @brief Finds the desyncing timer index, if one exists.
/// @return -1 if there's no desync, 0 if the final timer desyncs, and 1+ if a lap timer desyncs.
s32 KReplaySystem::getDesyncingTimerIdx() const {
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
KReplaySystem::DesyncingTimerPair KReplaySystem::getDesyncingTimer(s32 i) const {
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
void KReplaySystem::OnInit(System::RaceConfig *config, void * /* arg */) {
    config->setGhost(Instance()->m_rawGhost.data());
    config->raceScenario().players[0].type = System::RaceConfig::Player::Type::Ghost;
}

} // namespace Kinoko
