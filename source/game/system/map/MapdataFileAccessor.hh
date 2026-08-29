#pragma once

#include "game/system/map/MapdataAccessorBase.hh"

namespace Kinoko::System {

class MapdataFileAccessor {
public:
    struct SData {
        u32 magic;
        u32 fileSize;
        u16 sectionCount;
        u16 headerSize;
        u32 revision;
        s32 offsets[];
    };

    /// @addr{0x80512C2C}
    MapdataFileAccessor(const MapdataFileAccessor::SData *data) : m_rawData(data) {
        u32 offset = parse<u16>(data->headerSize) - parse<u16>(data->sectionCount) * 4;
        m_sectionDefOffset = offset;
        m_sectionDef =
                reinterpret_cast<const u32 *>(reinterpret_cast<const u8 *>(m_rawData) + offset);
        m_version = offset > 12 ? parse<u32>(m_sectionDef[-1]) : 0;
    }

    /// @addr{0x80514208}
    [[nodiscard]] const MapSectionHeader *findSection(u32 signature) const {
        const MapSectionHeader *sectionPtr = nullptr;

        for (size_t i = 0; i < parse<u16>(m_rawData->sectionCount); ++i) {
            const MapSectionHeader *header = reinterpret_cast<const MapSectionHeader *>(
                    reinterpret_cast<const u8 *>(m_rawData) + parse<u16>(m_rawData->headerSize) +
                    parse<u32>(m_sectionDef[i]));
            if (parse<u32>(header->magic) == signature) {
                sectionPtr = header;
                break;
            }
        }

        return sectionPtr;
    }

    [[nodiscard]] u32 version() const {
        return m_version;
    }

private:
    const SData *m_rawData;
    const u32 *m_sectionDef;
    u32 m_version;
    u32 m_sectionDefOffset;
};

} // namespace Kinoko::System
