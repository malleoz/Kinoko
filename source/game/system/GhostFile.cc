#include "GhostFile.hh"

#include "game/system/KPadDirector.hh"
#include "game/system/RaceConfig.hh"
#include "game/system/RaceManager.hh"

#include <egg/core/Decomp.hh>

#include <abstract/CRC32.hh>

#include <chrono>
#include <cstring>

namespace System {

/// @addr{0x8051C398}
GhostFile::GhostFile(const RawGhostFile &raw) {
    u8 *streamPtr = const_cast<u8 *>(raw.buffer());
    EGG::RamStream stream(streamPtr, RKG_HEADER_SIZE);
    reset();
    read(stream);
    m_inputs = raw.buffer() + RKG_HEADER_SIZE;
}

/// @addr{0x8051CB1C}
/// @brief Constructs a GhostFile object from KPadPlayer::m_toFileBuffer
/// @details In the base game, this is called when the splits screen appears after a time trial. In
/// Kinoko, since we do not implement these pages, we instead expect the end-user to call this when
/// they wish to save their inputs.
GhostFile::GhostFile(u32 /*playerIdx*/) {
    constexpr u32 LAP_COUNT = 3;

    m_lapCount = LAP_COUNT;
    m_lapTimes = {};
    auto player = RaceManager::Instance()->player();

    for (u32 i = 0; i < LAP_COUNT; ++i) {
        m_lapTimes[i] = player.getLapSplit(i + 1);
    }

    m_raceTime = player.raceTimer();

    const auto &scenario = RaceConfig::Instance()->raceScenario();
    const auto &scenarioPlayer = scenario.players[0];
    m_character = scenarioPlayer.character;
    m_vehicle = scenarioPlayer.vehicle;
    m_course = scenario.course;

    const auto &playerInput = KPadDirector::Instance()->playerInput();
    const auto *controller = playerInput.controller();

    m_controllerId =
            static_cast<u32>(controller ? controller->controlSource() : ControlSource::Unknown);

    m_driftIsAuto = controller ? controller->driftIsAuto() : false;
    m_type = 1;

    auto now =
            std::chrono::zoned_time{std::chrono::current_zone(), std::chrono::system_clock::now()};
    auto today = std::chrono::floor<std::chrono::days>(now.get_local_time());

    std::chrono::year_month_day ymd{today};
    u32 year = static_cast<u32>(int(ymd.year()));
    u8 month = static_cast<u8>(unsigned(ymd.month()));
    u8 day = static_cast<u8>(unsigned(ymd.day()));

    if (year < 2000) {
        year = 2000;
    } else {
        year -= 2000;
    }

    m_year = static_cast<u8>(std::min<u32>(99, year));
    m_month = static_cast<u8>(month);
    m_day = static_cast<u8>(day);

    m_location = 0;
    m_inputs = playerInput.toFileBuffer();
    m_inputSize = static_cast<u16>(playerInput.toFileBufferSize());
}

/// @addr{0x8051CF90}
GhostFile::~GhostFile() = default;

/// @addr{0x8051C4A0}
void GhostFile::reset() {
    m_userData = {};
    m_miiData = {};
    m_lapCount = 0;
    m_lapTimes[0].valid = false;
    m_lapTimes[1].valid = false;
    m_lapTimes[2].valid = false;
    m_lapTimes[3].valid = false;
    m_lapTimes[4].valid = false;
    m_raceTime.valid = false;
    m_character = Character::Mario;
    m_vehicle = Vehicle::Standard_Kart_S;
    m_course = Course::Mario_Circuit;
    m_inputSize = 0;
    m_inputs = nullptr;
    m_driftIsAuto = false;
}

/// @addr{0x8051C530}
void GhostFile::read(EGG::RamStream &stream) {
    constexpr size_t RKG_MII_DATA_SIZE = 0x4A;
    constexpr size_t RKG_USER_DATA_SIZE = 0x14;

    stream.skip(0x4); // RKGD

    // 0x04 - 0x07
    u32 data = stream.read_u32();
    m_raceTime = Timer(data);
    m_course = static_cast<Course>(data >> 0x2 & 0x3F);

    // 0x08 - 0x0B
    data = stream.read_u32();
    m_vehicle = static_cast<Vehicle>(data >> 0x1A);
    m_character = static_cast<Character>(data >> 0x14 & 0x3F);
    m_year = (data >> 0xD) & 0x7F;
    m_month = (data >> 0x9) & 0xF;
    m_day = (data >> 0x4) & 0x1F;

    // 0x0C - 0x0F
    data = stream.read_u32();
    m_type = data >> 0xA & 0x7F;
    m_driftIsAuto = data >> 0x11 & 0x1;
    m_inputSize = data & 0xFFFF;

    // 0x10
    m_lapCount = stream.read_u8();

    // 0x11 - 0x1F
    for (size_t i = 0; i < 5; ++i) {
        m_lapTimes[i] = Timer(stream.read_u32());
        stream.jump(stream.index() - 1);
    }

    stream.read(m_userData.data(), RKG_USER_DATA_SIZE);
    m_userData[10] = L'\0';

    // 0x34
    m_location = stream.read_u8();
    stream.skip(0x7);
    stream.read(m_miiData.data(), RKG_MII_DATA_SIZE);
}

/// @addr{0x8051CA80}
[[nodiscard]] RawGhostFile GhostFile::writeUncompressed() {
    RawGhostFile outFile;

    if (!m_inputs) {
        return outFile;
    }

    writeHeader(outFile);
    memcpy(outFile.buffer() + 0xE, &m_inputSize, sizeof(u16));
    memcpy(outFile.buffer() + 0x88, m_inputs, RKG_UNCOMPRESSED_INPUT_DATA_SECTION_SIZE);
    u32 checksum = parse<u32>(Abstract::CalcCRC32(outFile.buffer(),
            RKG_HEADER_SIZE + RKG_UNCOMPRESSED_INPUT_DATA_SECTION_SIZE));
    memcpy(outFile.buffer() + RKG_HEADER_SIZE + RKG_UNCOMPRESSED_INPUT_DATA_SECTION_SIZE, &checksum,
            sizeof(u32));

    return outFile;
}

/// @addr{0x8051C7F4}
void GhostFile::writeHeader(RawGhostFile &outFile) const {
    memcpy(outFile.buffer() + 0x20, m_userData.data(), 0x14);

    u32 *byte2 = reinterpret_cast<u32 *>(outFile.buffer() + 8);

    *byte2 = ((static_cast<u32>(m_year) << 0xD) & 0xFE000) | (*byte2 & 0xFFF01FFF);
    *byte2 = ((static_cast<u32>(m_month) << 0x9) & 0x1E00) | (*byte2 & 0xFFFFE1FF);
    *byte2 = ((static_cast<u32>(m_day) << 0x4) & 0x1F0) | (*byte2 & 0xFFFFFE0F);

    memcpy(outFile.buffer() + 0x3C, m_miiData.data(), m_miiData.size());

    *(outFile.buffer() + 0x10) = m_lapCount;

    for (u8 i = 0; i < m_lapCount; ++i) {
        u32 min = std::min<u32>(99, m_lapTimes[i].min);
        u32 sec = m_lapTimes[i].min > 100 ? 59 : m_lapTimes[i].sec;
        u32 mil = m_lapTimes[i].min > 100 ? 999 : m_lapTimes[i].mil;

        *(outFile.buffer() + 3 * i + 0x11) =
                static_cast<u8>((((min << 0x11) & 0xFE0000) | ((sec << 10) & 0x1FC00)) >> 0x10);
        *(outFile.buffer() + 3 * i + 0x12) =
                static_cast<u8>((((sec << 10) & 0x1FC00) | (mil & 0x3FF)) >> 0x8);
        *(outFile.buffer() + 3 * i + 0x13) = mil & 0xFF;
    }

    u32 min = std::min<u32>(99, m_raceTime.min);
    u32 sec = m_raceTime.min > 100 ? 59 : m_raceTime.sec;
    u32 mil = m_raceTime.min > 100 ? 999 : m_raceTime.mil;

    u32 *byte1 = reinterpret_cast<u32 *>(outFile.buffer() + 4);
    *byte1 = ((min << 25) & 0xFE000000) | (*byte1 & 0x1FFFFFF);
    *byte1 = ((sec << 18) & 0x1FC0000) | (*byte1 & 0xFE03FFFF);
    *byte1 = ((mil << 8) & 0x3FF00) | (*byte1 & 0xFFFC00FF);
    *byte1 = ((static_cast<u32>(m_course) << 2) & 0x3F) | (*byte1 & 0xFFFFFF03);

    // Correct endianness
    *byte1 = parse<u32>(*byte1);

    *byte2 = ((static_cast<u32>(m_character) & 0x3F) << 20) | (*byte2 & 0xFC0FFFFF);
    *byte2 = ((static_cast<u32>(m_vehicle) << 26) & 0xFC000000) | (*byte2 & 0x3FFFFFF);
    *byte2 = (m_controllerId & 0xF) | (*byte2 & 0xFFFFFFF0);

    // Correct endianness
    *byte2 = parse<u32>(*byte2);

    u16 *byte3 = reinterpret_cast<u16 *>(outFile.buffer() + 0xC);
    *byte3 = ((static_cast<u16>(m_driftIsAuto) << 1) & 0x2) | (*byte3 & 0xFFFD);
    *byte3 = ((m_type << 2) & 0x1FC) | (*byte3 & 0xFE03);

    // Correct endianness
    *byte3 = parse<u16>(*byte3);

    u32 *location = reinterpret_cast<u32 *>(outFile.buffer() + 0x34);
    *location = parse<u32>(m_location);

    u32 rkgd = parse<u32>(0x524B4744);
    memcpy(outFile.buffer(), &rkgd, sizeof(u32));
}

RawGhostFile::RawGhostFile() {
    memset(m_buffer, 0, sizeof(m_buffer));
}

RawGhostFile::RawGhostFile(const u8 *rkg) {
    init(rkg);
}

RawGhostFile::~RawGhostFile() = default;

RawGhostFile &RawGhostFile::operator=(const u8 *rkg) {
    init(rkg);

    return *this;
}

void RawGhostFile::init(const u8 *rkg) {
    if (!isValid(rkg)) {
        PANIC("Invalid RKG header");
    }

    if (compressed(rkg)) {
        if (!decompress(rkg)) {
            PANIC("Failed to decompress RKG!");
        }
    } else {
        memcpy(m_buffer, rkg, sizeof(m_buffer));
    }
}

/// @addr{0x8051D1B4}
bool RawGhostFile::decompress(const u8 *rkg) {
    memcpy(m_buffer, rkg, RKG_HEADER_SIZE);

    // Unset compressed flag
    *(m_buffer + 0xC) &= 0xF7;

    // Get uncompressed size. Skip past 0x4 bytes which represents the size of the compressed data
    s32 uncompressedSize = EGG::Decomp::GetExpandSize(rkg + RKG_HEADER_SIZE + 0x4);

    if (uncompressedSize <= 0 ||
            static_cast<u32>(uncompressedSize) > RKG_UNCOMPRESSED_INPUT_DATA_SECTION_SIZE) {
        return false;
    }

    EGG::Decomp::DecodeSZS(rkg + RKG_HEADER_SIZE + 0x4, m_buffer + RKG_HEADER_SIZE);

    // Set input data section length
    *reinterpret_cast<u16 *>(m_buffer + 0xE) = static_cast<u16>(uncompressedSize);

    return true;
}

/// @addr{0x8051C120}
/// @todo Check for valid controller type?
/// @todo Check lap times sum to race time?
bool RawGhostFile::isValid(const u8 *rkg) const {
    if (strncmp(reinterpret_cast<const char *>(rkg), "RKGD", 4) != 0) {
        PANIC("RKG header malformed");
        return false;
    }

    u32 ids = parse<u32>(*reinterpret_cast<const u32 *>(rkg + 0x8));
    Vehicle vehicle = static_cast<Vehicle>(ids >> 0x1a);
    Character character = static_cast<Character>((ids >> 0x14) & 0x3f);
    u8 year = (ids >> 0xd) & 0x7f;
    u8 day = (ids >> 0x4) & 0x1f;
    u8 month = (ids >> 0x9) & 0xf;

    if (vehicle >= Vehicle::Max || character >= Character::Max) {
        return false;
    }

    if (year >= 100 || day >= 32 || month > 12) {
        return false;
    }

    // Validate weight class match
    WeightClass charWeight = CharacterToWeight(character);
    WeightClass vehicleWeight = VehicleToWeight(vehicle);

    if (charWeight == WeightClass::Invalid) {
        PANIC("Invalid character weight class!");
    }
    if (vehicleWeight == WeightClass::Invalid) {
        PANIC("Invalid vehicle weight class!");
    }
    if (charWeight != vehicleWeight) {
        PANIC("Character/Bike weight class mismatch!");
    }

    return true;
}

} // namespace System
