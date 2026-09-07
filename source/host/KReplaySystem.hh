#pragma once

#include "host/KSystem.hh"
#include "host/SceneCreatorDynamic.hh"

#include <abstract/File.hh>

#include <egg/core/SceneManager.hh>

#include <game/system/RaceConfig.hh>

#include <filesystem>
#include <queue>

namespace Kinoko {

/// @brief Kinoko system designed to execute replays
/// @details This mode accepts a sequence of ghost `.rkg` files or directories containing ghost
/// `.rkg` files, in which case this system will recurse into the directories to find all ghost
/// files.
class KReplaySystem : public KSystem {
public:
    /// @brief Initializes the system by creating the scene manager and setting up the race
    /// configuration callback
    void init() override {
        auto *sceneCreator = EGG::egg_new<Host::SceneCreatorDynamic>();
        m_sceneMgr = EGG::egg_new<EGG::SceneManager>(sceneCreator);

        System::RaceConfig::RegisterInitCallback(OnInit, nullptr);
        Abstract::File::Remove("results.txt");
    }

    /// @copydoc KSystem::calc()
    void calc() override {
        m_sceneMgr->calc();
    }

    bool run() override;
    void parseOptions(int argc, char **argv) override;

    /// @brief Creates the singleton instance of the @ref KReplaySystem
    /// @return A pointer to the newly created @ref KReplaySystem instance
    static KReplaySystem *CreateInstance() {
        ASSERT(!s_instance);
        s_instance = EGG::egg_new<KReplaySystem>();
        return static_cast<KReplaySystem *>(s_instance);
    }

    /// @brief Destroys the singleton instance of the @ref KReplaySystem
    static void DestroyInstance() {
        ASSERT(s_instance);
        auto *instance = s_instance;
        s_instance = nullptr;
        EGG::egg_delete(instance);
    }

    /// @brief Returns the singleton instance of the @ref KReplaySystem
    /// @return A pointer to the singleton instance of the @ref KReplaySystem
    static KReplaySystem *Instance() {
        return static_cast<KReplaySystem *>(s_instance);
    }

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

    [[nodiscard]] bool calcEnd() const;

    /// @brief Reports failure to file.
    /// @param msg The message to report.
    void reportFail(const std::string &msg) const {
        std::string report(m_currentGhostPath.string());
        report += "\n" + std::string(msg);
        Abstract::File::Append("results.txt", report.c_str(), report.size());
    }

    bool runDirectory(const std::filesystem::path &dirPath);
    bool runGhost(const std::filesystem::path &ghostPath);
    void loadGhost(const std::filesystem::path &ghostPath);

    bool success() const;
    [[nodiscard]] s32 getDesyncingTimerIdx() const;
    [[nodiscard]] DesyncingTimerPair getDesyncingTimer(s32 i) const;

    /// @brief Initializes the race configuration as needed for replays.
    /// @param config The race configuration instance.
    static void OnInit(System::RaceConfig *config, void * /* arg */) {
        config->setGhost(Instance()->m_currentRawGhost.data());
        config->raceScenario().players[0].type = System::RaceConfig::Player::Type::Ghost;
    }

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
