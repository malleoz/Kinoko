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
/// @details A test is defined by a combination of an `.rkg` file, a `.krkg` file, and a target
/// frame at which playback should end (by default, race end).
/// @todo Create page to document the `.krkg` file format
class KTestSystem final : public KSystem {
public:
    void init() override;

    /// @brief Executes a frame
    void calc() override {
        m_sceneMgr->calc();
    }

    bool run() override;
    void parseOptions(int argc, char **argv) override;

    /// @brief Creates the singleton instance of the @ref KTestSystem
    /// @return A pointer to the newly created @ref KTestSystem instance
    static KTestSystem *CreateInstance() {
        ASSERT(!s_instance);
        s_instance = EGG::egg_new<KTestSystem>();
        return static_cast<KTestSystem *>(s_instance);
    }

    /// @brief Destroys the singleton instance of the @ref KTestSystem
    static void DestroyInstance() {
        ASSERT(s_instance);
        auto *instance = s_instance;
        s_instance = nullptr;
        EGG::egg_delete(instance);
    }

    /// @brief Returns the singleton instance of the @ref KTestSystem
    /// @return A pointer to the singleton instance of the @ref KTestSystem
    static KTestSystem *Instance() {
        return static_cast<KTestSystem *>(s_instance);
    }

private:
    /// @brief Structure representing the header of a KRKG test file
    struct TestHeader {
        u32 signature;     ///< Always KRKG
        u16 byteOrderMark; ///< Indicates file endianness (0xfeff=big-endian, 0xfffe=little-endian)
        u16 frameCount;    ///< Total duration of the test in frames
        u16 versionMajor;  ///< Major version of the KRKG file format for this test
        u16 versionMinor;  ///< Minor version of the KRKG file format for this test
        u32 dataOffset;    ///< Offset to the start of the test data within the file
    };

    /// @brief Structure representing a test case within a Kinoko test suite
    struct TestCase {
        std::string name;     ///< A unique name for the test case
        std::string rkgPath;  ///< Path to the `.rkg` file associated with this test case
        std::string krkgPath; ///< Path to the `.krkg` file associated with this test case
        u16 targetFrame;      ///< The frame at which playback should end for this test case
    };

    /// @brief Structure representing the data of a single frame captured within a `.krkg` file
    struct TestData {
        EGG::Vector3f pos;  ///< Expected value for @ref Kart::KartDynamics::m_pos
        EGG::Quatf fullRot; ///< Expected value for @ref Kart::KartDynamics::m_fullRot
        // Added in 0.2
        EGG::Vector3f extVel; ///< Expected value for @ref Kart::KartDynamics::m_extVel
        // Added in 0.3
        EGG::Vector3f intVel; ///< Expected value for @ref Kart::KartDynamics::m_intVel
        // Added in 0.4
        f32 speed;          ///< Expected value for @ref Kart::KartMove::m_speed
        f32 acceleration;   ///< Expected value for @ref Kart::KartMove::m_acceleration
        f32 softSpeedLimit; ///< Expected value for @ref Kart::KartMove::m_softSpeedLimit
        // Added in 0.5
        EGG::Quatf mainRot;    ///< Expected value for @ref Kart::KartDynamics::m_mainRot
        EGG::Vector3f angVel2; ///< Expected value for @ref Kart::KartDynamics::m_angVel2
        // Added in 0.6
        f32 raceCompletion; ///< Exp. value for @ref System::RaceManager::Player::m_raceCompletion
        u16 checkpointId;   ///< Exp. value for @ref System::RaceManager::Player::m_checkpointId
        u8 jugemId;         ///< Exp. value for @ref System::RaceManager::Player::m_jugemId
    };

    EGG_NEW_DELETE_FRIEND

    KTestSystem();
    ~KTestSystem() override;

    /// @brief Deleted copy constructor
    KTestSystem(const KTestSystem &) = delete;

    /// @brief Deleted move constructor
    KTestSystem(KTestSystem &&) = delete;

    /// @brief Deleted copy assignment operator
    KTestSystem &operator=(const KTestSystem &) = delete;

    /// @brief Deleted move assignment operator
    KTestSystem &operator=(KTestSystem &&) = delete;

    /// @brief Checks for equivalence between two values of integral type `T` and reports a
    /// desynchronization if they differ
    /// @tparam T The integral type of the values to check for equivalence
    /// @param t0 The first value to compare
    /// @param t1 The second value to compare
    /// @param name The name of the datapoint being checked
    /// @details If the two types do not match, then reports the desync and sets @ref m_sync to
    /// false.
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

    /// @brief Checks for equivalence between two values of type `T` and reports a
    /// desynchronization if they differ
    /// @tparam T The type of the values to check for equivalence
    /// @param t0 The first value to compare
    /// @param t1 The second value to compare
    /// @param name The name of the datapoint being checked
    /// @details If the two types do not match, then reports the desync and sets @ref m_sync to
    /// false. This function differs from the other templated version in that it handles
    /// non-integral types that can be converted to `std::string`.
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

    /// @brief Checks for equivalence between two floating point numbers and reports a
    /// desynchronization if they differ
    /// @param t0 The first value to compare
    /// @param t1 The second value to compare
    /// @param name The name of the datapoint being checked
    /// @details If the two types do not match, then reports the desync and sets @ref m_sync to
    /// false. Reports the value of the floats both as raw hexadecimal and as a numeric string.
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
    /// @return `true` if the queue still has elements remaining, `false` otherwise
    bool popTestCase() {
        ASSERT(m_testCases.size() > 0);
        m_testCases.pop();
        EGG::egg_free(m_stream.data());

        return !m_testCases.empty();
    }

    bool calcTest();
    TestData getCurrentFrameTestData();
    void testFrame(const TestData &data);

    /// @brief Runs a single test case, and ends when the test is finished or when a desync is
    /// found.
    /// @return Whether the run synchronized or desynchronized.
    /// @details This will also accumulate results in results.txt.
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
    /// @return The current test case.
    /// @details In the event that there is no active test case, this gets the next test case.
    const TestCase &getCurrentTestCase() const {
        ASSERT(!m_testCases.empty());
        return m_testCases.front();
    }

    /// @brief Initializes the race configuration as needed for test cases.
    /// @param config The race configuration instance.
    static void OnInit(System::RaceConfig *config, void * /* arg */) {
        std::span<const u8> rkg =
                Abstract::File::Load(Instance()->getCurrentTestCase().rkgPath.data());
        config->setGhost(rkg.data());
        EGG::egg_free(const_cast<u8 *>(rkg.data()));

        config->raceScenario().players[0].type = System::RaceConfig::Player::Type::Ghost;
    }

    EGG::SceneManager *m_sceneMgr; ///< Pointer to the scene manager instance
    EGG::RamStream m_stream;       ///< Stream containing the test case data
    std::queue<TestCase, alloc_deque<TestCase>> m_testCases; ///< Queue of test cases
    Host::EOption m_testMode; ///< Differentiates between test suite and ghost+krkg

    u16 m_versionMajor; ///< Major version of the test system
    u16 m_versionMinor; ///< Minor version of the test system
    u16 m_frameCount;   ///< Total number of frames in the current test
    u16 m_currentFrame; ///< The current frame being processed
    bool m_sync;        ///< Whether the test run synchronized or desynchronized
};

} // namespace Kinoko
