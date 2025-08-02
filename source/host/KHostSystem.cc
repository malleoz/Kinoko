#include "KHostSystem.hh"

#include "Singleton.hh"

#include "host/SceneCreatorDynamic.hh"

#include <game/system/KPadDirector.hh>
#include <game/system/RaceManager.hh>

#include <cstring>
#include <iostream>

#include <game/kart/KartObjectManager.hh>

// 8 22 23 0 for LC FK FR Manual

/// @brief Initializes the system.
void KHostSystem::init() {
    m_params.course = PromptForCourse();
    m_params.character = PromptForCharacter();
    m_params.vehicle = PromptForVehicle();
    m_params.driftIsAuto = PromptForAuto();

    ValidateParams(m_params);

    auto *sceneCreator = new Host::SceneCreatorDynamic;
    m_sceneMgr = new EGG::SceneManager(sceneCreator);

    System::RaceConfig::RegisterInitCallback(OnInit, &m_params);
    m_sceneMgr->changeScene(0);

    m_controller = Singleton<System::KPadDirector>::Instance()->hostController();
}

/// @brief Executes a frame.
void KHostSystem::calc() {
    m_sceneMgr->calc();

    const auto &pos = Singleton<Kart::KartObjectManager>::Instance()->object(0)->pos();
    REPORT("X: %f | Y: %f | Z: %f", static_cast<double>(pos.x), static_cast<double>(pos.y),
            static_cast<double>(pos.z));
}

/// @brief Persistant command line polling for a command
bool KHostSystem::run() {
    while (true) {
        s32 input;
        std::cin >> input;

        while (input <= static_cast<s32>(Options::Invalid) ||
                input >= static_cast<s32>(Options::Max)) {
            std::cin >> input;
        }

        Options option = static_cast<Options>(input);

        switch (option) {
        case Options::SetInputs:
            promptAndSetInputs();
            break;
        case Options::FrameAdvance:
            calc();
        default:
            break;
        }
    }

    return true;
}

/// @brief Parses non-generic command line options.
void KHostSystem::parseOptions(int /*argc*/, char ** /*argv*/) {
    // TODO
}

KHostSystem *KHostSystem::CreateInstance() {
    s_instance = new KHostSystem;
    return static_cast<KHostSystem *>(s_instance);
}

void KHostSystem::DestroyInstance() {
    ASSERT(s_instance);
    auto *instance = s_instance;
    s_instance = nullptr;
    delete instance;
}

KHostSystem::KHostSystem() = default;

KHostSystem::~KHostSystem() {
    if (s_instance) {
        s_instance = nullptr;
        WARN("KHostSystem instance not explicitly handled!");
    }

    delete m_sceneMgr;
}

void KHostSystem::promptAndSetInputs() {
    std::cout << "Set inputs (ABL UDLR 0-14 0-14)\n";
    char face[4];
    char dpad;
    u32 x, y;

    std::cin >> face >> dpad >> x >> y;

    REPORT("%s", face);
    REPORT("%c", dpad);
    REPORT("%u", x);
    REPORT("%u", y);

    u16 buttons = 0;
    System::Trick trick = System::Trick::None;
    if (strchr(face, 'A') || strchr(face, 'a')) {
        buttons |= 1;
    }
    if (strchr(face, 'B') || strchr(face, 'b')) {
        buttons |= 2;
    }
    if (strchr(face, 'L') || strchr(face, 'l')) {
        buttons |= 4;
    }

    if (dpad == 'U' || dpad == 'u') {
        trick = System::Trick::Up;
    } else if (dpad == 'D' || dpad == 'd') {
        trick = System::Trick::Down;
    } else if (dpad == 'L' || dpad == 'l') {
        trick = System::Trick::Left;
    } else if (dpad == 'R' || dpad == 'r') {
        trick = System::Trick::Right;
    }

    m_controller->setInputsRawStick(buttons, x, y, trick);
}

Course KHostSystem::PromptForCourse() {
    std::cout << "Choose course: ";

    u32 course;
    std::cin >> course;

    DEBUG("%s", COURSE_NAMES[course]);

    return static_cast<Course>(course);
}

Character KHostSystem::PromptForCharacter() {
    std::cout << "Choose character: ";

    u32 character;
    std::cin >> character;

    DEBUG("%s", CHARACTER_NAMES[character]);

    return static_cast<Character>(character);
}

Vehicle KHostSystem::PromptForVehicle() {
    std::cout << "Choose vehicle: ";

    u32 vehicle;
    std::cin >> vehicle;

    DEBUG("%s", VEHICLE_NAMES[vehicle]);

    return static_cast<Vehicle>(vehicle);
}

bool KHostSystem::PromptForAuto() {
    std::cout << "Auto Drift? 1 - Yes, 0 - No";

    bool autoDrift;
    std::cin >> autoDrift;

    return autoDrift;
}

// This is copied from GhostFile.cc. Move to Common.hh?
void KHostSystem::ValidateParams(const KRaceParams &params) {
    // Validate weight class match
    WeightClass charWeight = CharacterToWeight(params.character);
    WeightClass vehicleWeight = VehicleToWeight(params.vehicle);

    if (charWeight == WeightClass::Invalid) {
        PANIC("Invalid character weight class!");
    }
    if (vehicleWeight == WeightClass::Invalid) {
        PANIC("Invalid vehicle weight class!");
    }
    if (charWeight != vehicleWeight) {
        PANIC("Character/Bike weight class mismatch!");
    }
}

/// @brief Initializes the race configuration as needed for replays.
/// @param config The race configuration instance.
/// @param arg Unused optional argument.
void KHostSystem::OnInit(System::RaceConfig *config, void *arg) {
    auto *params = reinterpret_cast<KRaceParams *>(arg);

    auto &scenario = config->raceScenario();
    scenario.course = params->course;

    auto &player = scenario.players[0];
    player.type = System::RaceConfig::Player::Type::Local;
    player.character = params->character;
    player.vehicle = params->vehicle;
    player.driftIsAuto = params->driftIsAuto;
}
