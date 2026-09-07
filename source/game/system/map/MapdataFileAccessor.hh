#pragma once

#include "game/system/map/MapdataAccessorBase.hh"

namespace Kinoko::System {

/// @brief Describes the header of a course KMP file
/// @details Members are re-ordered from the base game for const-correctness.
class MapdataFileAccessor {
public:
    /// @brief Represents the raw data structure of a course KMP file header
    struct SData {
        u32 sectionName;  ///< Always RKMD in ASCII
        u32 fileSize;     ///< Length of the file in bytes
        u16 sectionCount; ///< Number of sections in the KMP file (15)
        u16 headerSize;   ///< The size of this header (usually 0x4C)
        u32 revision;     ///< Version of the KMP file
        s32 offsets[];    ///< Start of the offsets for each section of the KMP file
    };
    STATIC_ASSERT(sizeof(SData) == 0x10); // Offset array size is not known at compile-time

    /// @addr{0x80512C2C}
    /// @brief Constructor
    /// @param data Pointer to the raw KMP file header data
    MapdataFileAccessor(const MapdataFileAccessor::SData *data)
        : m_rawData(data),
          m_sectionDefOffset(parse<u16>(data->headerSize) - parse<u16>(data->sectionCount) * 4),
          m_sectionDef(reinterpret_cast<const u32 *>(
                  reinterpret_cast<const u8 *>(m_rawData) + m_sectionDefOffset)),
          m_version(m_sectionDefOffset > 12 ? parse<u32>(m_sectionDef[-1]) : 0) {}

    /// @addr{0x80514208}
    /// @brief Finds a section in the KMP file by its signature
    /// @param signature The signature of the section to find
    /// @return Pointer to the section header if found, nullptr otherwise
    [[nodiscard]] const MapSectionHeader *findSection(u32 signature) const {
        const MapSectionHeader *sectionPtr = nullptr;

        for (size_t i = 0; i < parse<u16>(m_rawData->sectionCount); ++i) {
            const MapSectionHeader *header = reinterpret_cast<const MapSectionHeader *>(
                    reinterpret_cast<const u8 *>(m_rawData) + parse<u16>(m_rawData->headerSize) +
                    parse<u32>(m_sectionDef[i]));
            if (parse<u32>(header->sectionName) == signature) {
                sectionPtr = header;
                break;
            }
        }

        return sectionPtr;
    }

    /// @beginGetters
    [[nodiscard]] u32 version() const {
        return m_version;
    }
    /// @endGetters

private:
    const SData *const m_rawData;  ///< Pointer to the raw KMP file header data
    const u32 m_sectionDefOffset;  ///< Offset to the section definition array
    const u32 *const m_sectionDef; ///< Pointer to the section definition array
    const u32 m_version;           ///< Version of the KMP file
};

} // namespace Kinoko::System
