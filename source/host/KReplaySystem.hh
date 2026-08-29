#pragma once

#include "host/KSystem.hh"
#include "host/SceneCreatorDynamic.hh"

#include <abstract/File.hh>

#include <egg/core/SceneManager.hh>

#include <game/system/RaceConfig.hh>

#include <filesystem>
#include <queue>

namespace Kinoko {

/// @brief Kinoko system designed to execute replays.
class KReplaySystem : public KSystem {
public:
    /// @brief Initializes the system.
    void init() override {
        auto *sceneCreator = EGG::egg_new<Host::SceneCreatorDynamic>();
        m_sceneMgr = EGG::egg_new<EGG::SceneManager>(sceneCreator);

        System::RaceConfig::RegisterInitCallback(OnInit, nullptr);
        Abstract::File::Remove("results.txt");
    }

    /// @brief Executes a frame.
    void calc() override {
        m_sceneMgr->calc();
    }

    bool run() override;
    void parseOptions(int argc, char **argv) override;

    static KReplaySystem *CreateInstance() {
        ASSERT(!s_instance);
        s_instance = EGG::egg_new<KReplaySystem>();
        return static_cast<KReplaySystem *>(s_instance);
    }

    static void DestroyInstance() {
        ASSERT(s_instance);
        auto *instance = s_instance;
        s_instance = nullptr;
        EGG::egg_delete(instance);
    }

    static KReplaySystem *Instance() {
        return static_cast<KReplaySystem *>(s_instance);
    }

    KReplaySystem();
    ~KReplaySystem() override;

private:
    typedef std::pair<const System::Timer &, const System::Timer &> DesyncingTimerPair;

    KReplaySystem(const KReplaySystem &) = delete;
    KReplaySystem(KReplaySystem &&) = delete;

    bool calcEnd() const;

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
    s32 getDesyncingTimerIdx() const;
    DesyncingTimerPair getDesyncingTimer(s32 i) const;

    /// @brief Initializes the race configuration as needed for replays.
    /// @param config The race configuration instance.
    /// @param arg Unused optional argument.
    static void OnInit(System::RaceConfig *config, void * /* arg */) {
        config->setGhost(Instance()->m_currentRawGhost);
        config->raceScenario().players[0].type = System::RaceConfig::Player::Type::Ghost;
    }

    EGG::SceneManager *m_sceneMgr;

    std::queue<std::filesystem::path> m_ghostArgs;
    size_t m_progressInterval;
    size_t m_replaysPlayed;
    size_t m_replaysSynced;
    std::filesystem::path m_currentGhostPath;
    const char *m_currentGhostFileName;
    const System::GhostFile *m_currentGhost;
    const u8 *m_currentRawGhost;
    size_t m_currentRawGhostSize;
};

} // namespace Kinoko
