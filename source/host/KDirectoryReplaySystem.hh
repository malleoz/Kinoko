#include "host/KSystem.hh"

#include <abstract/DVDFile.hh>

#include <egg/core/SceneManager.hh>

#include <game/system/RaceConfig.hh>

#include <atomic>
#include <generator>

/// @brief Kinoko system designed to execute replays from a given directory
class KDirectoryReplaySystem : public KSystem {
public:
    void init() override;
    void calc() override;
    bool run() override;
    void parseOptions(int argc, char **argv) override;

    static KDirectoryReplaySystem *CreateInstance();
    static void DestroyInstance();

    static KDirectoryReplaySystem *Instance() {
        return static_cast<KDirectoryReplaySystem *>(s_instance);
    }

private:
    typedef std::pair<const System::Timer &, const System::Timer &> DesyncingTimerPair;

    KDirectoryReplaySystem();
    KDirectoryReplaySystem(const KDirectoryReplaySystem &) = delete;
    KDirectoryReplaySystem(KDirectoryReplaySystem &&) = delete;
    ~KDirectoryReplaySystem() override;

    void runGhost();
    bool calcEnd() const;
    bool success() const;
    void reportFail(const std::string &msg) const;
    s32 getDesyncingTimerIdx() const;
    DesyncingTimerPair getDesyncingTimer(s32 i) const;

    static void OnInit(System::RaceConfig *config, void *arg);

    std::optional<std::generator<Abstract::DVDFile>> m_fileGenerator;

    EGG::SceneManager *m_sceneMgr;
    Abstract::DVDFile m_currentGhostFile;
    std::optional<System::GhostFile> m_currentGhost;

    std::atomic<size_t> m_replayCount;

    // Every thread will need its own ghost heap most likely
    EGG::ExpHeap *m_ghostHeap;
};
