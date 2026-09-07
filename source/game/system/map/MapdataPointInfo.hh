#pragma once

#include "game/system/map/MapdataAccessorBase.hh"

#include <egg/math/Vector.hh>
#include <egg/util/Stream.hh>

#include <span>

namespace Kinoko::System {

/// @brief Describes route information in a course KMP
class MapdataPointInfo {
public:
    /// @brief Represents a single point in a route
    struct Point {
        EGG::Vector3f pos; ///< Position of the point
        u16 setting[2];    ///< Route settings
    };

    /// @brief Represents the raw data structure of a route
    struct SData {
        u16 pointCount; ///< Number of points in the route
        u8 settings[2]; ///< Route settings
        Point points[]; ///< Array of points in the route
    };
    STATIC_ASSERT(sizeof(SData) == 0x4);

    /// @brief Constructor
    /// @param data Pointer to the raw route data
    MapdataPointInfo(const SData *data) : m_rawData(data) {
        EGG::RamStream stream =
                EGG::RamStream(data, sizeof(SData) + parse<u16>(data->pointCount) * sizeof(Point));
        read(stream);
    }

    /// @brief Default destructor
    ~MapdataPointInfo() = default;

    /// @brief Reads the route information data from the given stream
    /// @param stream The stream to read from
    void read(EGG::RamStream &stream) {
        u16 count = stream.read_u16();

        m_points = owning_span<Point>(count);

        for (auto &setting : m_settings) {
            setting = stream.read_u8();
        }

        for (auto &point : m_points) {
            EGG::Vector3f pos;
            pos.read(stream);

            u16 settings[2];
            settings[0] = stream.read_u16();
            settings[1] = stream.read_u16();

            point = Point(pos, {settings[0], settings[1]});
        }
    }

    /// @beginGetters
    [[nodiscard]] size_t pointCount() const {
        return m_points.size();
    }

    [[nodiscard]] u8 setting(size_t idx) const {
        ASSERT(idx < m_settings.size());
        return m_settings[idx];
    }

    [[nodiscard]] const owning_span<Point> &points() const {
        return m_points;
    }
    /// @endGetters

private:
    [[maybe_unused]] const SData *const m_rawData; ///< Pointer to the raw point info data
    std::array<u8, 2> m_settings;                  ///< Array of route settings
    owning_span<Point> m_points;                   ///< Array of points in the route
};

/// @brief Provides access to entries in the POTI section of the course KMP
class MapdataPointInfoAccessor
    : public MapdataAccessorBase<MapdataPointInfo, MapdataPointInfo::SData> {
public:
    /// @addr{0x80515D3C}
    /// @brief Constructor
    /// @param header Pointer to the section header of the POTI section
    MapdataPointInfoAccessor(const MapSectionHeader *header)
        : MapdataAccessorBase<MapdataPointInfo, MapdataPointInfo::SData>(header) {
        init(reinterpret_cast<const MapdataPointInfo::SData *>(m_sectionHeader + 1),
                parse<u16>(m_sectionHeader->count));
    }

    /// @brief Default virtual destructor
    ~MapdataPointInfoAccessor() override = default;

    /// @copydoc MapdataAccessorBase::init()
    void init(const MapdataPointInfo::SData *start, u16 count) {
        if (count != 0) {
            m_entries.reserve(count);
        }

        uintptr_t data = reinterpret_cast<uintptr_t>(start);

        for (u16 i = 0; i < count; ++i) {
            m_entries.push_back(EGG::egg_new<MapdataPointInfo>(
                    reinterpret_cast<MapdataPointInfo::SData *>(data)));
            data += m_entries[i]->pointCount() * sizeof(MapdataPointInfo::Point) +
                    offsetof(MapdataPointInfo::SData, points);
        }
    }
};

} // namespace Kinoko::System
