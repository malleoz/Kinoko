#pragma once

#include "game/system/map/MapdataAccessorBase.hh"

#include <egg/math/Vector.hh>

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

    MapdataPointInfo(const SData *data);
    ~MapdataPointInfo();

    void read(EGG::RamStream &stream);

    /// @beginGetters

    [[nodiscard]] size_t pointCount() const;
    [[nodiscard]] u8 setting(size_t idx) const;
    [[nodiscard]] const owning_span<Point> &points() const;

    /// @endGetters

private:
    [[maybe_unused]] const SData *const m_rawData; ///< Pointer to the raw point info data
    std::array<u8, 2> m_settings;                  ///< Array of route settings
    owning_span<Point> m_points;                   ///< Array of points in the route
};

/// @brief Provides access to entries in the `POTI` section of the course KMP
class MapdataPointInfoAccessor
    : public MapdataAccessorBase<MapdataPointInfo, MapdataPointInfo::SData> {
public:
    MapdataPointInfoAccessor(const MapSectionHeader *header);
    ~MapdataPointInfoAccessor() override;

private:
    void init(const MapdataPointInfo::SData *start, u16 count);
};

} // namespace Kinoko::System
