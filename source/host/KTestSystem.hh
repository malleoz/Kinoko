#pragma once

#include <Types.hh>

#include "host/KSystem.hh"
#include "host/Option.hh"

#include <egg/core/Heap.hh>
#include <egg/math/Quat.hh>

#include <queue>

namespace Kinoko {

namespace EGG {

class SceneManager;

} // namespace EGG

namespace System {

class RaceConfig;

} // namespace System

/// @brief Kinoko system designed to execute tests.
/// @details A test is defined by a combination of an `.rkg` file, a `.krkg` file, and a target
/// frame at which playback should end (by default, race end).
/// @todo Create page to document the `.krkg` file format
class KTestSystem final : public KSystem {
public:
    void init() override;
    void calc() override;
    bool run() override;
    void parseOptions(int argc, char **argv) override;

    static KTestSystem *CreateInstance();
    static void DestroyInstance();
    [[nodiscard]] static KTestSystem *Instance();

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

    template <IntegralType T>
    void checkDesync(const T &t0, const T &t1, const char *name);

    template <typename T>
    void checkDesync(const T &t0, const T &t1, const char *name);

    void checkDesync(const f32 &t0, const f32 &t1, const char *name);

    void initSuite();
    void startNextTestCase();
    bool popTestCase();
    bool calcTest();
    [[nodiscard]] TestData getCurrentFrameTestData();
    void testFrame(const TestData &data);
    bool runTest();
    void writeTestOutput() const;

    [[nodiscard]] const TestCase &getCurrentTestCase() const;

    static void OnInit(System::RaceConfig *config, void *arg);

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
