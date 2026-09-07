#include "KartParamFileManager.hh"

namespace Kinoko::Kart {

/// @addr{0x80591C9C}
/// @brief Clears all loaded parameter files from the manager (but does not clear them from memory)
void KartParamFileManager::clear() {
    m_kartParam = {};
    m_driverParam = {};
    m_bikeDispParam = {};
    m_kartDispParam = {};
    m_kartCameraParam = {};
}

/// @addr{0x805919F4}
/// @brief Loads and validates the kart parameter files.
void KartParamFileManager::init() {
    m_kartParam = load("kartParam.bin");
    m_driverParam = load("driverParam.bin");
    m_bikeDispParam = load("bikePartsDispParam.bin");
    m_kartDispParam = load("kartPartsDispParam.bin");
    m_kartCameraParam = load("kartCameraParam.bin");
    if (!validate()) {
        PANIC("Parameter files could not be validated!");
    }
}

/// @brief Gets a @ref EGG::RamStream for the provided character's parameters from `driverParam.bin`
/// @param character The character to get the parameters for
/// @return A @ref EGG::RamStream containing the character's parameters
/// @details Panics if the character is invalid and asserts that the `driverParam.bin` file is
/// loaded.
EGG::RamStream KartParamFileManager::getDriverStream(Character character) const {
    s32 idx;
    switch (character) {
    case Character::Small_Mii_Outfit_A_Male:
    case Character::Small_Mii_Outfit_A_Female:
    case Character::Small_Mii_Outfit_B_Male:
    case Character::Small_Mii_Outfit_B_Female:
    case Character::Small_Mii_Outfit_C_Male:
    case Character::Small_Mii_Outfit_C_Female:
    case Character::Small_Mii:
        idx = 23;
        break;
    case Character::Medium_Mii_Outfit_A_Male:
    case Character::Medium_Mii_Outfit_A_Female:
    case Character::Medium_Mii_Outfit_B_Male:
    case Character::Medium_Mii_Outfit_B_Female:
    case Character::Medium_Mii_Outfit_C_Male:
    case Character::Medium_Mii_Outfit_C_Female:
    case Character::Medium_Mii:
        idx = 24;
        break;
    case Character::Large_Mii_Outfit_A_Male:
    case Character::Large_Mii_Outfit_A_Female:
    case Character::Large_Mii_Outfit_B_Male:
    case Character::Large_Mii_Outfit_B_Female:
    case Character::Large_Mii_Outfit_C_Male:
    case Character::Large_Mii_Outfit_C_Female:
    case Character::Large_Mii:
        idx = 25;
        break;
    default:
        if (character > Character::Rosalina) {
            PANIC("Invalid character.");
        }

        idx = static_cast<s32>(character);
        break;
    }

    auto *file = reinterpret_cast<const ParamFile<KartParam::Stats> *>(m_driverParam.data());
    ASSERT(file);
    return EGG::RamStream(&file->params[idx], sizeof(KartParam::Stats));
}

/// @brief Gets a @ref EGG::RamStream for the provided vehicle's parameters from `kartParam.bin`
/// @param vehicle The vehicle to get the parameters for
/// @return A @ref EGG::RamStream containing the vehicle's parameters
/// @details Panics if the vehicle is invalid and asserts that the `kartParam.bin` file is loaded.
EGG::RamStream KartParamFileManager::getVehicleStream(Vehicle vehicle) const {
    if (vehicle >= Vehicle::Max) {
        PANIC("Invalid vehicle.");
    }

    s32 idx = static_cast<s32>(vehicle);
    auto *file = reinterpret_cast<const ParamFile<KartParam::Stats> *>(m_kartParam.data());
    ASSERT(file);
    return EGG::RamStream(&file->params[idx], sizeof(KartParam::Stats));
}

/// @brief Gets a @ref EGG::RamStream for the vehicle's hitbox params from the kart's @ref BSP file
/// @param vehicle The vehicle to get the hitbox params for
/// @return A @ref EGG::RamStream containing the vehicle's hitbox params
/// @details Panics if the vehicle is invalid and asserts that the vehicle's @ref BSP file is loaded
/// and has the correct size.
EGG::RamStream KartParamFileManager::getHitboxStream(Vehicle vehicle) const {
    if (vehicle >= Vehicle::Max) {
        PANIC("Invalid vehicle.");
    }

    auto *resourceManager = System::ResourceManager::Instance();

    std::span<const u8> file = resourceManager->getBsp(vehicle);
    ASSERT(!file.empty());
    ASSERT(file.size() == sizeof(BSP));
    return EGG::RamStream(file.data(), static_cast<u32>(file.size()));
}

/// @brief Gets a @ref EGG::RamStream for the provided bike's display parameters from
/// `bikePartsDispParam.bin`
/// @param vehicle The bike to get the display parameters for
/// @return A @ref EGG::RamStream containing the bike's display parameters
/// @details Panics if the vehicle is invalid and asserts that the `bikePartsDispParam.bin` file is
/// loaded.
EGG::RamStream KartParamFileManager::getBikeDispParamsStream(Vehicle vehicle) const {
    if (vehicle < Vehicle::Standard_Bike_S || vehicle >= Vehicle::Max) {
        PANIC("Invalid vehicle.");
    }

    // We need to index at the correct offset
    constexpr u32 KART_MAX = 18;
    s32 idx = static_cast<s32>(vehicle) - KART_MAX;

    auto *file = reinterpret_cast<const ParamFile<KartParam::BikeDisp> *>(m_bikeDispParam.data());
    ASSERT(file);
    return EGG::RamStream(&file->params[idx], sizeof(KartParam::BikeDisp));
}

/// @brief Gets a @ref EGG::RamStream for the provided kart's display parameters from
/// `kartPartsDispParam.bin`
/// @param vehicle The kart to get the display parameters for
/// @return A @ref EGG::RamStream containing the kart's display parameters
/// @details Panics if the vehicle is invalid and asserts that the `kartPartsDispParam.bin` file is
/// loaded.
EGG::RamStream KartParamFileManager::getKartDispParamsStream(Vehicle vehicle) const {
    if (vehicle < Vehicle::Standard_Kart_S || vehicle > Vehicle::Honeycoupe) {
        PANIC("Invalid vehicle.");
    }

    s32 idx = static_cast<s32>(vehicle);

    auto *file = reinterpret_cast<const ParamFile<KartParam::KartDisp> *>(m_kartDispParam.data());
    ASSERT(file);
    return EGG::RamStream(&file->params[idx], sizeof(KartParam::KartDisp));
}

/// @brief Gets a @ref EGG::RamStream for the provided character's camera parameters from
/// `kartCameraParam.bin`
/// @param character The character to get the camera parameters for
/// @return A @ref EGG::RamStream containing the character's camera parameters
/// @details Panics if the character's weight class is invalid and asserts that the
/// `kartCameraParam.bin` file is loaded.
/**
 * @note For each weight class, there are 4 sets of camera parameters, as follows:\n
 *
 * Index | Players    | Aspect Ratio |
 * ------|------------|--------------|
 * 0x0   | 1, 3, or 4 | 4:3          |
 * 0x1   | 1, 3, or 4 | 16:9         |
 * 0x2   | 2          | 4:3          |
 * 0x3   | 2          | 16:9         |
 * \n Because Kinoko currently only supports single player replays, we can ignore indices `0x2` and
 *`0x3`. In the base game, it is possible to create a ghost such that its playback only synchronizes
 *when the console is set to either 4:3 or 16:9, such that it desyncs if you change the aspect ratio
 *to 16:9 or 4:3 respectively. Because this is an incredibly niche scenario, we instead hard-code
 * Kinoko to always use the 16:9 camera parameters at index `0x1`.
 **/
EGG::RamStream KartParamFileManager::getKartCameraStream(Character character) const {
    WeightClass weightClass = CharacterToWeight(character);
    if (weightClass == WeightClass::Invalid) {
        PANIC("Invalid weight class when getting KartCamera stream");
    }

    auto *file = reinterpret_cast<const KartParam::KartCameraParam *>(m_kartCameraParam.data());
    ASSERT(file);

    // We skip 1 to get 16:9
    return EGG::RamStream(file + static_cast<u32>(weightClass) * 4 + 1,
            sizeof(KartParam::KartCameraParam));
}

/// @brief Private constructor that loads and validates the kart parameter files
KartParamFileManager::KartParamFileManager() {
    init();
}

/// @brief Private virtual destructor
KartParamFileManager::~KartParamFileManager() {
    if (s_instance) {
        s_instance = nullptr;
        WARN("KartParamFileManager instance not explicitly handled!");
    }
}

/// @brief Performs checks to make sure the files were loaded successfully
/// @return True if all files were loaded and validated successfully, false otherwise
/// @details Verifies that `kartParam.bin`, `driverParam.bin`, `bikePartsDispParam.bin`, and
/// `kartPartsDispParam.bin` are all loaded and that their sizes match the expected sizes based on
/// the number of entries in each file.
bool KartParamFileManager::validate() const {
    // Validate kartParam.bin
    if (m_kartParam.empty()) {
        return false;
    }

    auto *kartFile = reinterpret_cast<const ParamFile<KartParam::Stats> *>(m_kartParam.data());
    if (m_kartParam.size() !=
            parse<u32>(kartFile->count) * sizeof(KartParam::Stats) +
                    sizeof(decltype(kartFile->count))) {
        return false;
    }

    // Validate driverParam.bin
    if (m_driverParam.empty()) {
        return false;
    }

    auto *driverFile = reinterpret_cast<const ParamFile<KartParam::Stats> *>(m_driverParam.data());
    if (m_driverParam.size() !=
            parse<u32>(driverFile->count) * sizeof(KartParam::Stats) +
                    sizeof(decltype(driverFile->count))) {
        return false;
    }

    // Validate bikePartsDispParam.bin
    if (m_bikeDispParam.empty()) {
        return false;
    }

    auto *bikeDispFile =
            reinterpret_cast<const ParamFile<KartParam::BikeDisp> *>(m_bikeDispParam.data());
    if (m_bikeDispParam.size() !=
            parse<u32>(bikeDispFile->count) * sizeof(KartParam::BikeDisp) +
                    sizeof(decltype(bikeDispFile->count))) {
        return false;
    }

    // Validate kartPartsDispParam.bin
    if (m_kartDispParam.empty()) {
        return false;
    }

    auto *kartDispFile =
            reinterpret_cast<const ParamFile<KartParam::KartDisp> *>(m_kartDispParam.data());
    if (m_kartDispParam.size() !=
            parse<u32>(kartDispFile->count) * sizeof(KartParam::KartDisp) +
                    sizeof(decltype(kartDispFile->count))) {
        return false;
    }

    // Validate kartCameraParam.bin
    if (m_kartCameraParam.empty()) {
        return false;
    }

    return true;
}

KartParamFileManager *KartParamFileManager::s_instance = nullptr;

} // namespace Kinoko::Kart
