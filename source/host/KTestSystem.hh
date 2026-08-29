#pragma once

#include "host/KSystem.hh"
#include "host/Option.hh"

#include <abstract/File.hh>

#include <egg/core/SceneManager.hh>
#include <egg/math/Quat.hh>

#include <game/system/RaceConfig.hh>

#include <queue>

namespace Kinoko {

/// @brief Kinoko system designed to execute tests.
class KTestSystem final : public KSystem {
public:
    void init() override;

    /// @brief Executes a frame.
    void calc() override {
        m_sceneMgr->calc();
    }

    bool run() override;
    void parseOptions(int argc, char **argv) override;

    static KTestSystem *CreateInstance() {
        ASSERT(!s_instance);
        s_instance = EGG::egg_new<KTestSystem>();
        return static_cast<KTestSystem *>(s_instance);
    }

    static void DestroyInstance() {
        ASSERT(s_instance);
        auto *instance = s_instance;
        s_instance = nullptr;
        EGG::egg_delete(instance);
    }

    static KTestSystem *Instance() {
        return static_cast<KTestSystem *>(s_instance);
    }

private:
    struct TestCase {
        std::string name;
        std::string rkgPath;
        std::string krkgPath;
        u16 targetFrame;
    };

    struct TestData {
        EGG::Vector3f pos;
        EGG::Quatf fullRot;
        // Added in 0.2
        EGG::Vector3f extVel;
        // Added in 0.3
        EGG::Vector3f intVel;
        // Added in 0.4
        f32 speed;
        f32 acceleration;
        f32 softSpeedLimit;
        // Added in 0.5
        EGG::Quatf mainRot;
        EGG::Vector3f angVel2;
        // Added in 0.6
        f32 raceCompletion;
        u16 checkpointId;
        u8 jugemId;
    };

    EGG_NEW_DELETE_FRIEND

    KTestSystem();
    ~KTestSystem() override;

    KTestSystem(const KTestSystem &) = delete;
    KTestSystem(KTestSystem &&) = delete;

    template <IntegralType T>
    void checkDesync(const T &t0, const T &t1, const char *name) {
        if (t0 == t1) {
            return;
        }

        if (m_sync) {
            REPORT("Test Case Failed: %s [%d / %d]", getCurrentTestCase().name.c_str(),
                    m_currentFrame, m_frameCount);
        }

        REPORT("DESYNC! Name: %s", name);
        REPORT("Expected: %d", t0);
        REPORT("Observed: %d", t1);

        m_sync = false;
    }

    template <typename T>
    void checkDesync(const T &t0, const T &t1, const char *name) {
        if (t0 == t1) {
            return;
        }

        m_sceneMgr->currentScene()->heap()->enableAllocation();

        if (m_sync) {
            REPORT("Test Case Failed: %s [%d / %d]", getCurrentTestCase().name.c_str(),
                    m_currentFrame, m_frameCount);
        }

        REPORT("DESYNC! Name: %s", name);
        std::string s0(t0);
        std::string s1(t1);
        REPORT("Expected: %s", s0.c_str());
        REPORT("Observed: %s", s1.c_str());

        m_sceneMgr->currentScene()->heap()->disableAllocation();

        m_sync = false;
    }

    void checkDesync(const f32 &t0, const f32 &t1, const char *name) {
        if (t0 == t1) {
            return;
        }

        m_sceneMgr->currentScene()->heap()->enableAllocation();

        if (m_sync) {
            REPORT("Test Case Failed: %s [%d / %d]", getCurrentTestCase().name.c_str(),
                    m_currentFrame, m_frameCount);
        }

        REPORT("DESYNC! Name: %s", name);
        std::string s0 = std::to_string(t0);
        std::string s1 = std::to_string(t1);
        REPORT("Expected: 0x%08X | %s", f2u(t0), s0.c_str());
        REPORT("Observed: 0x%08X | %s", f2u(t1), s1.c_str());

        m_sceneMgr->currentScene()->heap()->disableAllocation();

        m_sync = false;
    }

    void initSuite();

    void startNextTestCase();

    /// @brief Pops the current test case and frees the KRKG buffer.
    /// @return Whether the queue still has elements remaining.
    bool popTestCase() {
        ASSERT(m_testCases.size() > 0);
        m_testCases.pop();
        EGG::egg_free(m_stream.data());

        return !m_testCases.empty();
    }

    bool calcTest();
    TestData findCurrentFrameEntry();
    void testFrame(const TestData &data);

    /// @brief Runs a single test case, and ends when the test is finished or when a desync is
    /// found.
    /// @details This will also accumulate results in results.txt.
    /// @return Whether the run synchronized or desynchronized.
    bool runTest() {
        while (calcTest()) {
            calc();
        }

        // TODO: Use a system heap! std::string relies on heap allocation
        // The heap is destroyed after this and there is no further allocation, so it's not
        // re-disabled
        m_sceneMgr->currentScene()->heap()->enableAllocation();
        writeTestOutput();
        return m_sync;
    }

    /// @brief Writes details about the current test to file.
    /// @details This is designed to be cumulative across multiple tests.
    void writeTestOutput() const {
        std::string outStr(getCurrentTestCase().name.data());
        outStr += "\n" + std::string(m_sync ? "1" : "0") + "\n";
        outStr += std::to_string(getCurrentTestCase().targetFrame) + "\n";
        outStr += std::to_string(m_frameCount) + "\n";
        Abstract::File::Append("results.txt", outStr.c_str(), outStr.size());
    }

    /// @brief Gets the current test case.
    /// @details In the event that there is no active test case, this gets the next test case.
    /// @return The current test case.
    const TestCase &getCurrentTestCase() const {
        ASSERT(!m_testCases.empty());
        return m_testCases.front();
    }

    /// @brief Initializes the race configuration as needed for test cases.
    /// @param config The race configuration instance.
    /// @param arg Unused optional argument.
    static void OnInit(System::RaceConfig *config, void * /* arg */) {
        size_t size;
        u8 *rkg = Abstract::File::Load(Instance()->getCurrentTestCase().rkgPath.data(), size);
        config->setGhost(rkg);
        EGG::egg_free(rkg);

        config->raceScenario().players[0].type = System::RaceConfig::Player::Type::Ghost;
    }

    EGG::SceneManager *m_sceneMgr;
    EGG::RamStream m_stream;
    std::queue<TestCase, std::deque<TestCase, EGG::Allocator<TestCase>>> m_testCases;
    Host::EOption m_testMode; ///< Differentiates between test suite and ghost+krkg

    u16 m_versionMajor;
    u16 m_versionMinor;
    u16 m_frameCount;
    u16 m_currentFrame;
    bool m_sync;
};

} // namespace Kinoko
