#pragma once

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

namespace Kinoko {

namespace Host {

class Context;

} // namespace Host

/// @brief High-level handling for generic system operations, such as input reading, race
/// configuration, and resource management.
namespace System {

/// @brief Concept which enforces that a type is derived from @ref MapdataAccessorBase
/// @tparam T The type to check if it's derived from @ref MapdataAccessorBase.
template <typename T>
concept MapdataDerived = is_derived_from_template_v<MapdataAccessorBase, T>;

/// @addr{0x809BD6E8}
/// @brief Top-level interface for accessing data from `course.kmp`
/// @details This class is responsible for parsing and providing access to various sections of the
/// `course.kmp` file, such as checkpoints, the kart starting position, and other course data.
class CourseMap : EGG::Disposer {
    /// @brief Grants access to the singleton so a @ref Host::Context can restore the instance's
    /// state on context switch
    friend class Host::Context;

public:
    void init();

    /// @brief Parses a section of the `course.kmp` file and returns an instance of the specified
    /// map data accessor type
    /// @tparam T The type of map data accessor to parse. Must be derived from @ref
    /// MapdataAccessorBase.
    /// @param sectionName Signature of the section to parse.
    /// @return Pointer to an instance of the specified map data accessor type, or `nullptr` if the
    /// section is not found.
    template <MapdataDerived T>
    [[nodiscard]] T *parseMapdata(u32 sectionName) const {
        const MapSectionHeader *sectionPtr = m_header->findSection(sectionName);
        return sectionPtr ? EGG::egg_new<T>(sectionPtr) : nullptr;
    }

    [[nodiscard]] s16 findSector(const EGG::Vector3f &pos, u16 checkpointIdx, f32 &distanceRatio);
    [[nodiscard]] s16 findRecursiveSector(const EGG::Vector3f &pos, s16 depth,
            bool searchBackwardsFirst, MapdataCheckPoint *checkpoint, f32 &completion,
            bool playerIsForwards) const;

    /// @addr{0x80511E7C}
    /// @brief Finds at what frame subdivision of a frame the kart entered the checkpoint
    /// @param i Index of the checkpoint.
    /// @param pos Current position of the kart.
    /// @param prevPos Previous position of the kart.
    /// @return Entry offset in milliseconds for the specified checkpoint.
    /// @details This assumes the player is entering the checkpoint as intended, and not from the
    /// side.
    [[nodiscard]] u16 getCheckPointEntryOffsetMs(u16 i, const EGG::Vector3f &pos,
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
    [[nodiscard]] f32 getCheckPointEntryOffsetExact(u16 i, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos) const {
        EGG::Vector2f prevPosXZ = EGG::Vector2f(prevPos.x, prevPos.z);
        EGG::Vector2f posXZ = EGG::Vector2f(pos.x, pos.z);

        const MapdataCheckPoint *checkPoint = getCheckPoint(i);
        ASSERT(checkPoint);
        return checkPoint->getEntryOffsetExact(prevPosXZ, posXZ);
    }

    [[nodiscard]] s16 getCurrentAreaID(s16 i, const EGG::Vector3f &pos,
            MapdataAreaBase::Type type) const;

    /// @brief Identifies whether the entries in the `KTPT` section of the KMP contain a player
    /// index
    /// @return `true` if the KMP version is @ref REVISION_PLAYER_IDX or later, `false` otherwise.
    /// @details Player indices were introduced to this section starting from revision 1830 of the
    /// KMP file format.
    [[nodiscard]] bool hasPlayerIdx() const {
        return version() >= REVISION_PLAYER_IDX;
    }

    /// @brief Identifies whether the entries in the `AREA` section of the KMP contain a rail index
    /// @return `true` if the KMP version is @ref REVISION_RAIL_IDX or later, `false` otherwise.
    /// @details Rail indices were introduced to this section starting from revision 2200 of the KMP
    /// file format.
    [[nodiscard]] bool hasRailIdx() const {
        return version() >= REVISION_RAIL_IDX;
    }

    /// @beginGetters

    /// @addr{0x80518AE0}
    /// @brief Fetches the cannon point at the given index
    /// @param i Index of the cannon point
    /// @return A const pointer to the cannon point at the given index in @ref m_cannonPoint
    [[nodiscard]] const MapdataCannonPoint *getCannonPoint(u16 i) const {
        return i < getCannonPointCount() ? m_cannonPoint->get(i) : nullptr;
    }

    /// @addr{0x80515C70}
    /// @brief Fetches the check path at the given index
    /// @param i Index of the check path
    /// @return A const pointer to the check path at the given index in @ref m_checkPath
    [[nodiscard]] const MapdataCheckPath *getCheckPath(u16 i) const {
        return i < getCheckPathCount() ? m_checkPath->get(i) : nullptr;
    }

    /// @addr{0x80515C24}
    /// @brief Fetches the check point at the given index
    /// @param i Index of the check point
    /// @return A pointer to the check point at the given index in @ref m_checkPoint
    [[nodiscard]] MapdataCheckPoint *getCheckPoint(u16 i) {
        return i < getCheckPointCount() ? m_checkPoint->get(i) : nullptr;
    }

    /// @addr{0x80515C24}
    /// @brief Fetches the check point at the given index
    /// @param i Index of the check point
    /// @return A const pointer to the check point at the given index in @ref m_checkPoint
    [[nodiscard]] const MapdataCheckPoint *getCheckPoint(u16 i) const {
        return i < getCheckPointCount() ? m_checkPoint->get(i) : nullptr;
    }

    /// @addr{0x80514148}
    /// @brief Fetches the geo object at the given index
    /// @param i Index of the geo object
    /// @return A const pointer to the geo object at the given index in @ref m_geoObj
    [[nodiscard]] const MapdataGeoObj *getGeoObj(u16 i) const {
        return i < getGeoObjCount() ? m_geoObj->get(i) : nullptr;
    }

    /// @addr{0x80516768}
    /// @brief Fetches the area at the given index
    /// @param i Index of the area
    /// @return A const pointer to the area at the given index in @ref m_area
    [[nodiscard]] const MapdataAreaBase *getArea(u16 i) const {
        return i < getAreaCount() ? m_area->get(i) : nullptr;
    }

    /// @addr{0x805167B4}
    /// @brief Fetches the sorted area at the given index
    /// @param i Index of the sorted area
    /// @return A const pointer to the sorted area at the given index in @ref m_area
    [[nodiscard]] const MapdataAreaBase *getAreaSorted(u16 i) const {
        return i < getAreaCount() ? m_area->getSorted(i) : nullptr;
    }

    /// @addr{0x80515E04}
    /// @brief Fetches the point info at the given index
    /// @param i Index of the point info
    /// @return A const pointer to the point info at the given index in @ref m_pointInfo
    [[nodiscard]] const MapdataPointInfo *getPointInfo(u16 i) const {
        return i < getPointInfoCount() ? m_pointInfo->get(i) : nullptr;
    }

    /// @addr{0x80518920}
    /// @brief Fetches the respawn point at the given index
    /// @param i Index of the respawn point
    /// @return A const pointer to the respawn point at the given index in @ref m_jugemPoint
    [[nodiscard]] const MapdataJugemPoint *getJugemPoint(u16 i) const {
        return i < getJugemPointCount() ? m_jugemPoint->get(i) : nullptr;
    }

    /// @addr{0x80518B78}
    /// @brief Fetches the stage info
    /// @return A const pointer to the stage info at index 0 in @ref m_stageInfo
    [[nodiscard]] const MapdataStageInfo *getStageInfo() const {
        return getStageInfoCount() != 0 ? m_stageInfo->get(0) : nullptr;
    }

    /// @addr{0x80514B30}
    /// @brief Fetches the start point at the given index
    /// @param i Index of the start point
    /// @return A const pointer to the start point at the given index in @ref m_startPoint
    [[nodiscard]] const MapdataStartPoint *getStartPoint(u16 i) const {
        return i < getStartPointCount() ? m_startPoint->get(i) : nullptr;
    }

    /// @brief Fetches the number of cannon points
    /// @return The number of cannon points in @ref m_cannonPoint
    [[nodiscard]] u16 getCannonPointCount() const {
        return m_cannonPoint ? m_cannonPoint->size() : 0;
    }

    /// @brief Fetches the number of check paths
    /// @return The number of check paths in @ref m_checkPath
    [[nodiscard]] u16 getCheckPathCount() const {
        return m_checkPath ? m_checkPath->size() : 0;
    }

    /// @brief Fetches the number of check points
    /// @return The number of check points in @ref m_checkPoint
    [[nodiscard]] u16 getCheckPointCount() const {
        return m_checkPoint ? m_checkPoint->size() : 0;
    }

    /// @brief Fetches the number of geo objects
    /// @return The number of geo objects in @ref m_geoObj
    [[nodiscard]] u16 getGeoObjCount() const {
        return m_geoObj ? m_geoObj->size() : 0;
    }

    /// @addr{0x80512CB4}
    /// @brief Fetches the number of areas
    /// @return The number of areas in @ref m_area
    [[nodiscard]] u16 getAreaCount() const {
        return m_area ? m_area->size() : 0;
    }

    /// @brief Fetches the number of point infos
    /// @return The number of point infos in @ref m_pointInfo
    [[nodiscard]] u16 getPointInfoCount() const {
        return m_pointInfo ? m_pointInfo->size() : 0;
    }

    /// @brief Fetches the number of respawn points
    /// @return The number of respawn points in @ref m_jugemPoint
    [[nodiscard]] u16 getJugemPointCount() const {
        return m_jugemPoint ? m_jugemPoint->size() : 0;
    }

    /// @brief Fetches the number of stage infos
    /// @return The number of stage infos in @ref m_stageInfo
    [[nodiscard]] u16 getStageInfoCount() const {
        return m_stageInfo ? m_stageInfo->size() : 0;
    }

    /// @brief Fetches the number of start points
    /// @return The number of start points in @ref m_startPoint
    [[nodiscard]] u16 getStartPointCount() const {
        return m_startPoint ? m_startPoint->size() : 0;
    }

    /// @brief Gets the version of this KMP file
    /// @return The version of this KMP file
    [[nodiscard]] u32 version() const {
        return m_header->version();
    }

    /// @brief Fetches the check path accessor
    /// @return A pointer to the check path accessor, @ref m_checkPath
    [[nodiscard]] MapdataCheckPathAccessor *checkPath() const {
        return m_checkPath;
    }

    /// @brief Fetches the check point accessor
    /// @return A pointer to the check point accessor, @ref m_checkPoint
    [[nodiscard]] MapdataCheckPointAccessor *checkPoint() const {
        return m_checkPoint;
    }

    /// @brief Gets the angle of diagonal rows relative to the pole position
    /// @return The angle of diagonal rows relative to the pole position
    [[nodiscard]] f32 skewAngle() const {
        return m_skewAngle;
    }

    /// @brief Fetches the side-to-side spacing between karts in the same starting row
    /// @return The side-to-side spacing between karts in the same starting row
    [[nodiscard]] f32 lateralSpacing() const {
        return m_lateralSpacing;
    }

    /// @brief Fetches the front-to-back spacing between karts in the same starting row
    /// @return The front-to-back spacing between karts in the same starting row
    [[nodiscard]] f32 longitudinalSpacing() const {
        return m_longitudinalSpacing;
    }

    /// @brief Fetches the front-to-back offset of the starting row
    /// @return The front-to-back offset of the starting row
    [[nodiscard]] f32 longitudinalOffset() const {
        return m_longitudinalOffset;
    }

    /// @brief Fetches the wide front-to-back offset of the starting row
    /// @return The wide front-to-back offset of the starting row
    [[nodiscard]] f32 longitudinalWideOffset() const {
        return m_longitudinalWideOffset;
    }

    /// @endGetters

    /// @addr{0x80512694}
    /// @brief Creates the singleton instance of the @ref CourseMap
    /// @return A pointer to the newly created @ref CourseMap instance
    /// @pre The singleton instance of the @ref CourseMap does not already exist
    static CourseMap *CreateInstance() {
        ASSERT(!s_instance);
        s_instance = EGG::egg_new<CourseMap>();
        return s_instance;
    }

    /// @addr{0x8051271C}
    /// @brief Destroys the singleton instance of the @ref CourseMap
    /// @pre The singleton instance of the @ref CourseMap exists
    static void DestroyInstance() {
        ASSERT(s_instance);
        auto *instance = s_instance;
        s_instance = nullptr;
        EGG::egg_delete(instance);
    }

    /// @brief Returns the singleton instance of the @ref CourseMap
    /// @return A pointer to the singleton instance of the @ref CourseMap
    [[nodiscard]] static CourseMap *Instance() {
        return s_instance;
    }

private:
    EGG_NEW_DELETE_FRIEND

    /// @addr{0x8051276C}
    /// @brief Private constructor
    /// @details Initializes @ref m_header, @ref m_startPoint, and @ref m_stageInfo to `nullptr`.
    /// Sets @ref m_skewAngle, @ref m_lateralSpacing, @ref m_longitudinalSpacing, @ref
    /// m_longitudinalOffset, and @ref m_longitudinalWideOffset to zero.
    CourseMap()
        : m_header(nullptr),
          m_startPoint(nullptr),
          m_stageInfo(nullptr),
          m_skewAngle(0.0f),
          m_lateralSpacing(0.0f),
          m_longitudinalSpacing(0.0f),
          m_longitudinalOffset(0.0f),
          m_longitudinalWideOffset(0.0f) {}

    ~CourseMap() override;

    [[nodiscard]] s16 findSectorBetweenSides(const EGG::Vector3f &pos,
            MapdataCheckPoint *checkpoint, f32 &distanceRatio);
    [[nodiscard]] s16 findSectorOutsideSector(const EGG::Vector3f &pos,
            MapdataCheckPoint *checkpoint, f32 &distanceRatio);
    [[nodiscard]] s16 findSectorRegional(const EGG::Vector3f &pos, MapdataCheckPoint *checkpoint,
            f32 &distanceRatio);
    [[nodiscard]] s16 searchNextCheckpoint(const EGG::Vector3f &pos, s16 depth,
            const MapdataCheckPoint *checkpoint, f32 &distanceRatio, bool playerIsForwards,
            bool useCache) const;
    [[nodiscard]] s16 searchPrevCheckpoint(const EGG::Vector3f &pos, s16 depth,
            const MapdataCheckPoint *checkpoint, f32 &distanceRatio, bool playerIsForwards,
            bool useCache) const;

    /// @addr{0x80511E00}
    /// @brief Clears the sector checked status for all checkpoints
    /// @details Iterates all checkpoints, calling @ref MapdataCheckPointAccessor::clearSearched()
    /// on each one.
    void clearSectorChecked() {
        for (size_t i = 0; i < m_checkPoint->size(); ++i) {
            getCheckPoint(i)->clearSearched();
        }
    }

    MapdataFileAccessor *m_header;             ///< Accessor for the header of the KMP file
    MapdataStartPointAccessor *m_startPoint;   ///< Accessorr for the STPT section of the KMP file
    MapdataCheckPathAccessor *m_checkPath;     ///< Accessor for the CKPH section of the KMP file
    MapdataCheckPointAccessor *m_checkPoint;   ///< Accessor for the CKPT section of the KMP file
    MapdataPointInfoAccessor *m_pointInfo;     ///< Accessor for the POTI section of the KMP file
    MapdataGeoObjAccessor *m_geoObj;           ///< Accessor for the GOBJ section of the KMP file
    MapdataAreaAccessor *m_area;               ///< Accessor for the AREA section of the KMP file
    MapdataJugemPointAccessor *m_jugemPoint;   ///< Accessor for the JGPT section of the KMP file
    MapdataCannonPointAccessor *m_cannonPoint; ///< Accessor for the CNPT section of the KMP file
    MapdataStageInfoAccessor *m_stageInfo;     ///< Accessor for the STGI section of the KMP file

    f32 m_skewAngle;              ///< Angle of the diagonal rows relative to the pole position
    f32 m_lateralSpacing;         ///< Side-to-side spacing between karts in the same starting row
    f32 m_longitudinalSpacing;    ///< Front-to-back spacing between karts in the same starting row
    f32 m_longitudinalOffset;     ///< Front-to-back offset per half-row
    f32 m_longitudinalWideOffset; ///< Front-to-back offset for wide starting grids

    /// @addr{0x80512C10}
    /// @brief Loads a course KMP file into memory
    /// @param filename The name of the course KMP file to load
    /// @return A span over the loaded file data, or an empty span if the file could not be loaded
    [[nodiscard]] std::span<const u8> LoadFile(const char *filename) {
        return ResourceManager::Instance()->getFile(filename, ArchiveId::Course);
    }

    /// @brief Helper function which converts a big-endian ASCII string to a `u32` signature used to
    /// identify each section of the course KMP file
    /// @param s A reference to a `char[4]` array of containing the big-endian ASCII string to
    /// convert to a `u32` signature
    /// @return The `u32` signature corresponding to the big-endian ASCII string
    [[nodiscard]] static consteval u32 Signature(const char (&s)[5]) {
        return (u32(s[0]) << 24) | (u32(s[1]) << 16) | (u32(s[2]) << 8) | u32(s[3]);
    }

    /// @addr{0x809BD6E8}
    /// @brief The pointer to the singleton instance of the class
    static CourseMap *s_instance;

    /// @brief Revision of the KMP file format at which the player index was introduced to the
    /// `KTPT` section
    /// @details Even though all vanilla course KMP files have revision 2520, we add checks for
    /// posterity's sake.
    static constexpr u32 REVISION_PLAYER_IDX = 1831;

    /// @brief Revision of the KMP file format at which the rail index was introduced to the `AREA`
    /// section
    /// @details Even though all vanilla course KMP files have revision 2520, we add checks for
    /// posterity's sake.
    static constexpr u32 REVISION_RAIL_IDX = 2200;
};

} // namespace System

} // namespace Kinoko
