#pragma once

#include "game/system/TimerManager.hh"

#include <egg/util/Stream.hh>

#include <cstring>

namespace Kinoko::System {

/// @brief The size of the `.rkg` file header
static constexpr size_t RKG_HEADER_SIZE = 0x88;

/// @brief The size of the uncompressed input data section in a `.rkg` file
static constexpr size_t RKG_UNCOMPRESSED_INPUT_DATA_SECTION_SIZE = 0x2774;

/// @brief The size of the CRC-32 checksum in a `.rkg` file
static constexpr size_t CRC_32_SIZE = 0x4;

/// @brief The maximum possible size of a `.rkg` file
static constexpr size_t RKG_MAX_FILE_SIZE =
        RKG_HEADER_SIZE + RKG_UNCOMPRESSED_INPUT_DATA_SECTION_SIZE + CRC_32_SIZE;

/**
 * @brief The binary data of a ghost saved to a file
 * @details The file format of a `.rkg` file is as follows:
 *
 * Offset | Size       | Description                                                               |
 * ------ | ---------- | ------------------------------------------------------------------------- |
 * 0x00   | 4 bytes    | **RKGD** in ASCII.                                                        |
 * 0x04   | 7 bits     | **Minutes** field of finishing time                                       |
 * 0x04.7 | 7 bits     | **Seconds** field of finishing time                                       |
 * 0x05.6 | 10 bits    | **Milliseconds** field of finishing time                                  |
 * 0x07   | 6 bits     | **Track ID**                                                              |
 * 0x07.6 | 2 bits     | **Unknown.** Probably padding.                                            |
 * 0x08   | 6 bits     | **Vehicle ID**                                                            |
 * 0x08.6 | 6 bits     | **Character ID**                                                          |
 * 0x09.4 | 7 bits     | **Year** relative to 2000                                                 |
 * 0x0A.3 | 4 bits     | **Month**                                                                 |
 * 0x0A.7 | 5 bits     | **Day**                                                                   |
 * 0x0B.4 | 4 bits     | **Controller ID**                                                         |
 * 0x0C   | 4 bits     | **Unknown.** Always 0?                                                    |
 * 0x0C.4 | 1 bit      | **Compressed** flag (1 for compressed, 0 for raw)                         |
 * 0x0C.5 | 2 bits     | **Unknown.** Always 0?                                                    |
 * 0x0C.7 | 7 bits     | **Ghost type**                                                            |
 * 0x0D.6 | 1 bit      | **Drift type** (1 for auto, 0 for manual)                                 |
 * 0x0E   | 2 bytes    | **Input data length** (measured after decompression)                      |
 * 0x10   | 1 byte     | **Lap count**                                                             |
 * 0x11   | 5x3 bytes  | Lap **split** times. Stored in the same 7-7-10 format as the finish time. |
 * 0x20   | 0x14 bytes | **Unknown**                                                               |
 * 0x34   | 1 byte     | **Country code** or 0xFF if sharing location disabled.                    |
 * 0x35   | 1 byte     | **Status code** or 0xFF if sharing location disabled.                     |
 * 0x36   | 2 bytes    | **Location code** or 0xFFFF if sharing location disabled.                 |
 * 0x38   | 4 bytes    | **Unknown**                                                               |
 * 0x3C   | 0x4A bytes | **Mii data**                                                              |
 * 0x86   | 2 bytes    | **CRC-16-CCITT-XModem** of Mii data                                       |
 *
 * @see https://wiki.tockdom.com/wiki/RKG_(File_Format)
 **/

class RawGhostFile {
public:
    /// @brief Constructor that initializes the raw ghost file buffer to zero
    RawGhostFile() {
        memset(m_buffer, 0, sizeof(m_buffer));
    }

    /// @brief Constructor that initializes the raw ghost file buffer with the provided binary data
    /// @param rkg Pointer to the binary data from a ghost file
    RawGhostFile(const u8 *rkg) {
        init(rkg);
    }

    /// @brief Default destructor
    ~RawGhostFile() = default;

    /// @brief Copy assignment operator
    /// @param rkg Pointer to the binary data from a ghost file
    /// @return Reference to the current `RawGhostFile` instance
    /// @details First clears the buffer to avoid stale data before initializing it with the new
    /// binary data.
    RawGhostFile &operator=(const u8 *rkg) {
        memset(m_buffer, 0, sizeof(m_buffer));
        init(rkg);
        return *this;
    }

    void init(const u8 *rkg);
    [[nodiscard]] bool decompress(const u8 *rkg);
    [[nodiscard]] bool isValid(const u8 *rkg) const;

    /// @beginGetters

    /// @brief Gets the buffer containing the binary data of the ghost file
    /// @return Pointer to the buffer containing the binary data of the ghost file
    [[nodiscard]] const u8 *buffer() const {
        return m_buffer;
    }

    /// @endGetters

    /// @brief Parses a value of type `T` from the raw ghost file buffer at the specified offset,
    /// keeping in mind endianness byte-swapping
    /// @tparam T The type of data to parse from the raw ghost file buffer
    /// @param offset The offset from the start of the buffer to read the value of type `T`
    /// @return The value of type `T` parsed from the raw ghost file buffer at the specified offset
    template <ParseableType T>
    [[nodiscard]] T parseAt(size_t offset) const {
        return parse<T>(*reinterpret_cast<const T *>(m_buffer + offset));
    }

private:
    /// @brief Gets the "compressed" flag bit from the provided `.rkg` file pointer
    /// @param rkg The `.rkg` file pointer to check for the compressed flag
    /// @return `true` if the "compressed" flag is set, `false` otherwise
    [[nodiscard]] bool compressed(const u8 *rkg) const {
        return ((*(rkg + 0xC) >> 3) & 1) == 1;
    }

    u8 m_buffer[RKG_MAX_FILE_SIZE]; ///< Buffer containing the binary data of the ghost file
};
STATIC_ASSERT(sizeof(RawGhostFile) == RKG_MAX_FILE_SIZE);

/// @brief Parsed representation of a ghost `.rkg` file
/// @see RawGhostFile
class GhostFile {
public:
    /// @addr{0x8051C398}
    /// @brief Constructor
    /// @param raw The raw ghost file to parse and initialize the `GhostFile` instance from
    /// @details Creates a @ref EGG::RamStream to read the header section of the raw ghost file.
    GhostFile(const RawGhostFile &raw) : m_inputs(raw.buffer() + RKG_HEADER_SIZE) {
        u8 *streamPtr = const_cast<u8 *>(raw.buffer());
        EGG::RamStream stream(streamPtr, RKG_HEADER_SIZE);
        read(stream);
    }

    /// @addr{0x8051CF90}
    /// @brief Default destructor
    ~GhostFile() = default;

    void read(EGG::RamStream &stream);

    /// @beginGetters

    /// @brief Gets the lap timer at the provided index
    /// @param i The index of the lap to get the timer for
    /// @return Const ref to the lap @ref Timer for the specified lap index from @ref m_lapTimes.
    /// @pre Requires that `i` is less than the number of laps in the race
    [[nodiscard]] const Timer &lapTimer(size_t i) const {
        ASSERT(i < m_lapTimes.size());
        return m_lapTimes[i];
    }

    /// @brief Gets the total race time
    /// @return Const ref to the total race @ref Timer, @ref m_raceTime.
    [[nodiscard]] const Timer &raceTimer() const {
        return m_raceTime;
    }

    /// @brief Gets the character used in the race
    /// @return The @ref Character used in the race, @ref m_character
    [[nodiscard]] Character character() const {
        return m_character;
    }

    /// @brief Gets the vehicle used in the race
    /// @return The @ref Vehicle used in the race, @ref m_vehicle
    [[nodiscard]] Vehicle vehicle() const {
        return m_vehicle;
    }

    /// @brief Gets the course that the race is taking place on
    /// @return The @ref Course of the race, @ref m_course
    [[nodiscard]] Course course() const {
        return m_course;
    }

    /// @brief Gets the pointer to the ghost's input data section
    /// @return Const pointer to the input data section, @ref m_inputs
    [[nodiscard]] const u8 *inputs() const {
        return m_inputs;
    }

    /// @brief Checks if the drift mode is automatic
    /// @return The value of @ref m_driftIsAuto which is `true` when using automatic and `false`
    /// when using manual
    [[nodiscard]] bool driftIsAuto() const {
        return m_driftIsAuto;
    }

    /// @endGetters

private:
    std::array<wchar_t, 11> m_userData;  ///< Unused
    std::array<u8, 76> m_miiData;        ///< See https://wiibrew.org/wiki/Mii_data#Mii_format
    u8 m_lapCount;                       ///< The number of laps in the race (always 3)
    std::array<Timer, 5> m_lapTimes;     ///< Array of @ref Timer objects for each lap
    Timer m_raceTime;                    ///< The total race time
    Character m_character;               ///< The character used in the race
    Vehicle m_vehicle;                   ///< The vehicle used in the race
    Course m_course;                     ///< The course of the race
    [[maybe_unused]] u32 m_controllerId; ///< The ID of the controller used
    u8 m_year;                           ///< The year, relative to 2000
    u8 m_month;                          ///< The month [1-12]
    u8 m_day;                            ///< The day of the month [1-31]
    u32 m_type;                          ///< The type of ghost
    bool m_driftIsAuto;                  ///< True for automatic, false for manual
    u32 m_location;                      ///< 0xFFFF if sharing disabled
    u16 m_inputSize;                     ///< The size of the decompressed input data section
    const u8 *const m_inputs;            ///< Pointer to the ghost's input data section
};

} // namespace Kinoko::System
