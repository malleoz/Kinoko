#include "KTestSystem.hh"

#include "host/SceneCreatorDynamic.hh"

#include <abstract/File.hh>

#include "egg/core/Scene.hh"
#include <egg/core/SceneManager.hh>

#include <game/kart/KartObject.hh>
#include <game/kart/KartObjectManager.hh>

#include <game/system/RaceConfig.hh>
#include <game/system/RaceManager.hh>

namespace Kinoko {

/// @enum Changelog
/// @brief Represents the changelog entries for the test system.
/// @details We use an unscoped enum to avoid static_casting in all usecases. This is defined in the
/// source due to its lack of scoping.
enum Changelog {
    Initial = 1,
    AddedExtVel = 2,
    AddedIntVel = 3,
    AddedSpeed = 4,
    AddedRotation = 5,
    AddedCheckpoints = 6,
};

/// @brief Initializes the system
/// @details Creates the scene manager, registers the @ref OnInit() callback function, initializes
/// the test suite, and starts the first test case.
void KTestSystem::init() {
    auto *sceneCreator = EGG::egg_new<Host::SceneCreatorDynamic>();
    m_sceneMgr = EGG::egg_new<EGG::SceneManager>(sceneCreator);

    System::RaceConfig::RegisterInitCallback(OnInit, nullptr);
    Abstract::File::Remove("results.txt");

    if (m_testMode == Host::EOption::Suite) {
        initSuite();
    }

    startNextTestCase();
    m_sceneMgr->changeScene(0);
}

/// @brief Executes a frame
void KTestSystem::calc() {
    m_sceneMgr->calc();
}

/// @brief Executes a run
/// @details A run consists of iterating over all tests.
/// @return Whether the run was successful or not.
bool KTestSystem::run() {
    bool success = true;

    while (true) {
        success &= runTest();

        if (!popTestCase()) {
            break;
        }

        // TODO: Use a system heap! We currently have a dependency on the scene heap
        m_sceneMgr->destroyScene(m_sceneMgr->currentScene());
        startNextTestCase();
        m_sceneMgr->createScene(2, m_sceneMgr->currentScene());
    }

    return success;
}

/// @brief Parses non-generic command line options.
/// @details Test mode currently accepts either a suite of tests or a RKG + KRKG.
/// @param argc The number of arguments.
/// @param argv The arguments.
void KTestSystem::parseOptions(int argc, char **argv) {
    if (argc < 2) {
        PANIC("Expected suite/ghost/krkg argument!");
    }

    std::optional<char *> rkgPath;
    std::optional<char *> krkgPath;
    std::optional<u16> target;

    for (int i = 0; i < argc; ++i) {
        std::optional<Host::EOption> flag = Host::Option::CheckFlag(argv[i]);
        if (!flag || *flag == Host::EOption::Invalid) {
            WARN("Expected a flag! Got: %s", argv[i]);
            continue;
        }

        switch (*flag) {
        case Host::EOption::Suite: {
            if (m_testMode != Host::EOption::Invalid) {
                PANIC("Mode was already set!");
            }

            m_testMode = Host::EOption::Suite;

            ASSERT(i + 1 < argc);

            std::span<const u8> data = Abstract::File::Load(argv[++i]);

            if (data.empty()) {
                PANIC("Failed to load suite data!");
            }

            m_stream = EGG::RamStream(data.data(), static_cast<u32>(data.size()));
            m_stream.setEndian(std::endian::big);

        } break;
        case Host::EOption::Ghost:
            if (m_testMode != Host::EOption::Invalid && m_testMode != Host::EOption::Ghost) {
                PANIC("Mode was already set!");
            }

            m_testMode = Host::EOption::Ghost;
            ASSERT(i + 1 < argc);
            rkgPath = argv[++i];

            break;
        case Host::EOption::KRKG:
            if (m_testMode != Host::EOption::Invalid && m_testMode != Host::EOption::Ghost) {
                PANIC("Mode was already set!");
            }

            m_testMode = Host::EOption::Ghost;
            ASSERT(i + 1 < argc);
            krkgPath = argv[++i];

            break;
        case Host::EOption::TargetFrame:
            ASSERT(i + 1 < argc);
            {
                if (strlen(argv[++i]) > 5) {
                    PANIC("Target has too many digits");
                }
                target = atoi(argv[i]);
                if (target < 0 || target > std::numeric_limits<u16>::max()) {
                    PANIC("Target is out of bounds (expected 0-65535), got %d\n", *target);
                }
            }

            break;
        case Host::EOption::Invalid:
        default:
            PANIC("Invalid flag!");
            break;
        }
    }

    if (target && m_testMode != Host::EOption::Ghost) {
        PANIC("'--framecount' is only supported in a single ghost test");
    }

    if (m_testMode == Host::EOption::Ghost) {
        if (!rkgPath) {
            PANIC("Missing ghost argument!");
        }

        if (!krkgPath) {
            PANIC("Missing KRKG argument!");
        }

        if (!target) {
            target = 0;
        }

        m_testCases.emplace(*rkgPath, *rkgPath, *krkgPath, *target);
    }
}

/// @brief Creates the singleton instance of the @ref KTestSystem
/// @return A pointer to the newly created @ref KTestSystem instance
/// @pre The singleton instance of the @ref KTestSystem has not been created yet.
KTestSystem *KTestSystem::CreateInstance() {
    ASSERT(!s_instance);
    s_instance = EGG::egg_new<KTestSystem>();
    return static_cast<KTestSystem *>(s_instance);
}

/// @brief Destroys the singleton instance of the @ref KTestSystem
/// @pre The singleton instance of the @ref KTestSystem has already been created.
void KTestSystem::DestroyInstance() {
    ASSERT(s_instance);
    auto *instance = s_instance;
    s_instance = nullptr;
    EGG::egg_delete(instance);
}

/// @brief Returns the singleton instance of the @ref KTestSystem
/// @return A pointer to the singleton instance of the @ref KTestSystem
KTestSystem *KTestSystem::Instance() {
    return static_cast<KTestSystem *>(s_instance);
}

/// @brief Default private constructor
/// @details Initializes @ref m_testMode to @ref Host::EOption::Invalid.
KTestSystem::KTestSystem() : m_testMode(Host::EOption::Invalid) {}

/// @brief Private virtual destructor
KTestSystem::~KTestSystem() {
    if (s_instance) {
        s_instance = nullptr;
        WARN("KTestSystem instance not explicitly handled!");
    }
}

/// @brief Checks for equivalence between two values of integral type `T` and reports a
/// desynchronization if they differ
/// @tparam T The integral type of the values to check for equivalence
/// @param t0 The first value to compare
/// @param t1 The second value to compare
/// @param name The name of the datapoint being checked
/// @details If the two types do not match, then reports the desync and sets @ref m_sync to
/// false.
template <IntegralType T>
void KTestSystem::checkDesync(const T &t0, const T &t1, const char *name) {
    if (t0 == t1) {
        return;
    }

    if (m_sync) {
        REPORT("Test Case Failed: %s [%d / %d]", getCurrentTestCase().name.c_str(), m_currentFrame,
                m_frameCount);
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
void KTestSystem::checkDesync(const T &t0, const T &t1, const char *name) {
    if (t0 == t1) {
        return;
    }

    m_sceneMgr->currentScene()->heap()->enableAllocation();

    if (m_sync) {
        REPORT("Test Case Failed: %s [%d / %d]", getCurrentTestCase().name.c_str(), m_currentFrame,
                m_frameCount);
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
void KTestSystem::checkDesync(const f32 &t0, const f32 &t1, const char *name) {
    if (t0 == t1) {
        return;
    }

    m_sceneMgr->currentScene()->heap()->enableAllocation();

    if (m_sync) {
        REPORT("Test Case Failed: %s [%d / %d]", getCurrentTestCase().name.c_str(), m_currentFrame,
                m_frameCount);
    }

    REPORT("DESYNC! Name: %s", name);
    std::string s0 = std::to_string(t0);
    std::string s1 = std::to_string(t1);
    REPORT("Expected: 0x%08X | %s", f2u(t0), s0.c_str());
    REPORT("Observed: 0x%08X | %s", f2u(t1), s1.c_str());

    m_sceneMgr->currentScene()->heap()->disableAllocation();

    m_sync = false;
}

/// @brief Initializes the test suite by reading the KRKG file and generating the list of test cases
/// @details Validates that the header and footer signatures are present in the file. Validates that
/// the `.krkg` file version is supported by the Kinoko executable. Validates that `.rkg` and
/// `.krkg` filenames are specified for each test case.
void KTestSystem::initSuite() {
    constexpr u32 TEST_HEADER_SIGNATURE = 0x54535448; // TSTH
    constexpr u32 TEST_FOOTER_SIGNATURE = 0x54535446; // TSTF
    constexpr u16 SUITE_MAJOR_VER = 1;
    constexpr u16 SUITE_MAX_MINOR_VER = 0;

    u16 numTestCases = m_stream.read_u16();
    u16 testMajorVer = m_stream.read_u16();
    u16 testMinorVer = m_stream.read_u16();

    if (testMajorVer != SUITE_MAJOR_VER || testMinorVer > SUITE_MAX_MINOR_VER) {
        PANIC("Version not supported! Provided file is %d.%d while Kinoko supports up to %d.%d",
                testMajorVer, testMinorVer, SUITE_MAJOR_VER, SUITE_MAX_MINOR_VER);
    }

    for (u16 i = 0; i < numTestCases; ++i) {
        // Validate alignment
        if (m_stream.read_u32() != TEST_HEADER_SIGNATURE) {
            PANIC("Invalid binary data for test case!");
        }

        u16 totalSize = m_stream.read_u16();
        TestCase testCase;

        u16 nameLen = m_stream.read_u16();
        testCase.name = m_stream.read_string();
        if (nameLen != testCase.name.size() + 1) {
            PANIC("Test case name length mismatch!");
        }

        u16 rkgPathLen = m_stream.read_u16();
        testCase.rkgPath = m_stream.read_string();
        if (rkgPathLen != testCase.rkgPath.size() + 1) {
            PANIC("Test case RKG Path length mismatch!");
        }

        u16 krkgPathLen = m_stream.read_u16();
        testCase.krkgPath = m_stream.read_string();
        if (krkgPathLen != testCase.krkgPath.size() + 1) {
            PANIC("Test case KRKG Path length mismatch!");
        }

        testCase.targetFrame = m_stream.read_u16();

        // Validate alignment
        if (m_stream.read_u32() != TEST_FOOTER_SIGNATURE) {
            PANIC("Invalid binary data for test case!");
        }

        if (totalSize != sizeof(u16) * 4 + nameLen + rkgPathLen + krkgPathLen) {
            PANIC("Unexpected bytes in test case");
        }

        m_testCases.push(testCase);
    }
}

/// @brief Starts the next test case.
/// @details Reads in data from the `KRKG` file and initializes the test system's state accordingly.
void KTestSystem::startNextTestCase() {
    constexpr u32 KRKG_SIGNATURE = 0x4b524b47; // KRKG

    std::span<const u8> krkg = Abstract::File::Load(getCurrentTestCase().krkgPath.data());
    m_stream = EGG::RamStream(krkg.data(), static_cast<u32>(krkg.size()));
    m_currentFrame = -1;
    m_sync = true;

    // Initialize endianness for the RAM stream
    u16 mark = reinterpret_cast<const TestHeader *>(krkg.data())->byteOrderMark;
    std::endian endian = parse<u16>(mark) == 0xfeff ? std::endian::big : std::endian::little;
    m_stream.setEndian(endian);

    ASSERT(m_stream.read_u32() == KRKG_SIGNATURE);
    m_stream.skip(2);
    m_frameCount = m_stream.read_u16();
    m_versionMajor = m_stream.read_u16();
    m_versionMinor = m_stream.read_u16();

    ASSERT(m_stream.read_u32() == m_stream.index());

    // If we're in Ghost mode instead of Suite mode and framecount not specified, then target the
    // total framecount of the KRKG.
    if (m_testMode == Host::EOption::Ghost) {
        ASSERT(m_testCases.size() == 1);
        auto &front = m_testCases.front();
        if (front.targetFrame == 0) {
            front.targetFrame = m_frameCount;
        }

        front.targetFrame = std::min(front.targetFrame, m_frameCount);
    }
}

/// @brief Pops the current test case and frees the KRKG buffer.
/// @return `true` if the queue still has elements remaining, `false` otherwise
bool KTestSystem::popTestCase() {
    ASSERT(m_testCases.size() > 0);
    m_testCases.pop();
    EGG::egg_free(m_stream.data());

    return !m_testCases.empty();
}

/// @brief Checks one frame in the test.
/// @return Whether the test can continue.
/// @details Increments the system's evaluated frame counter. Checks if the test case has reached
/// its target frame. If the test case needs to continue, checks the frame's physics match expected
/// values.
bool KTestSystem::calcTest() {
    ++m_currentFrame;

    // Check if we're out of frames
    u16 targetFrame = getCurrentTestCase().targetFrame;
    ASSERT(targetFrame <= m_frameCount);
    if (m_currentFrame > targetFrame) {
        REPORT("Test Case Passed: %s [%d / %d]", getCurrentTestCase().name.c_str(), targetFrame,
                m_frameCount);
        return false;
    }

    // Test the current frame
    testFrame(getCurrentFrameTestData());
    return m_sync;
}

/// @brief Gets the test data of the current frame.
/// @return The test data of the current frame.
KTestSystem::TestData KTestSystem::getCurrentFrameTestData() {
    EGG::Vector3f pos;
    EGG::Quatf fullRot;
    EGG::Vector3f extVel;
    EGG::Vector3f intVel;
    f32 speed = 0.0f;
    f32 acceleration = 0.0f;
    f32 softSpeedLimit = 0.0f;
    EGG::Quatf mainRot = EGG::Quatf::ident; // Initialize to avoid maybe-uninitialized warning
    EGG::Vector3f angVel2;
    f32 raceCompletion = 0.0f;
    u16 checkpointId = 0;
    u8 jugemId = 0;

    pos.read(m_stream);
    fullRot.read(m_stream);

    if (m_versionMinor >= Changelog::AddedExtVel) {
        extVel.read(m_stream);
    }

    if (m_versionMinor >= Changelog::AddedIntVel) {
        intVel.read(m_stream);
    }

    if (m_versionMinor >= Changelog::AddedSpeed) {
        speed = m_stream.read_f32();
        acceleration = m_stream.read_f32();
        softSpeedLimit = m_stream.read_f32();
    }

    if (m_versionMinor >= Changelog::AddedRotation) {
        mainRot.read(m_stream);
        angVel2.read(m_stream);
    }

    if (m_versionMinor >= Changelog::AddedCheckpoints) {
        raceCompletion = m_stream.read_f32();
        checkpointId = m_stream.read_u16();
        jugemId = m_stream.read_u8();
        m_stream.skip(1);
    }

    TestData data;
    data.pos = pos;
    data.fullRot = fullRot;
    data.extVel = extVel;
    data.intVel = intVel;
    data.speed = speed;
    data.acceleration = acceleration;
    data.softSpeedLimit = softSpeedLimit;
    data.mainRot = mainRot;
    data.angVel2 = angVel2;
    data.raceCompletion = raceCompletion;
    data.checkpointId = checkpointId;
    data.jugemId = jugemId;
    return data;
}

/// @brief Tests the frame against the provided test data.
/// @param data The test data to compare against.
/// @details If any check fails, sets @ref m_sync to false without short-circuiting, so that we
/// capture all desyncs that occurred.
void KTestSystem::testFrame(const TestData &data) {
    auto *object = Kart::KartObjectManager::Instance()->object(0);
    const auto &pos = object->pos();
    const auto &fullRot = object->fullRot();
    const auto &extVel = object->extVel();
    const auto &intVel = object->intVel();
    f32 speed = object->speed();
    f32 acceleration = object->acceleration();
    f32 softSpeedLimit = object->softSpeedLimit();
    const auto &mainRot = object->mainRot();
    const auto &angVel2 = object->angVel2();

    const auto &player = System::RaceManager::Instance()->player();
    f32 raceCompletion = player.raceCompletion();
    u16 checkpointId = player.checkpointId();
    u8 jugemId = player.jugemId();

    switch (m_versionMinor) {
    case Changelog::AddedCheckpoints:
        checkDesync(data.raceCompletion, raceCompletion, "raceCompletion");
        checkDesync(data.checkpointId, checkpointId, "checkpointId");
        checkDesync(data.jugemId, jugemId, "jugemId");
        [[fallthrough]];
    case Changelog::AddedRotation:
        checkDesync(data.mainRot, mainRot, "mainRot");
        checkDesync(data.angVel2, angVel2, "angVel2");
        [[fallthrough]];
    case Changelog::AddedSpeed:
        checkDesync(data.speed, speed, "speed");
        checkDesync(data.acceleration, acceleration, "acceleration");
        checkDesync(data.softSpeedLimit, softSpeedLimit, "softSpeedLimit");
        [[fallthrough]];
    case Changelog::AddedIntVel:
        checkDesync(data.intVel, intVel, "intVel");
        [[fallthrough]];
    case Changelog::AddedExtVel:
        checkDesync(data.extVel, extVel, "extVel");
        [[fallthrough]];
    default:
        checkDesync(data.pos, pos, "pos");
        checkDesync(data.fullRot, fullRot, "fullRot");
    }
}

/// @brief Runs a single test case, and ends when the test is finished or when a desync is
/// found.
/// @return `true` if the run synchronized, `false` is the run desynced.
/// @details This will also accumulate results in results.txt.
bool KTestSystem::runTest() {
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
void KTestSystem::writeTestOutput() const {
    std::string outStr(getCurrentTestCase().name.data());
    outStr += "\n" + std::string(m_sync ? "1" : "0") + "\n";
    outStr += std::to_string(getCurrentTestCase().targetFrame) + "\n";
    outStr += std::to_string(m_frameCount) + "\n";
    Abstract::File::Append("results.txt", outStr.c_str(), outStr.size());
}

/// @brief Gets the current test case.
/// @return The current test case.
/// @details In the event that there is no active test case, this gets the next test case.
/// @pre There is at least one test case in the queue.
const KTestSystem::TestCase &KTestSystem::getCurrentTestCase() const {
    ASSERT(!m_testCases.empty());
    return m_testCases.front();
}

/// @brief Initializes the race configuration as needed for test cases.
/// @param config The race configuration instance.
/// @details Calls @ref System::RaceConfig::setGhost() with the current test case's RKG
/// filepath and changes the player type to `Ghost`.
void KTestSystem::OnInit(System::RaceConfig *config, void * /* arg */) {
    std::span<const u8> rkg = Abstract::File::Load(Instance()->getCurrentTestCase().rkgPath.data());
    config->setGhost(rkg.data());
    EGG::egg_free(const_cast<u8 *>(rkg.data()));

    config->raceScenario().players[0].type = System::RaceConfig::Player::Type::Ghost;
}

} // namespace Kinoko
