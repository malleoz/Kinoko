#include "host/KSystem.hh"

#include <abstract/DVDFile.hh>

#include <egg/core/SceneManager.hh>

#include <game/system/RaceConfig.hh>

#include <atomic>
#include <generator>
#include <latch>
#include <thread>
#include <utility>

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

    void startThread();
    std::optional<Abstract::DVDFile> getNextGhost();

    static void OnInit(System::RaceConfig *config, void *arg);

    std::mutex m_generatorMutex;
    std::optional<std::generator<Abstract::DVDFile>> m_fileGenerator;
    std::optional<decltype(std::declval<std::generator<Abstract::DVDFile> &>().begin())>
            m_generatorIt;

    static thread_local std::unique_ptr<EGG::SceneManager> m_sceneMgr;
    static thread_local Abstract::DVDFile m_currentGhostFile;
    static thread_local std::optional<System::GhostFile> m_currentGhost;

    std::atomic<size_t> m_replayCount;
    u16 m_threadCount;
    std::span<std::thread> m_threads;
    std::unique_ptr<std::latch> m_startLatch;

    // Every thread will need its own ghost heap most likely
    static thread_local EGG::ExpHeap *m_ghostHeap;
};
