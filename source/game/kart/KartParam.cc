#include "KartParam.hh"

#include "game/kart/KartParamFileManager.hh"

namespace Kinoko::Kart {

/// @addr{Inlined in 0x0x8058F5B4}
/// @brief Constructor which initializes all stats and hitboxes based on the provided
/// character/vehicle combo
/// @param character The character to initialize stats for
/// @param vehicle The vehicle to initialize stats for
/// @param playerIdx The player index of the given vehicle (always 0 in Kinoko)
KartParam::KartParam(Character character, Vehicle vehicle, u8 playerIdx) {
    initStats(character, vehicle);
    initHitboxes(vehicle);
    m_playerIdx = playerIdx;
    m_isBike = vehicle >= Vehicle::Standard_Bike_S;
    if (m_isBike) {
        initBikeDispParams(vehicle);
    } else {
        initKartDispParams(vehicle);
    }

    initCameraParams(character);
}

/// @brief Default destructor
KartParam::~KartParam() = default;

/// @addr{0x80591FA4}
/// @brief Parses out the character/vehicle combo stats
/// @param character The character to initialize stats for
/// @param vehicle The vehicle to initialize stats for
void KartParam::initStats(Character character, Vehicle vehicle) {
    auto *fileManager = KartParamFileManager::Instance();

    auto vehicleStream = fileManager->getVehicleStream(vehicle);
    auto driverStream = fileManager->getDriverStream(character);

    m_stats = Stats(vehicleStream);
    m_stats.applyCharacterBonus(driverStream);
}

/// @addr{0x80592594}
/// @brief Initializes the display parameters for a given bike from bikePartsDispParam.bin
/// @param vehicle The bike to initialize display parameters for
void KartParam::initBikeDispParams(Vehicle vehicle) {
    auto *fileManager = KartParamFileManager::Instance();

    auto dispParamsStream = fileManager->getBikeDispParamsStream(vehicle);
    m_bikeDisp = BikeDisp(dispParamsStream);
}

/// @addr{0x805924CC}
/// @brief Initializes the display parameters for a given kart from kartPartsDispParam.bin
/// @param vehicle The kart to initialize display parameters for
void KartParam::initKartDispParams(Vehicle vehicle) {
    auto *fileManager = KartParamFileManager::Instance();

    auto dispParamsStream = fileManager->getKartDispParamsStream(vehicle);
    m_kartDisp = KartDisp(dispParamsStream);
}

/// @addr{Inlined in 0x0x8058F5B4}
/// @brief Initializes the hitboxes and wheels for a given vehicle from kartParam.bin
/// @param vehicle The vehicle to initialize hitboxes for
void KartParam::initHitboxes(Vehicle vehicle) {
    auto *fileManager = KartParamFileManager::Instance();

    auto hitboxStream = fileManager->getHitboxStream(vehicle);
    m_bsp = BSP(hitboxStream);
}

/// @addr{0x80592718}
/// @brief Initializes the camera parameters for a given character from kartCameraParam.bin
/// @param character The character to initialize camera parameters for
void KartParam::initCameraParams(Character character) {
    auto *fileManager = KartParamFileManager::Instance();

    auto cameraStream = fileManager->getKartCameraStream(character);
    m_camera = KartCameraParam(cameraStream);
}

/// @brief Parses out the stats for a given KartParam.bin stream
/// @param stream A RamStream of data from KartParam.bin
void KartParam::Stats::read(EGG::RamStream &stream) {
    body = static_cast<Body>(stream.read_s32());
    driftType = static_cast<DriftType>(stream.read_s32());
    weightClass = static_cast<WeightClass>(stream.read_s32());
    _00c = stream.read_f32();
    weight = stream.read_f32();
    bumpDeviationLevel = stream.read_f32();
    speed = stream.read_f32();
    turningSpeed = stream.read_f32();
    tilt = stream.read_f32();
    accelerationStandardA[0] = stream.read_f32();
    accelerationStandardA[1] = stream.read_f32();
    accelerationStandardA[2] = stream.read_f32();
    accelerationStandardA[3] = stream.read_f32();
    accelerationStandardT[0] = stream.read_f32();
    accelerationStandardT[1] = stream.read_f32();
    accelerationStandardT[2] = stream.read_f32();
    accelerationDriftA[0] = stream.read_f32();
    accelerationDriftA[1] = stream.read_f32();
    accelerationDriftT[0] = stream.read_f32();
    handlingManualTightness = stream.read_f32();
    handlingAutomaticTightness = stream.read_f32();
    handlingReactivity = stream.read_f32();
    driftManualTightness = stream.read_f32();
    driftAutomaticTightness = stream.read_f32();
    driftReactivity = stream.read_f32();
    driftOutsideTargetAngle = stream.read_f32();
    driftOutsideDecrement = stream.read_f32();
    miniTurboDuration = stream.read_u32();

    for (size_t i = 0; i < kclSpeed.size(); ++i) {
        kclSpeed[i] = stream.read_f32();
    }
    for (size_t i = 0; i < kclRot.size(); ++i) {
        kclRot[i] = stream.read_f32();
    }

    _170 = stream.read_f32();
    _174 = stream.read_f32();
    _178 = stream.read_f32();
    _17c = stream.read_f32();

    maxNormalForce = stream.read_f32();
    megaScale = stream.read_f32();
    shrinkScale = stream.read_f32();
}

/// @brief Applies character stats on top of the kart stats
/// @param stream A RamStream of data from driverParam.bin
void KartParam::Stats::applyCharacterBonus(EGG::RamStream &stream) {
    stream.skip(0x10);
    weight += stream.read_f32();

    stream.skip(0x4);
    speed += stream.read_f32();
    turningSpeed += stream.read_f32();

    stream.skip(0x4);
    accelerationStandardA[0] += stream.read_f32();
    accelerationStandardA[1] += stream.read_f32();
    accelerationStandardA[2] += stream.read_f32();
    accelerationStandardA[3] += stream.read_f32();
    accelerationStandardA[0] += stream.read_f32();
    accelerationStandardA[1] += stream.read_f32();
    accelerationStandardA[2] += stream.read_f32();
    accelerationDriftA[0] += stream.read_f32();
    accelerationDriftA[1] += stream.read_f32();
    accelerationDriftT[0] += stream.read_f32();
    handlingManualTightness += stream.read_f32();
    handlingAutomaticTightness += stream.read_f32();
    handlingReactivity += stream.read_f32();
    driftManualTightness += stream.read_f32();
    driftAutomaticTightness += stream.read_f32();
    driftReactivity += stream.read_f32();
    driftOutsideTargetAngle += stream.read_f32();
    driftOutsideDecrement += stream.read_f32();
    miniTurboDuration += stream.read_u32();

    for (size_t i = 0; i < kclSpeed.size(); ++i) {
        kclSpeed[i] += stream.read_f32();
    }

    for (size_t i = 0; i < kclRot.size(); ++i) {
        kclRot[i] += stream.read_f32();
    }
}

/// @brief Uninitialized default constructor
BSP::BSP() = default;

/// @brief Constructor which parses out the hitboxes and wheels for a given KartParam.bin stream
/// @param stream A @ref EGG::RamStream of data from KartParam.bin
BSP::BSP(EGG::RamStream &stream) {
    read(stream);
}

/// @brief Parses out the hitboxes and wheels for a given KartParam.bin stream
/// @param stream A @ref EGG::RamStream of data from KartParam.bin
void BSP::read(EGG::RamStream &stream) {
    offsetY = stream.read_f32();

    for (auto &hitbox : hitboxes) {
        hitbox.enable = stream.read_u16();
        stream.skip(2);
        hitbox.position.read(stream);
        hitbox.radius = stream.read_f32();
        hitbox.wallsOnly = stream.read_u16();
        hitbox.tireIdx = stream.read_u16();
    }

    cuboids[0].read(stream);
    cuboids[1].read(stream);
    angVel0Factor = stream.read_f32();
    _1a0 = stream.read_f32();

    for (auto &wheel : wheels) {
        wheel.enable = stream.read_u16();
        stream.skip(2);
        wheel.springStiffness = stream.read_f32();
        wheel.dampingFactor = stream.read_f32();
        wheel.maxTravel = stream.read_f32();
        wheel.springTop.read(stream);
        wheel.xRot = stream.read_f32();
        wheel.wheelRadius = stream.read_f32();
        wheel.sphereRadius = stream.read_f32();
        wheel._28 = stream.read_u32();
    }

    rumbleHeight = stream.read_f32();
    rumbleSpeed = stream.read_f32();
}

} // namespace Kinoko::Kart
