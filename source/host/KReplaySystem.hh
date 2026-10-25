#pragma once

#include "host/KSystem.hh"

#include <egg/core/Heap.hh>

#include <filesystem>
#include <queue>

namespace Kinoko {

namespace EGG {

class SceneManager;

} // namespace EGG

namespace System {

class GhostFile;
class RaceConfig;
struct Timer;

} // namespace System

/// @brief Kinoko system designed to execute replays
/// @details This mode accepts a sequence of ghost `.rkg` files or directories containing ghost
/// `.rkg` files, in which case this system will recurse into the directories to find all ghost
/// files.
///
/// In worker mode, ghost paths are instead read line-by-line from stdin, and each ghost is replayed
/// in the same process. For every ghost, exactly one line is written to stdout of the form:
/// `KINOKO_RESULT\t<STATUS>\t<path>\t<message>`, where STATUS is one of SYNC, DESYNC, NOFINISH,
/// or ERROR. This allows an external process to hand off ghosts to a pool of workers.
class KReplaySystem : public KSystem {
public:
    void init() override;
    void calc() override;
    bool run() override;
    void parseOptions(int argc, char **argv) override;

    static KReplaySystem *CreateInstance();
    static void DestroyInstance();
    [[nodiscard]] static KReplaySystem *Instance();

private:
    /// @brief Represents a pair of timers that are desynchronizing during a replay
    typedef std::pair<const System::Timer &, const System::Timer &> DesyncingTimerPair;

    /// @brief The outcome of a completed replay
    enum class Result {
        Sync,     ///< The race finished with the same timers as the ghost
        Desync,   ///< The race finished, but a timer differs from the ghost
        NoFinish, ///< The race did not finish before the time limit
    };

    EGG_NEW_DELETE_FRIEND

    KReplaySystem();
    ~KReplaySystem() override;

    /// @brief Deleted copy constructor
    KReplaySystem(const KReplaySystem &) = delete;

    /// @brief Deleted move constructor
    KReplaySystem(KReplaySystem &&) = delete;

    /// @brief Deleted copy assignment operator
    KReplaySystem &operator=(const KReplaySystem &) = delete;

    /// @brief Deleted move assignment operator
    KReplaySystem &operator=(KReplaySystem &&) = delete;

    void reportFail(const std::string &msg) const;
    bool runDirectory(const std::filesystem::path &dirPath);
    bool runGhost(const std::filesystem::path &ghostPath);
    bool runWorker();
    void reportWorkerResult(const char *status, const std::string &path,
            const std::string &msg) const;
    void loadGhost(const std::filesystem::path &ghostPath);
    [[nodiscard]] bool tryLoadGhost(const std::filesystem::path &ghostPath, const char *&err);
    void unloadGhost();
    [[nodiscard]] Result replayGhost(std::string &failMsg);

    [[nodiscard]] Result checkResult(std::string &failMsg) const;
    [[nodiscard]] bool calcEnd() const;
    [[nodiscard]] s32 getDesyncingTimerIdx() const;
    [[nodiscard]] DesyncingTimerPair getDesyncingTimer(s32 i) const;

    static void OnInit(System::RaceConfig *config, void *arg);

    EGG::SceneManager *m_sceneMgr;                 ///< Pointer to the scene manager instance
    bool m_workerMode;                             ///< Whether ghost paths are read from stdin
    std::queue<std::filesystem::path> m_ghostArgs; ///< Queue of ghost files/folders to be replayed
    size_t m_progressInterval;                     ///< Interval at which progress is reported
    size_t m_replaysPlayed;                        ///< Number of replays that have been played
    size_t m_replaysSynced; ///< Number of replays that have successfully synchronized
    std::filesystem::path m_currentGhostPath; ///< Path to the currently loaded ghost file
    const char *m_currentGhostFileName;       ///< Name of the currently loaded ghost file
    const System::GhostFile *m_currentGhost;  ///< Pointer to the currently loaded ghost file
    std::span<const u8> m_currentRawGhost;    ///< The raw ghost data
};

} // namespace Kinoko
