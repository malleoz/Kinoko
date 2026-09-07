#pragma once

#include "game/system/map/MapdataAccessorBase.hh"

#include <egg/math/Vector.hh>

namespace Kinoko::System {

class MapdataPointInfo;

/// @brief Base class that describes an area of the course
class MapdataAreaBase {
public:
    /// @brief The shape of the area
    enum class Shape : s8 {
        Box = 0,      ///< Box-shaped area
        Cylinder = 1, ///< Cylinder-shaped area
    };

    /// @brief The type of the area
    enum class Type : s8 {
        MovingRoad = 3, ///< Moving road area
    };

    /// @brief Raw data structure for an area of the course
    struct SData {
        s8 shape;               ///< Corresponds to @ref Shape
        s8 type;                ///< Corresponds to @ref Type
        u8 _02[0x03 - 0x02];    ///< Unused in Kinoko
        u8 priority;            ///< The priority of the area
        EGG::Vector3f position; ///< Position of the area
        EGG::Vector3f rotation; ///< Rotation of the area
        EGG::Vector3f scale;    ///< Scale of the area
        s16 parameters[2];      ///< Setting parameters
        // Pre Revision 2200: End of structure
        s8 railId;           ///< Rail ID associated with the area
        u8 _2d[0x30 - 0x2d]; ///< Unused in Kinoko
    };
    STATIC_ASSERT(sizeof(SData) == 0x30);

    MapdataAreaBase(const SData *data, s16 index);

    /// @brief Default destructor
    virtual ~MapdataAreaBase() = default;

    void read(EGG::Stream &stream);

    /// @brief Virtual function that tests whether the provided position lies within the area
    /// @param pos The position to check
    /// @return True if the position lies within the area, false otherwise
    [[nodiscard]] virtual bool testImpl(const EGG::Vector3f &pos) const = 0;

    /// @addr{0x805160B0}
    /// @brief Tests whether the provided position lies within the area
    /// @param pos The position to check
    /// @return True if the position lies within the area, false otherwise
    /// @details Performs a preliminary check using the bounding sphere before invoking the more
    /// detailed test implementation.
    [[nodiscard]] bool test(const EGG::Vector3f &pos) const {
        return (m_position - pos).squaredLength() > m_boundingRadiusSq ? false : testImpl(pos);
    }

    /// @beginGetters
    [[nodiscard]] MapdataPointInfo *getPointInfo() const;

    [[nodiscard]] Type type() const {
        return m_type;
    }

    [[nodiscard]] u8 priority() const {
        return m_priority;
    }

    /// @brief Fetches the area settings
    /// @param i The index of the setting to fetch
    /// @return The value of the specified setting
    [[nodiscard]] s16 param(size_t i) const {
        ASSERT(i < m_params.size());
        return m_params[i];
    }

    [[nodiscard]] s16 index() const {
        return m_index;
    }
    /// @endGetters

protected:
    [[maybe_unused]] const SData *const m_rawData; ///< Pointer to the raw area data
    Type m_type;                                   ///< Type of the area
    u8 m_priority;                                 ///< Priority of the area
    EGG::Vector3f m_position;                      ///< Position of the area
    EGG::Vector3f m_rotation;                      ///< Rotation of the area in degrees
    EGG::Vector3f m_scale;                         ///< Scale of the area
    std::array<s16, 2> m_params;                   ///< Area settings parameters
    s8 m_railId; ///< @ref Field::Rail associated with the area (or -1 if no rail)

    EGG::Vector3f m_right;      ///< Right direction vector of the area
    EGG::Vector3f m_up;         ///< Up direction vector of the area
    EGG::Vector3f m_forward;    ///< Forward direction vector of the area
    EGG::Vector3f m_dimensions; ///< Dimensions of the area
    f32 m_ellipseRadiusSq;      ///< Square of the x-axis dimension (used for cylinders only)
    f32 m_ellipseRatio;     ///< Ratio between the x and z-axis dimensions (used for cylinders only)
    f32 m_boundingRadiusSq; ///< Square of the bounding sphere containing the entire area shape
    const s16 m_index;      ///< Index of the entry in the AREA section of the KMP
};

/// @brief Represents a box-shaped area in the map
class MapdataAreaBox final : public MapdataAreaBase {
public:
    MapdataAreaBox(const SData *data, s16 index);
    ~MapdataAreaBox() = default;

    [[nodiscard]] bool testImpl(const EGG::Vector3f &pos) const override;
};

/// @brief Represents a cylinder-shaped area in the map
class MapdataAreaCylinder final : public MapdataAreaBase {
public:
    MapdataAreaCylinder(const SData *data, s16 index);
    ~MapdataAreaCylinder() = default;

    [[nodiscard]] bool testImpl(const EGG::Vector3f &pos) const override;
};

/// @brief Provides access to entries in the AREA section of the course KMP
class MapdataAreaAccessor final
    : public MapdataAccessorBase<MapdataAreaBase, MapdataAreaBase::SData> {
public:
    MapdataAreaAccessor(const MapSectionHeader *header);
    ~MapdataAreaAccessor() override;

    void init(const MapdataAreaBase::SData *start, u16 count);
    void sort();

    /// @brief Returns the sorted entry at the specified index
    /// @param i The index of the sorted entry to retrieve
    /// @return A pointer to the sorted entry at the specified index, or nullptr if the index is out
    /// of bounds
    [[nodiscard]] MapdataAreaBase *getSorted(u16 i) const {
        ASSERT(!m_sortedEntries.empty());
        return i < m_sortedEntries.size() ? m_sortedEntries[i] : nullptr;
    }

private:
    /// @brief Array of sorted entries in the AREA (higher priority first)
    owning_span<MapdataAreaBase *> m_sortedEntries;
};

} // namespace Kinoko::System
