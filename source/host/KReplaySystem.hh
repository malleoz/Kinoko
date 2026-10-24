#pragma once

#include "host/KSystem.hh"

#include <egg/core/SceneManager.hh>

#include <game/system/RaceConfig.hh>

#include <array>
#include <optional>

namespace Kinoko {

/// @brief Kinoko system designed to execute replays.
/// @details In the default mode, a single ghost is provided via the ghost flag. In worker mode,
/// ghost paths are read line-by-line from stdin and each ghost is replayed in the same process.
/// For every ghost, exactly one line is written to stdout of the form:
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

    static KReplaySystem *Instance() {
        return static_cast<KReplaySystem *>(s_instance);
    }

    KReplaySystem();
    ~KReplaySystem() override;

private:
    typedef std::pair<const System::Timer &, const System::Timer &> DesyncingTimerPair;

    enum class Result {
        Sync,
        Desync,
        NoFinish,
    };

    KReplaySystem(const KReplaySystem &) = delete;
    KReplaySystem(KReplaySystem &&) = delete;

    bool runWorker();
    bool readWorkerGhostPath();
    void reportWorkerResult(const char *status, const char *msg) const;

    bool loadGhost(const char *path, const char *&err);
    bool calcEnd() const;
    void reportFail(const std::string &msg) const;

    Result checkResult();
    s32 getDesyncingTimerIdx() const;
    DesyncingTimerPair getDesyncingTimer(s32 i) const;

    static void OnInit(System::RaceConfig *config, void *arg);

    EGG::SceneManager *m_sceneMgr;
    bool m_workerMode;

    const char *m_currentGhostFileName;
    std::optional<System::GhostFile> m_currentGhost;

    /// Raw file contents of the current ghost. Fixed-size so that replaying many ghosts in
    /// worker mode does not require any per-ghost heap allocation.
    std::array<u8, sizeof(System::RawGhostFile)> m_rawGhost;
    std::array<char, 4096> m_workerGhostPath;
    std::array<char, 256> m_failMsg;
};

} // namespace Kinoko
