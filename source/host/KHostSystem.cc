#include "KHostSystem.hh"

#include "host/SceneCreatorDynamic.hh"

#include <game/system/KPadDirector.hh>
#include <game/system/RaceManager.hh>

#include <iostream>

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

    m_controller = System::KPadDirector::Instance()->hostController();
}

/// @brief Executes an action.
void KHostSystem::calc() {
    // TODO
}

/// @brief Persistant command line polling for a command
bool KHostSystem::run() {
    // TODO
    return false;
}

/// @brief Parses non-generic command line options.
void KHostSystem::parseOptions(int /*argc*/, char ** /*argv*/) {
    // TODO
}

KHostSystem *KHostSystem::CreateInstance() {
    ASSERT(!s_instance);
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
    std::cout << "Choose character: ";

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
