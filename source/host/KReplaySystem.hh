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
    void loadGhost(const std::filesystem::path &ghostPath);

    bool success() const;
    [[nodiscard]] bool calcEnd() const;
    [[nodiscard]] s32 getDesyncingTimerIdx() const;
    [[nodiscard]] DesyncingTimerPair getDesyncingTimer(s32 i) const;

    static void OnInit(System::RaceConfig *config, void *arg);

    EGG::SceneManager *m_sceneMgr;                 ///< Pointer to the scene manager instance
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
