#include "KHostSystem.hh"

#include "host/SceneCreatorDynamic.hh"

#include <game/system/KPadDirector.hh>
#include <game/system/RaceManager.hh>

#include <cstring>
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
    m_savestateMgr.setSceneMgr(m_sceneMgr);

    System::RaceConfig::RegisterInitCallback(OnInit, &m_params);
    m_sceneMgr->changeScene(0);

    m_controller = System::KPadDirector::Instance()->hostController();
    m_savestateMgr.setController(m_controller);
}

/// @brief Executes a frame.
void KHostSystem::calc() {
    m_sceneMgr->calc();
    m_savestateMgr.calcState();
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
            break;
        case Options::LoadState:
            loadState();
            break;
        case Options::SaveState:
            saveState();
            break;
        case Options::LoadContext:
            loadContext();
            break;
        case Options::SaveContext:
            saveContext();
            break;
        case Options::ChangeRace:
            changeRace();
            break;
        default:
            break;
        }
    }

    return true;
}

KHostSystem *KHostSystem::CreateInstance() {
    ASSERT(!s_instance);
    s_instance = reinterpret_cast<KHostSystem *>(malloc(sizeof(KHostSystem)));
    new (s_instance) KHostSystem;
    return static_cast<KHostSystem *>(s_instance);
}

void KHostSystem::DestroyInstance() {
    ASSERT(s_instance);
    auto *instance = s_instance;
    s_instance = nullptr;
    free(instance);
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

void KHostSystem::loadState() {
    size_t slot;
    std::cin >> slot;

    m_savestateMgr.loadState(slot);
}

void KHostSystem::saveState() {
    size_t slot;
    std::cin >> slot;

    m_savestateMgr.saveState(slot);
}

void KHostSystem::loadContext() {
    size_t slot;
    std::cin >> slot;

    m_savestateMgr.loadContext(slot);
}

void KHostSystem::saveContext() {
    size_t slot;
    std::cin >> slot;

    m_savestateMgr.saveContext(slot);
}

void KHostSystem::changeRace() {
    m_params.course = PromptForCourse();
    m_params.character = PromptForCharacter();
    m_params.vehicle = PromptForVehicle();
    m_params.driftIsAuto = PromptForAuto();

    ValidateParams(m_params);

    m_sceneMgr->destroyScene(m_sceneMgr->currentScene());

    System::RaceConfig::RegisterInitCallback(OnInit, &m_params);

    m_sceneMgr->createScene(2, m_sceneMgr->currentScene());
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

    // We don't need the callback anymore. In fact, if we keep it, then it will interfere with
    // savestates by overwriting any RaceScenario assignments that happen from the savestate load.
    System::RaceConfig::RegisterInitCallback(nullptr, nullptr);
}
