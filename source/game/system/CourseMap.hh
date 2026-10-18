#pragma once

#include "game/system/map/MapdataArea.hh"

namespace Kinoko {

namespace Host {

class Context;

} // namespace Host

/// @brief High-level handling for generic system operations, such as input reading, race
/// configuration, and resource management.
namespace System {

template <typename T, typename TData>
class MapdataAccessorBase;

class MapdataCannonPointAccessor;
class MapdataCheckPathAccessor;
class MapdataCheckPointAccessor;
class MapdataFileAccessor;
class MapdataGeoObjAccessor;
class MapdataJugemPointAccessor;
class MapdataPointInfoAccessor;
class MapdataStageInfoAccessor;
class MapdataStartPointAccessor;

class MapdataCannonPoint;
class MapdataCheckPath;
class MapdataCheckPoint;
class MapdataGeoObj;
class MapdataJugemPoint;
class MapdataPointInfo;
class MapdataStageInfo;
class MapdataStartPoint;

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
    friend Host::Context;

public:
    void init();

    [[nodiscard]] s16 findSector(const EGG::Vector3f &pos, u16 checkpointIdx, f32 &distanceRatio);
    [[nodiscard]] u16 getCheckPointEntryOffsetMs(u16 i, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos) const;
    [[nodiscard]] f32 getCheckPointEntryOffsetExact(u16 i, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos) const;
    [[nodiscard]] s16 getCurrentAreaID(s16 i, const EGG::Vector3f &pos,
            MapdataAreaBase::Type type) const;

    [[nodiscard]] bool hasPlayerIdx() const;
    [[nodiscard]] bool hasRailIdx() const;

    /// @beginGetters

    [[nodiscard]] MapdataCheckPathAccessor *checkPathAccessor() const;
    [[nodiscard]] MapdataCheckPointAccessor *checkPointAccessor() const;

    [[nodiscard]] const MapdataCannonPoint *getCannonPoint(u16 i) const;
    [[nodiscard]] const MapdataCheckPath *getCheckPath(u16 i) const;
    [[nodiscard]] MapdataCheckPoint *getCheckPoint(u16 i);
    [[nodiscard]] const MapdataCheckPoint *getCheckPoint(u16 i) const;
    [[nodiscard]] const MapdataGeoObj *getGeoObj(u16 i) const;
    [[nodiscard]] const MapdataAreaBase *getArea(u16 i) const;
    [[nodiscard]] const MapdataAreaBase *getAreaSorted(u16 i) const;
    [[nodiscard]] const MapdataPointInfo *getPointInfo(u16 i) const;
    [[nodiscard]] const MapdataJugemPoint *getJugemPoint(u16 i) const;
    [[nodiscard]] const MapdataStageInfo *getStageInfo() const;
    [[nodiscard]] const MapdataStartPoint *getStartPoint(u16 i) const;

    [[nodiscard]] u16 getCannonPointCount() const;
    [[nodiscard]] u16 getCheckPathCount() const;
    [[nodiscard]] u16 getCheckPointCount() const;
    [[nodiscard]] u16 getGeoObjCount() const;
    [[nodiscard]] u16 getAreaCount() const;
    [[nodiscard]] u16 getPointInfoCount() const;
    [[nodiscard]] u16 getJugemPointCount() const;
    [[nodiscard]] u16 getStageInfoCount() const;
    [[nodiscard]] u16 getStartPointCount() const;

    [[nodiscard]] u32 version() const;
    [[nodiscard]] f32 skewAngle() const;
    [[nodiscard]] f32 lateralSpacing() const;
    [[nodiscard]] f32 longitudinalSpacing() const;
    [[nodiscard]] f32 longitudinalOffset() const;
    [[nodiscard]] f32 longitudinalWideOffset() const;

    /// @endGetters

    static CourseMap *CreateInstance();
    static void DestroyInstance();
    [[nodiscard]] static CourseMap *Instance();

private:
    EGG_NEW_DELETE_FRIEND

    CourseMap();
    ~CourseMap() override;

    void clearSectorChecked();
    [[nodiscard]] s16 findRecursiveSector(const EGG::Vector3f &pos, s16 depth,
            bool searchBackwardsFirst, MapdataCheckPoint *checkpoint, f32 &completion,
            bool playerIsForwards) const;
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

    template <MapdataDerived T>
    [[nodiscard]] T *parseMapdata(u32 sectionName) const;

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

    [[nodiscard]] static std::span<const u8> LoadFile(const char *filename);

    /// @brief Helper function which converts a big-endian ASCII string to a `u32` signature
    /// used to identify each section of the course KMP file
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

    /// @brief Revision of the KMP file format at which the rail index was introduced to the
    /// `AREA` section
    /// @details Even though all vanilla course KMP files have revision 2520, we add checks for
    /// posterity's sake.
    static constexpr u32 REVISION_RAIL_IDX = 2200;
};

} // namespace System

} // namespace Kinoko
