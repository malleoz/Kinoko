#include "CourseMap.hh"

#include "game/system/ResourceManager.hh"
#include "game/system/map/MapdataArea.hh"
#include "game/system/map/MapdataCannonPoint.hh"
#include "game/system/map/MapdataCheckPath.hh"
#include "game/system/map/MapdataCheckPoint.hh"
#include "game/system/map/MapdataFileAccessor.hh"
#include "game/system/map/MapdataGeoObj.hh"
#include "game/system/map/MapdataJugemPoint.hh"
#include "game/system/map/MapdataPointInfo.hh"
#include "game/system/map/MapdataStageInfo.hh"
#include "game/system/map/MapdataStartPoint.hh"

namespace Kinoko::System {

/// @addr{0x805127EC}
/// @brief Initializes the course map by loading the course KMP file and parsing its sections
/// @details Sorts the @ref m_area entries by priority. Also computes the starting position spacing,
/// represented by @ref m_skewAngle, @ref m_longitudinalOffset, @ref m_longitudinalWideOffset, @ref
/// m_lateralSpacing, and @ref m_longitudinalSpacing.
void CourseMap::init() {
    std::span<const u8> buffer = LoadFile("course.kmp");
    m_header = EGG::egg_new<MapdataFileAccessor>(
            reinterpret_cast<const MapdataFileAccessor::SData *>(buffer.data()));

    constexpr u32 AREA_SIGNATURE = Signature("AREA");
    constexpr u32 CANNON_POINT_SIGNATURE = Signature("CNPT");
    constexpr u32 CHECK_PATH_SIGNATURE = Signature("CKPH");
    constexpr u32 CHECK_POINT_SIGNATURE = Signature("CKPT");
    constexpr u32 GEO_OBJ_SIGNATURE = Signature("GOBJ");
    constexpr u32 JUGEM_POINT_SIGNATURE = Signature("JGPT");
    constexpr u32 START_POINT_SIGNATURE = Signature("KTPT");
    constexpr u32 POINT_INFO_SIGNATURE = Signature("POTI");
    constexpr u32 STAGE_INFO_SIGNATURE = Signature("STGI");

    m_startPoint = parseMapdata<MapdataStartPointAccessor>(START_POINT_SIGNATURE);
    m_checkPath = parseMapdata<MapdataCheckPathAccessor>(CHECK_PATH_SIGNATURE);
    m_checkPoint = parseMapdata<MapdataCheckPointAccessor>(CHECK_POINT_SIGNATURE);
    m_geoObj = parseMapdata<MapdataGeoObjAccessor>(GEO_OBJ_SIGNATURE);
    m_pointInfo = parseMapdata<MapdataPointInfoAccessor>(POINT_INFO_SIGNATURE);
    m_area = parseMapdata<MapdataAreaAccessor>(AREA_SIGNATURE);
    m_jugemPoint = parseMapdata<MapdataJugemPointAccessor>(JUGEM_POINT_SIGNATURE);
    m_cannonPoint = parseMapdata<MapdataCannonPointAccessor>(CANNON_POINT_SIGNATURE);
    m_stageInfo = parseMapdata<MapdataStageInfoAccessor>(STAGE_INFO_SIGNATURE);

    ASSERT(m_area);
    m_area->sort();

    m_areaTypeMask.makeAllZero();
    for (u16 i = 0; i < getAreaCount(); ++i) {
        MapdataAreaBase::Type type = getArea(i)->type();
        m_areaTypeMask.setBit(type);
    }

    const MapdataStageInfo *stageInfo = getStageInfo();
    constexpr u8 TRANSLATION_MODE_NARROW = 1;
    if (stageInfo && stageInfo->translationMode() == TRANSLATION_MODE_NARROW) {
        m_skewAngle = 25.0f;
        m_longitudinalOffset = 250.0f;
        m_longitudinalWideOffset = 0.0f;
    } else {
        m_skewAngle = 30.0f;
        m_longitudinalOffset = 400.0f;
        m_longitudinalWideOffset = 100.0f;
    }

    m_lateralSpacing = 800.0f;
    m_longitudinalSpacing = 1200.0f;
}

/// @addr{0x80511500}
/// @brief Finds the sector (checkpoint) that contains the given position
/// @param pos The position to check
/// @param checkpointIdx The index of the checkpoint to start the search from
/// @param distanceRatio Output parameter for the distance ratio within the sector
/// @return The ID of the sector containing the position, or -1 if not found
s16 CourseMap::findSector(const EGG::Vector3f &pos, u16 checkpointIdx, f32 &distanceRatio) {
    clearSectorChecked();

    MapdataCheckPoint *checkpoint = getCheckPoint(checkpointIdx);
    s16 id = -1;

    MapdataCheckPoint::SectorOccupancy occupancy =
            checkpoint->checkSectorAndDistanceRatio(pos, distanceRatio);
    checkpoint->setSearched(m_sectorSearchId);

    switch (occupancy) {
    // The player is fully inside the current checkpoint, so just set to current checkpoint
    case MapdataCheckPoint::SectorOccupancy::InsideSector:
        id = checkpoint->id();
        break;

    // The player is between the sides of the quad, but NOT between this checkpoint and
    // next; player is likely in the same checkpoint group
    case MapdataCheckPoint::SectorOccupancy::BetweenSides:
        id = findSectorBetweenSides(pos, checkpoint, distanceRatio);
        break;

    // The player is not between the sides of the quad (may still be between this checkpoint
    // and next); player is likely in a different checkpoint group
    case MapdataCheckPoint::SectorOccupancy::OutsideSector:
        id = findSectorOutsideSector(pos, checkpoint, distanceRatio);
        break;

    default:
        break;
    }

    return id > -1 ? id : findSectorRegional(pos, checkpoint, distanceRatio);
}

/// @addr{0x80511E7C}
/// @brief Finds at what frame subdivision of a frame the kart entered the checkpoint
/// @param i Index of the checkpoint.
/// @param pos Current position of the kart.
/// @param prevPos Previous position of the kart.
/// @return Entry offset in milliseconds for the specified checkpoint.
/// @details This assumes the player is entering the checkpoint as intended, and not from the
/// side.
u16 CourseMap::getCheckPointEntryOffsetMs(u16 i, const EGG::Vector3f &pos,
        const EGG::Vector3f &prevPos) const {
    EGG::Vector2f prevPosXZ = EGG::Vector2f(prevPos.x, prevPos.z);
    EGG::Vector2f posXZ = EGG::Vector2f(pos.x, pos.z);

    const MapdataCheckPoint *checkPoint = getCheckPoint(i);
    ASSERT(checkPoint);
    return checkPoint->getEntryOffsetMs(prevPosXZ, posXZ);
}

/// @brief Finds at what fractional millisecond of a frame the player enters the checkpoint
/// @param i Index of the checkpoint.
/// @param pos Current position of the kart.
/// @param prevPos Previous position of the kart.
/// @return Entry offset in fractional milliseconds for the specified checkpoint.
/// @details This assumes the player is entering the checkpoint as intended, and not from the
/// side. This function isn't in the base game, but it can be used to determine improvements to
/// runs.
f32 CourseMap::getCheckPointEntryOffsetExact(u16 i, const EGG::Vector3f &pos,
        const EGG::Vector3f &prevPos) const {
    EGG::Vector2f prevPosXZ = EGG::Vector2f(prevPos.x, prevPos.z);
    EGG::Vector2f posXZ = EGG::Vector2f(pos.x, pos.z);

    const MapdataCheckPoint *checkPoint = getCheckPoint(i);
    ASSERT(checkPoint);
    return checkPoint->getEntryOffsetExact(prevPosXZ, posXZ);
}

/// @addr{0x80516808}
/// @brief Gets the ID of the current area that contains the given position, if any
/// @param i The index of the area to check first
/// @param pos The position to check
/// @param type The type of area to search for
/// @return The ID of the area containing the position, or -1 if not found
s16 CourseMap::getCurrentAreaID(s16 i, const EGG::Vector3f &pos, MapdataAreaBase::Type type) const {
    // Kinoko-specific: skip the search entirely if the course has no areas of this type
    if (m_areaTypeMask.offBit(type)) {
        return -1;
    }

    // Check if we're colliding with the provided area ID
    if (i >= 0) {
        const MapdataAreaBase *area = getArea(i);
        if (area->type() == type && area->test(pos)) {
            return i;
        }
    }

    // Search all areas of the same type
    for (i = 0; i < getAreaCount(); ++i) {
        const MapdataAreaBase *area = getAreaSorted(i);
        if (area->type() == type && area->test(pos)) {
            return area->index();
        }
    }

    return -1;
}

/// @brief Identifies whether the entries in the `KTPT` section of the KMP contain a player
/// index
/// @return `true` if the KMP version is @ref REVISION_PLAYER_IDX or later, `false` otherwise.
/// @details Player indices were introduced to this section starting from revision 1830 of the
/// KMP file format.
bool CourseMap::hasPlayerIdx() const {
    return version() >= REVISION_PLAYER_IDX;
}

/// @brief Identifies whether the entries in the `AREA` section of the KMP contain a rail index
/// @return `true` if the KMP version is @ref REVISION_RAIL_IDX or later, `false` otherwise.
/// @details Rail indices were introduced to this section starting from revision 2200 of the KMP
/// file format.
bool CourseMap::hasRailIdx() const {
    return version() >= REVISION_RAIL_IDX;
}

/// @brief Fetches the check path accessor
/// @return A pointer to the check path accessor, @ref m_checkPath
MapdataCheckPathAccessor *CourseMap::checkPathAccessor() const {
    return m_checkPath;
}

/// @brief Fetches the check point accessor
/// @return A pointer to the check point accessor, @ref m_checkPoint
MapdataCheckPointAccessor *CourseMap::checkPointAccessor() const {
    return m_checkPoint;
}

/// @addr{0x80518AE0}
/// @brief Fetches the cannon point at the given index
/// @param i Index of the cannon point
/// @return A const pointer to the cannon point at the given index in @ref m_cannonPoint
const MapdataCannonPoint *CourseMap::getCannonPoint(u16 i) const {
    return i < getCannonPointCount() ? m_cannonPoint->get(i) : nullptr;
}

/// @addr{0x80515C70}
/// @brief Fetches the check path at the given index
/// @param i Index of the check path
/// @return A const pointer to the check path at the given index in @ref m_checkPath
const MapdataCheckPath *CourseMap::getCheckPath(u16 i) const {
    return i < getCheckPathCount() ? m_checkPath->get(i) : nullptr;
}

/// @addr{0x80515C24}
/// @brief Fetches the check point at the given index
/// @param i Index of the check point
/// @return A pointer to the check point at the given index in @ref m_checkPoint
MapdataCheckPoint *CourseMap::getCheckPoint(u16 i) {
    return i < getCheckPointCount() ? m_checkPoint->get(i) : nullptr;
}

/// @addr{0x80515C24}
/// @brief Fetches the check point at the given index
/// @param i Index of the check point
/// @return A const pointer to the check point at the given index in @ref m_checkPoint
const MapdataCheckPoint *CourseMap::getCheckPoint(u16 i) const {
    return i < getCheckPointCount() ? m_checkPoint->get(i) : nullptr;
}

/// @addr{0x80514148}
/// @brief Fetches the geo object at the given index
/// @param i Index of the geo object
/// @return A const pointer to the geo object at the given index in @ref m_geoObj
const MapdataGeoObj *CourseMap::getGeoObj(u16 i) const {
    return i < getGeoObjCount() ? m_geoObj->get(i) : nullptr;
}

/// @addr{0x80516768}
/// @brief Fetches the area at the given index
/// @param i Index of the area
/// @return A const pointer to the area at the given index in @ref m_area
const MapdataAreaBase *CourseMap::getArea(u16 i) const {
    return i < getAreaCount() ? m_area->get(i) : nullptr;
}

/// @addr{0x805167B4}
/// @brief Fetches the sorted area at the given index
/// @param i Index of the sorted area
/// @return A const pointer to the sorted area at the given index in @ref m_area
const MapdataAreaBase *CourseMap::getAreaSorted(u16 i) const {
    return i < getAreaCount() ? m_area->getSorted(i) : nullptr;
}

/// @addr{0x80515E04}
/// @brief Fetches the point info at the given index
/// @param i Index of the point info
/// @return A const pointer to the point info at the given index in @ref m_pointInfo
const MapdataPointInfo *CourseMap::getPointInfo(u16 i) const {
    return i < getPointInfoCount() ? m_pointInfo->get(i) : nullptr;
}

/// @addr{0x80518920}
/// @brief Fetches the respawn point at the given index
/// @param i Index of the respawn point
/// @return A const pointer to the respawn point at the given index in @ref m_jugemPoint
const MapdataJugemPoint *CourseMap::getJugemPoint(u16 i) const {
    return i < getJugemPointCount() ? m_jugemPoint->get(i) : nullptr;
}

/// @addr{0x80518B78}
/// @brief Fetches the stage info
/// @return A const pointer to the stage info at index 0 in @ref m_stageInfo
const MapdataStageInfo *CourseMap::getStageInfo() const {
    return getStageInfoCount() != 0 ? m_stageInfo->get(0) : nullptr;
}

/// @addr{0x80514B30}
/// @brief Fetches the start point at the given index
/// @param i Index of the start point
/// @return A const pointer to the start point at the given index in @ref m_startPoint
const MapdataStartPoint *CourseMap::getStartPoint(u16 i) const {
    return i < getStartPointCount() ? m_startPoint->get(i) : nullptr;
}

/// @brief Fetches the number of cannon points
/// @return The number of cannon points in @ref m_cannonPoint
u16 CourseMap::getCannonPointCount() const {
    return m_cannonPoint ? m_cannonPoint->size() : 0;
}

/// @brief Fetches the number of check paths
/// @return The number of check paths in @ref m_checkPath
u16 CourseMap::getCheckPathCount() const {
    return m_checkPath ? m_checkPath->size() : 0;
}

/// @brief Fetches the number of check points
/// @return The number of check points in @ref m_checkPoint
u16 CourseMap::getCheckPointCount() const {
    return m_checkPoint ? m_checkPoint->size() : 0;
}

/// @brief Fetches the number of geo objects
/// @return The number of geo objects in @ref m_geoObj
u16 CourseMap::getGeoObjCount() const {
    return m_geoObj ? m_geoObj->size() : 0;
}

/// @addr{0x80512CB4}
/// @brief Fetches the number of areas
/// @return The number of areas in @ref m_area
u16 CourseMap::getAreaCount() const {
    return m_area ? m_area->size() : 0;
}

/// @brief Fetches the number of point infos
/// @return The number of point infos in @ref m_pointInfo
u16 CourseMap::getPointInfoCount() const {
    return m_pointInfo ? m_pointInfo->size() : 0;
}

/// @brief Fetches the number of respawn points
/// @return The number of respawn points in @ref m_jugemPoint
u16 CourseMap::getJugemPointCount() const {
    return m_jugemPoint ? m_jugemPoint->size() : 0;
}

/// @brief Fetches the number of stage infos
/// @return The number of stage infos in @ref m_stageInfo
u16 CourseMap::getStageInfoCount() const {
    return m_stageInfo ? m_stageInfo->size() : 0;
}

/// @brief Fetches the number of start points
/// @return The number of start points in @ref m_startPoint
u16 CourseMap::getStartPointCount() const {
    return m_startPoint ? m_startPoint->size() : 0;
}

/// @brief Gets the version of this KMP file
/// @return The version of this KMP file
u32 CourseMap::version() const {
    return m_header->version();
}

/// @brief Gets the angle of diagonal rows relative to the pole position
/// @return The angle of diagonal rows relative to the pole position
f32 CourseMap::skewAngle() const {
    return m_skewAngle;
}

/// @brief Fetches the side-to-side spacing between karts in the same starting row
/// @return The side-to-side spacing between karts in the same starting row
f32 CourseMap::lateralSpacing() const {
    return m_lateralSpacing;
}

/// @brief Fetches the front-to-back spacing between karts in the same starting row
/// @return The front-to-back spacing between karts in the same starting row
f32 CourseMap::longitudinalSpacing() const {
    return m_longitudinalSpacing;
}

/// @brief Fetches the front-to-back offset of the starting row
/// @return The front-to-back offset of the starting row
f32 CourseMap::longitudinalOffset() const {
    return m_longitudinalOffset;
}

/// @brief Fetches the wide front-to-back offset of the starting row
/// @return The wide front-to-back offset of the starting row
f32 CourseMap::longitudinalWideOffset() const {
    return m_longitudinalWideOffset;
}

/// @addr{0x80512694}
/// @brief Creates the singleton instance of the @ref CourseMap
/// @return A pointer to the newly created @ref CourseMap instance
/// @pre The singleton instance of the @ref CourseMap does not already exist
CourseMap *CourseMap::CreateInstance() {
    ASSERT(!s_instance);
    s_instance = EGG::egg_new<CourseMap>();
    return s_instance;
}

/// @addr{0x8051271C}
/// @brief Destroys the singleton instance of the @ref CourseMap
/// @pre The singleton instance of the @ref CourseMap exists
void CourseMap::DestroyInstance() {
    ASSERT(s_instance);
    auto *instance = s_instance;
    s_instance = nullptr;
    EGG::egg_delete(instance);
}

/// @brief Returns the singleton instance of the @ref CourseMap
/// @return A pointer to the singleton instance of the @ref CourseMap
CourseMap *CourseMap::Instance() {
    return s_instance;
}

/// @addr{0x8051276C}
/// @brief Private constructor
/// @details Initializes @ref m_header, @ref m_startPoint, and @ref m_stageInfo to `nullptr`.
/// Sets @ref m_skewAngle, @ref m_lateralSpacing, @ref m_longitudinalSpacing, @ref
/// m_longitudinalOffset, and @ref m_longitudinalWideOffset to zero.
CourseMap::CourseMap()
    : m_header(nullptr),
      m_startPoint(nullptr),
      m_stageInfo(nullptr),
      m_skewAngle(0.0f),
      m_lateralSpacing(0.0f),
      m_longitudinalSpacing(0.0f),
      m_longitudinalOffset(0.0f),
      m_longitudinalWideOffset(0.0f) {
    // Performance improvement for Kinoko - a unique identifier representing the last search
    // performed, which avoids having to clear a "searched" flag for every checkpoint.
    m_sectorSearchId = 0;
}

/// @addr{0x805127AC}
/// @brief Private destructor that destroys the accessors to the `course.kmp` sections
CourseMap::~CourseMap() {
    if (s_instance) {
        s_instance = nullptr;
        WARN("CourseMap instance not explicitly handled!");
    }

    EGG::egg_delete(m_header);
    EGG::egg_delete(m_startPoint);
    EGG::egg_delete(m_checkPath);
    EGG::egg_delete(m_checkPoint);
    EGG::egg_delete(m_pointInfo);
    EGG::egg_delete(m_geoObj);
    EGG::egg_delete(m_area);
    EGG::egg_delete(m_jugemPoint);
    EGG::egg_delete(m_cannonPoint);
    EGG::egg_delete(m_stageInfo);
}

/// @addr{0x80511E00}
/// @brief Clears the sector checked status for all checkpoints
/// @details Iterates all checkpoints, calling @ref
/// MapdataCheckPointAccessor::clearSearched() on each one.
/// @note Kinoko-specific: rather than clearing every checkpoint, this starts a new search epoch, so
/// that checkpoints marked during previous searches no longer count as searched. Checkpoints are
/// only cleared when the epoch wraps around.
void CourseMap::clearSectorChecked() {
    if (++m_sectorSearchId != 0) {
        return;
    }

    for (size_t i = 0; i < m_checkPoint->size(); ++i) {
        getCheckPoint(i)->clearSearched();
    }

    m_sectorSearchId = 1;
}

/// @addr{0x80511110}
/// @brief Recursively depth-first searches for the sector (checkpoint) that contains the position
/// @param pos The position to check
/// @param depth The current depth of the recursive search
/// @param searchBackwardsFirst Whether to search backwards first
/// @param checkpoint The checkpoint to start the search from
/// @param distanceRatio Output parameter for the distance ratio within the sector
/// @param playerIsForwards Whether the player is moving forwards
/// @return The ID of the sector containing the position, or -1 if not found
/// @details This function performs a depth-first search through the checkpoint graph, starting from
/// the given checkpoint, to find the sector that contains the specified position. It considers the
/// player's movement direction and the search order (forwards or backwards) to determine the most
/// likely sector. The search is limited by a maximum depth to prevent infinite recursion.
s16 CourseMap::findRecursiveSector(const EGG::Vector3f &pos, s16 depth, bool searchBackwardsFirst,
        MapdataCheckPoint *checkpoint, f32 &distanceRatio, bool playerIsForwards) const {
    constexpr s16 MAX_DEPTH = 6;

    if (depth >= 0 && depth > MAX_DEPTH) {
        return -1;
    }

    MapdataCheckPoint::SectorOccupancy completion =
            MapdataCheckPoint::SectorOccupancy::OutsideSector;

    if (!checkpoint->searched(m_sectorSearchId)) {
        completion = checkpoint->checkSectorAndDistanceRatio(pos, distanceRatio);
        checkpoint->setSearched(m_sectorSearchId);
    }

    // If player is inside current checkpoint, stop searching
    if (completion == MapdataCheckPoint::SectorOccupancy::InsideSector) {
        return checkpoint->id();
    }

    // Search type 0: Search forwards first, then backwards
    if (!searchBackwardsFirst) {
        // If "player is forwards" but completion < 0, force completion to 0 and return
        // current checkpoint (GHOST CHECKPOINT!)
        if (playerIsForwards && completion == MapdataCheckPoint::SectorOccupancy::BetweenSides &&
                distanceRatio < 0.0f) {
            distanceRatio = 0.0f;
            return checkpoint->id();
        }

        // Stop if current checkpoint is a KCP
        if (checkpoint->type() >= 0) {
            return -1;
        }

        // If player is between the sides of the quad but NOT between this checkpoint and next, AND
        // completion > 0, then "player is forwards"
        bool forward = completion == MapdataCheckPoint::SectorOccupancy::BetweenSides &&
                distanceRatio > 0.0f;

        // Search forwards, including checkpoints already searched
        s16 id = searchNextCheckpoint(pos, depth, checkpoint, distanceRatio, forward, false);

        // If that fails, search backwards, excluding checkpoints already searched
        return id == -1 ?
                searchPrevCheckpoint(pos, depth, checkpoint, distanceRatio, forward, true) :
                id;
    }

    // Search type 1: Search backwards first, then forwards

    // If "player is backwards" flag is true but completion > 1, force completion to 1 and return
    // current checkpoint (GHOST CHECKPOINT!)
    if (playerIsForwards && completion == MapdataCheckPoint::SectorOccupancy::BetweenSides &&
            distanceRatio > 1.0f) {
        distanceRatio = 1.0f;
        return checkpoint->id();
    }

    // Stop if current checkpoint is a KCP (skipped for online players, but they aren't supported)
    if (checkpoint->type() >= 0) {
        return -1;
    }

    // If player is between the sides of the quad but NOT between this checkpoint and next, AND
    // completion < 0, then set "player is backwards" flag
    bool forward =
            completion == MapdataCheckPoint::SectorOccupancy::BetweenSides && distanceRatio < 0.0f;

    // Search backwards, including checkpoints already searched
    s16 id = searchPrevCheckpoint(pos, depth, checkpoint, distanceRatio, forward, false);

    // If that fails, search forwards, excluding checkpoints already searched
    return id == -1 ? searchNextCheckpoint(pos, depth, checkpoint, distanceRatio, forward, true) :
                      id;
}

/// @brief Searches for the sector containing the given position when the player is @ref
/// MapdataCheckPoint::SectorOccupancy::BetweenSides of the current checkpoint's quad
/// @param pos The position to check
/// @param checkpoint The checkpoint to start the search from
/// @param distanceRatio Output parameter for the completion ratio within the sector
/// @return The ID of the sector containing the position, or -1 if not found
s16 CourseMap::findSectorBetweenSides(const EGG::Vector3f &pos, MapdataCheckPoint *checkpoint,
        f32 &distanceRatio) {
    s16 id = -1;

    // Search order varies depending on whether player is closer to the next or previous checkpoint.
    if (distanceRatio > 0.5f) {
        // Step 1: Starting at current checkpoint, search forwards
        id = searchNextCheckpoint(pos, 0, checkpoint, distanceRatio, false, false);

        if (id != -1) {
            return id;
        }

        // Step 2: If step 1 fails, start at next checkpoint(s) and search backwards
        for (size_t i = 0; i < checkpoint->nextCount(); ++i) {
            MapdataCheckPoint *next = checkpoint->nextPoint(i);

            for (size_t j = 0; j < next->prevCount(); ++j) {
                MapdataCheckPoint *prev = next->prevPoint(j);

                if (prev == checkpoint) {
                    continue;
                }

                id = findRecursiveSector(pos, 1, true, prev, distanceRatio, false);
                if (id != -1) {
                    return id;
                }
            }
        }

        // Step 3: If step 2 fails, start at previous checkpoint(s) and search forwards
        for (size_t i = 0; i < checkpoint->prevCount(); ++i) {
            MapdataCheckPoint *prev = checkpoint->prevPoint(i);

            for (size_t j = 0; j < prev->nextCount(); ++j) {
                MapdataCheckPoint *next = prev->nextPoint(j);

                if (next == checkpoint) {
                    continue;
                }

                id = findRecursiveSector(pos, 1, false, next, distanceRatio, false);
                if (id != -1) {
                    return id;
                }
            }
        }

        // Step 4: If step 3 fails, start at current checkpoint and search backwards
        return searchPrevCheckpoint(pos, 0, checkpoint, distanceRatio, false, false);
    } else {
        // Step 1: Starting at current checkpoint, search backwards
        id = searchPrevCheckpoint(pos, 0, checkpoint, distanceRatio, false, false);

        if (id != -1) {
            return id;
        }

        // Step 2: If step 1 fails, start at prev checkpoint(s) and search forwards
        for (size_t i = 0; i < checkpoint->prevCount(); ++i) {
            MapdataCheckPoint *prev = checkpoint->prevPoint(i);

            for (size_t j = 0; j < prev->nextCount(); ++j) {
                MapdataCheckPoint *next = prev->nextPoint(j);

                if (next == checkpoint) {
                    continue;
                }

                id = findRecursiveSector(pos, 1, false, next, distanceRatio, false);

                if (id != -1) {
                    return id;
                }
            }
        }

        // Step 3: If step 2 fails, start at next checkpoint(s) and search backwards
        for (size_t i = 0; i < checkpoint->nextCount(); ++i) {
            MapdataCheckPoint *next = checkpoint->nextPoint(i);

            for (size_t j = 0; j < next->prevCount(); ++j) {
                MapdataCheckPoint *prev = next->prevPoint(j);

                if (prev == checkpoint) {
                    continue;
                }

                id = findRecursiveSector(pos, 1, true, prev, distanceRatio, false);

                if (id != -1) {
                    return id;
                }
            }
        }

        // Step 4: If step 3 fails, start at current checkpoint and search forwards
        return searchNextCheckpoint(pos, 0, checkpoint, distanceRatio, false, false);
    }

    return id;
}

/// @brief Searches for the sector containing the given position when the player is @ref
/// MapdataCheckPoint::SectorOccupancy::OutsideSector the current checkpoint's quad
/// @param pos The position to check
/// @param checkpoint The checkpoint to start the search from
/// @param distanceRatio Output parameter for the completion ratio within the sector
/// @return The ID of the sector containing the position, or -1 if not found
s16 CourseMap::findSectorOutsideSector(const EGG::Vector3f &pos, MapdataCheckPoint *checkpoint,
        f32 &distanceRatio) {
    s16 id = -1;

    // Step 1: Starting at next checkpoint(s), search backwards
    for (size_t i = 0; i < checkpoint->nextCount(); ++i) {
        MapdataCheckPoint *next = checkpoint->nextPoint(i);

        for (size_t j = 0; j < next->prevCount(); ++j) {
            MapdataCheckPoint *prev = next->prevPoint(j);

            if (prev == checkpoint) {
                continue;
            }

            id = findRecursiveSector(pos, 1, true, prev, distanceRatio, false);

            if (id != -1) {
                return id;
            }
        }
    }

    // Step 2: If step 1 fails, start at prev checkpoint(s) and search forwards
    for (size_t i = 0; i < checkpoint->prevCount(); ++i) {
        MapdataCheckPoint *prev = checkpoint->prevPoint(i);

        for (size_t j = 0; j < prev->nextCount(); ++j) {
            MapdataCheckPoint *next = prev->nextPoint(j);

            if (next == checkpoint) {
                continue;
            }

            id = findRecursiveSector(pos, 1, false, next, distanceRatio, false);

            if (id != -1) {
                return id;
            }
        }
    }

    // Step 3: If step 2 fails, start at next checkpoint(s) and search forwards
    for (size_t i = 0; i < checkpoint->nextCount(); ++i) {
        id = findRecursiveSector(pos, 1, false, checkpoint->nextPoint(i), distanceRatio, false);

        if (id != -1) {
            return id;
        }
    }

    // Step 4: If step 3 fails, start at prev checkpoint(s) and search backwards
    for (size_t i = 0; i < checkpoint->prevCount(); ++i) {
        id = findRecursiveSector(pos, 1, true, checkpoint->prevPoint(i), distanceRatio, false);

        if (id != -1) {
            return id;
        }
    }

    return id;
}

/// @brief Last resort fallback which removes the recursion limit and searches all checkpoints
/// @param pos The position to check
/// @param checkpoint The checkpoint to start the search from
/// @param distanceRatio Output parameter for the completion ratio within the sector
/// @return The ID of the sector containing the position, or -1 if not found
s16 CourseMap::findSectorRegional(const EGG::Vector3f &pos, MapdataCheckPoint *checkpoint,
        f32 &distanceRatio) {
    s16 id = -1;

    // Step 1: Search all next checkpoints until player or key checkpoint is found
    for (size_t i = 0; i < checkpoint->nextCount(); ++i) {
        id = findRecursiveSector(pos, -1, false, checkpoint->nextPoint(i), distanceRatio, false);

        if (id != -1) {
            return id;
        }
    }

    // Step 2: Search all previous checkpoints until player or key checkpoint is found
    for (size_t i = 0; i < checkpoint->prevCount(); ++i) {
        id = findRecursiveSector(pos, -1, true, checkpoint->prevPoint(i), distanceRatio, false);

        if (id != -1) {
            return id;
        }
    }

    return id;
}

/// @addr{0x80510F58}
/// @brief Recurses through the next checkpoints to find the sector containing the given position
/// @param pos The position to check
/// @param depth The current depth of the recursive search
/// @param checkpoint The checkpoint to start the search from
/// @param distanceRatio Output parameter for the completion ratio within the sector
/// @param playerIsForwards Whether the player is moving forwards
/// @param useCache Whether to use the cached search results
/// @return The ID of the sector containing the position, or -1 if not found
s16 CourseMap::searchNextCheckpoint(const EGG::Vector3f &pos, s16 depth,
        const MapdataCheckPoint *checkpoint, f32 &distanceRatio, bool playerIsForwards,
        bool useCache) const {
    s16 id = -1;
    depth = depth >= 0 ? depth + 1 : -1;

    for (size_t i = 0; i < checkpoint->nextCount(); ++i) {
        MapdataCheckPoint *next = checkpoint->nextPoint(i);

        if (!useCache || !next->searched(m_sectorSearchId)) {
            id = findRecursiveSector(pos, depth, false, next, distanceRatio, playerIsForwards);

            if (id != -1) {
                return id;
            }
        }
    }

    return id;
}

/// @addr{0x80511034}
/// @brief Recurses through the previous checkpoints to find the sector containing the given
/// position
/// @param pos The position to check
/// @param depth The current depth of the recursive search
/// @param checkpoint The checkpoint to start the search from
/// @param distanceRatio Output parameter for the completion ratio within the sector
/// @param playerIsForwards Whether the player is moving forwards
/// @param useCache Whether to use the cached search results
/// @return The ID of the sector containing the position, or -1 if not found
s16 CourseMap::searchPrevCheckpoint(const EGG::Vector3f &pos, s16 depth,
        const MapdataCheckPoint *checkpoint, f32 &distanceRatio, bool playerIsForwards,
        bool useCache) const {
    s16 id = -1;
    depth = depth >= 0 ? depth + 1 : -1;

    for (size_t i = 0; i < checkpoint->prevCount(); ++i) {
        MapdataCheckPoint *prev = checkpoint->prevPoint(i);

        if (!useCache || !prev->searched(m_sectorSearchId)) {
            id = findRecursiveSector(pos, depth, true, prev, distanceRatio, playerIsForwards);

            if (id != -1) {
                return id;
            }
        }
    }

    return id;
}

/// @brief Parses a section of the `course.kmp` file and returns an instance of the
/// specified map data accessor type
/// @tparam T The type of map data accessor to parse. Must be derived from @ref
/// MapdataAccessorBase.
/// @param sectionName Signature of the section to parse.
/// @return Pointer to an instance of the specified map data accessor type, or `nullptr` if
/// the section is not found.
template <MapdataDerived T>
T *CourseMap::parseMapdata(u32 sectionName) const {
    const MapSectionHeader *sectionPtr = m_header->findSection(sectionName);
    return sectionPtr ? EGG::egg_new<T>(sectionPtr) : nullptr;
}

/// @addr{0x80512C10}
/// @brief Loads a course KMP file into memory
/// @param filename The name of the course KMP file to load
/// @return A span over the loaded file data, or an empty span if the file could not be
/// loaded
std::span<const u8> CourseMap::LoadFile(const char *filename) {
    return ResourceManager::Instance()->getFile(filename, ArchiveId::Course);
}

CourseMap *CourseMap::s_instance = nullptr;

} // namespace Kinoko::System
