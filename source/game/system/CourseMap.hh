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

        MapdataCheckPoint *checkPoint = getCheckPoint(i);
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

        MapdataCheckPoint *checkPoint = getCheckPoint(i);
        ASSERT(checkPoint);
        return checkPoint->getEntryOffsetExact(prevPosXZ, posXZ);
    }

    [[nodiscard]] s16 getCurrentAreaID(s16 i, const EGG::Vector3f &pos,
            MapdataAreaBase::Type type) const;

    /// @beginGetters
    /// @addr{0x80518AE0}
    [[nodiscard]] MapdataCannonPoint *getCannonPoint(u16 i) const {
        return i < getCannonPointCount() ? m_cannonPoint->get(i) : nullptr;
    }

    /// @addr{0x80515C70}
    [[nodiscard]] MapdataCheckPath *getCheckPath(u16 i) const {
        return i < getCheckPathCount() ? m_checkPath->get(i) : nullptr;
    }

    /// @addr{0x80515C24}
    [[nodiscard]] MapdataCheckPoint *getCheckPoint(u16 i) const {
        return i < getCheckPointCount() ? m_checkPoint->get(i) : nullptr;
    }

    /// @addr{0x80514148}
    [[nodiscard]] MapdataGeoObj *getGeoObj(u16 i) const {
        return i < getGeoObjCount() ? m_geoObj->get(i) : nullptr;
    }

    /// @addr{0x80516768}
    [[nodiscard]] MapdataAreaBase *getArea(u16 i) const {
        return i < getAreaCount() ? m_area->get(i) : nullptr;
    }

    /// @addr{0x805167B4}
    [[nodiscard]] MapdataAreaBase *getAreaSorted(u16 i) const {
        return i < getAreaCount() ? m_area->getSorted(i) : nullptr;
    }

    /// @addr{0x80515E04}
    [[nodiscard]] MapdataPointInfo *getPointInfo(u16 i) const {
        return i < getPointInfoCount() ? m_pointInfo->get(i) : nullptr;
    }

    /// @addr{0x80518920}
    [[nodiscard]] MapdataJugemPoint *getJugemPoint(u16 i) const {
        return i < getJugemPointCount() ? m_jugemPoint->get(i) : nullptr;
    }

    /// @addr{0x80518B78}
    [[nodiscard]] MapdataStageInfo *getStageInfo() const {
        return getStageInfoCount() != 0 ? m_stageInfo->get(0) : nullptr;
    }

    /// @addr{0x80514B30}
    [[nodiscard]] MapdataStartPoint *getStartPoint(u16 i) const {
        return i < getStartPointCount() ? m_startPoint->get(i) : nullptr;
    }

    [[nodiscard]] u16 getCannonPointCount() const {
        return m_cannonPoint ? m_cannonPoint->size() : 0;
    }

    [[nodiscard]] u16 getCheckPathCount() const {
        return m_checkPath ? m_checkPath->size() : 0;
    }

    [[nodiscard]] u16 getCheckPointCount() const {
        return m_checkPoint ? m_checkPoint->size() : 0;
    }

    [[nodiscard]] u16 getGeoObjCount() const {
        return m_geoObj ? m_geoObj->size() : 0;
    }

    /// @addr{0x80512CB4}
    [[nodiscard]] u16 getAreaCount() const {
        return m_area ? m_area->size() : 0;
    }

    [[nodiscard]] u16 getPointInfoCount() const {
        return m_pointInfo ? m_pointInfo->size() : 0;
    }

    [[nodiscard]] u16 getJugemPointCount() const {
        return m_jugemPoint ? m_jugemPoint->size() : 0;
    }

    [[nodiscard]] u16 getStageInfoCount() const {
        return m_stageInfo ? m_stageInfo->size() : 0;
    }

    [[nodiscard]] u16 getStartPointCount() const {
        return m_startPoint ? m_startPoint->size() : 0;
    }

    [[nodiscard]] u32 version() const {
        return m_header->version();
    }

    [[nodiscard]] MapdataCheckPathAccessor *checkPath() const {
        return m_checkPath;
    }

    [[nodiscard]] MapdataCheckPointAccessor *checkPoint() const {
        return m_checkPoint;
    }

    [[nodiscard]] f32 skewAngle() const {
        return m_skewAngle;
    }

    [[nodiscard]] f32 lateralSpacing() const {
        return m_lateralSpacing;
    }

    [[nodiscard]] f32 longitudinalSpacing() const {
        return m_longitudinalSpacing;
    }

    [[nodiscard]] f32 longitudinalOffset() const {
        return m_longitudinalOffset;
    }

    [[nodiscard]] f32 longitudinalWideOffset() const {
        return m_longitudinalWideOffset;
    }
    /// @endGetters

    /// @addr{0x80512694}
    /// @brief Creates the singleton instance of the @ref CourseMap
    /// @return A pointer to the newly created @ref CourseMap instance
    static CourseMap *CreateInstance() {
        ASSERT(!s_instance);
        s_instance = EGG::egg_new<CourseMap>();
        return s_instance;
    }

    /// @addr{0x8051271C}
    /// @brief Destroys the singleton instance of the @ref CourseMap
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

    CourseMap();
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
    void clearSectorChecked() {
        for (size_t i = 0; i < m_checkPoint->size(); ++i) {
            getCheckPoint(i)->clearSearched();
        }
    }

    MapdataFileAccessor *m_header; ///< Accessor for the header of the `course.kmp` file data
    MapdataStartPointAccessor *m_startPoint;   ///< Accessorr for the STPT section of `course.kmp`
    MapdataCheckPathAccessor *m_checkPath;     ///< Accessor for the CKPH section of `course.kmp`
    MapdataCheckPointAccessor *m_checkPoint;   ///< Accessor for the CKPT section of `course.kmp`
    MapdataPointInfoAccessor *m_pointInfo;     ///< Accessor for the POTI section of `course.kmp`
    MapdataGeoObjAccessor *m_geoObj;           ///< Accessor for the GOBJ section of `course.kmp`
    MapdataAreaAccessor *m_area;               ///< Accessor for the AREA section of `course.kmp`
    MapdataJugemPointAccessor *m_jugemPoint;   ///< Accessor for the JGPT section of `course.kmp`
    MapdataCannonPointAccessor *m_cannonPoint; ///< Accessor for the CNPT section of `course.kmp`
    MapdataStageInfoAccessor *m_stageInfo;     ///< Accessor for the STGI section of `course.kmp`

    // TODO: Better names
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

    static CourseMap *s_instance; ///< @addr{0x809BD6E8}
};

} // namespace System

} // namespace Kinoko
