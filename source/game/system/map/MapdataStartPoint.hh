#pragma once

#include "game/system/map/MapdataAccessorBase.hh"

#include <egg/math/Vector.hh>

namespace Kinoko::System {

/// @brief Describes the starting point information of a course
class MapdataStartPoint {
public:
    /// @brief Represents the raw data structure of the starting point information
    struct SData {
        EGG::Vector3f pos; ///< Position of the starting point
        EGG::Vector3f rot; ///< Rotation of the starting point
        // Pre Revision 1830: End of structure
        s16 playerIndex; ///< Index of the player starting at this point
    };

    MapdataStartPoint(const SData *data);

    /// @brief Default destructor
    ~MapdataStartPoint() = default;

    void read(EGG::Stream &stream);
    void findKartStartPoint(EGG::Vector3f &pos, EGG::Vector3f &rot, u8 placement, u8 playerCount);

private:
    [[maybe_unused]] const SData *const m_rawData; ///< Pointer to the raw starting point data
    EGG::Vector3f m_position;                      ///< Position of the starting point
    EGG::Vector3f m_rotation;                      ///< Rotation of the starting point
    s16 m_playerIndex;                             ///< Index of the player starting at this point
};

/// @brief Provides access to entries in the KTPT section of the course KMP
class MapdataStartPointAccessor
    : public MapdataAccessorBase<MapdataStartPoint, MapdataStartPoint::SData> {
public:
    MapdataStartPointAccessor(const MapSectionHeader *header);
    ~MapdataStartPointAccessor() override;
};

} // namespace Kinoko::System
