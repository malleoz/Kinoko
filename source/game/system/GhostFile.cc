#include "GhostFile.hh"

#include <egg/core/Decomp.hh>

#include <cstring>

namespace Kinoko::System {

/// @brief Constructor that initializes the raw ghost file buffer to zero
RawGhostFile::RawGhostFile() {
    memset(m_buffer, 0, sizeof(m_buffer));
}

/// @brief Constructor that initializes the raw ghost file buffer with the provided binary data
/// @param rkg Pointer to the binary data from a ghost file
RawGhostFile::RawGhostFile(const u8 *rkg) {
    init(rkg);
}

/// @brief Default destructor
RawGhostFile::~RawGhostFile() = default;

/// @brief Validates the provided raw ghost file buffer, decompresses it if necessary, and copies it
/// into the buffer
/// @param rkg Pointer to the binary data from a ghost file
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
/// @brief Decompresses the input data section of the raw ghost file if it is compressed
/// @param rkg Pointer to the binary data from a ghost file
/// @return `true` if decompression was successful, `false` otherwise
/// @pre The raw ghost file buffer must contain a YAZ1 compressed input data section
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
/// @brief Checks if the provided raw ghost file buffer contains a valid RKG header
/// @param rkg Pointer to the binary data from a ghost file
/// @return `true` if the raw ghost file buffer is valid, `false` otherwise
/// @details Checks that the file magic is present in the first 4 bytes (`RKGD`). Checks that the
/// vehicle and character combo is valid. Requires that the ghost was set within 100 years after
/// 2000 and that the day and month are within the expected value range.
/// @warning This function shows that after the year 2100, any times set will be considered
/// inauthentic and will not appear in the time trial ghost menu.
/// @todo Check for valid controller type?
/// @todo Check lap times sum to race time?
/// @todo Check that month is not 0? The base game does not fail here
/// @todo Check that day is not 0?
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

/// @addr{0x8051C398}
/// @brief Constructor
/// @param raw The raw ghost file to parse and initialize the `GhostFile` instance from
GhostFile::GhostFile(const RawGhostFile &raw) : m_inputs(raw.buffer() + RKG_HEADER_SIZE) {
    u8 *streamPtr = const_cast<u8 *>(raw.buffer());
    EGG::RamStream stream(streamPtr, RKG_HEADER_SIZE);
    read(stream);
}

/// @addr{0x8051CF90}
/// @brief Default destructor
GhostFile::~GhostFile() = default;

/// @addr{0x8051C530}
/// @brief Reads and parses the ghost file data from the provided @ref EGG::RamStream
/// @param stream The stream containing the raw ghost file data
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

} // namespace Kinoko::System
