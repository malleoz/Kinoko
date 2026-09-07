#pragma once

#include "game/system/map/MapdataAccessorBase.hh"

#include <egg/util/Stream.hh>

namespace Kinoko::System {

/// @brief Describes stage information of a course
class MapdataStageInfo {
public:
    /// @brief Represents the raw data structure of stage information
    /// @details Pole position refers to the position of the player who starts the race in first.
    struct SData {
        u8 _0[0x1 - 0x0];   ///< Unused in Kinoko
        u8 polePosition;    ///< Pole position side (0 = left, 1 = right)
        u8 translationMode; ///< Specifies whether players start closer together
        u8 _3[0xc - 0x3];   ///< Unused in Kinoko
    };
    STATIC_ASSERT(sizeof(SData) == 0xc);

    /// @brief Constructor
    /// @param data Pointer to the raw stage information data
    MapdataStageInfo(const SData *data) : m_rawData(data) {
        EGG::RamStream stream = EGG::RamStream(data, sizeof(SData));
        read(stream);
    }

    /// @brief Reads the stage information data from the given stream
    /// @details We skip implementing this function and opt to instead pull data directly from the
    /// raw structure since the single byte fields we fetch are not susceptible to endianness
    /// issues.
    void read(EGG::Stream & /*stream*/) {}

    /// @beginGetters
    [[nodiscard]] u8 polePosition() const {
        return m_rawData->polePosition;
    }

    [[nodiscard]] u8 translationMode() const {
        return m_rawData->translationMode;
    }
    /// @endGetters

private:
    const SData *const m_rawData; ///< Pointer to the raw stage information data
};

/// @brief Provides access to entries in the STGI section of the course KMP
class MapdataStageInfoAccessor
    : public MapdataAccessorBase<MapdataStageInfo, MapdataStageInfo::SData> {
public:
    /// @brief Constructor
    /// @param header Pointer to the section header of the STGI section in the course KMP
    MapdataStageInfoAccessor(const MapSectionHeader *header)
        : MapdataAccessorBase<MapdataStageInfo, MapdataStageInfo::SData>(header) {
        init(reinterpret_cast<const MapdataStageInfo::SData *>(m_sectionHeader + 1),
                parse<u16>(m_sectionHeader->count));
    }

    /// @brief Default virtual destructor
    ~MapdataStageInfoAccessor() override = default;
};

} // namespace Kinoko::System
