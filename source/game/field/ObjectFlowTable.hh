#pragma once

#include "game/field/obj/ObjectId.hh"

namespace Kinoko::Field {

/// @brief Maps to @ref SObjectCollisionSet::mode. Determines what type of collision an object has.
/// @note In the base game, the @ref ObjectDirector will construct @ref ObjectKCL objects for course
/// objects having a collision mode of @ref CollisionMode::Ground. However, since @ref
/// ObjectDirector::createObject() is implemented in Kinoko as a switch statement, we add an
/// explicit case for the one object that has this collision mode: @ref ObjectId::WLDokanGC (the
/// giant pipes on GCN Waluigi Stadium).
enum class CollisionMode {
    None = 0,     ///< The object does not have any collision
    Sphere = 1,   ///< Maps to @ref ObjectCollisionSphere
    Cylinder = 2, ///< Maps to @ref ObjectCollisionCylinder
    Box = 3,      ///< Maps to @ref ObjectCollisionBox
    Ground = 4,   ///< Primitive KCL object
    Original = 5, ///< Unused
};

/// @brief Structure of the `ObjFlow.bin` table entries
/// @details Defines the associated @ref ObjectId, the name of the object, the name of the resource
/// file(s) to load for this object, and the collision mode and parameters.
struct SObjectCollisionSet {
    u16 id;              ///< Maps to the @ref ObjectId of the object
    char name[32];       ///< The name of the object, used to fetch the associated @ref ObjectId
    char resources[64];  ///< The name of the resource file(s) to load for this object
    u8 _62[0x6a - 0x62]; ///< Unused in Kinoko
    s16 mode;            ///< The collision mode of the object, maps to @ref CollisionMode

    /// @brief Collision parameters, interpreted based on @ref mode
    union {
        /// @brief Valid when @ref mode is @ref CollisionMode::Sphere
        struct {
            s16 radius; ///< The radius of the sphere
        } sphere;
        /// @brief Valid when @ref mode is @ref CollisionMode::Cylinder
        struct {
            s16 radius; ///< The radius of the cylinder
            s16 height; ///< The height of the cylinder
        } cylinder;
        /// @brief Valid when @ref mode is @ref CollisionMode::Box
        struct {
            s16 x; ///< The half-width of the box along the x-axis
            s16 y; ///< The half-width of the box along the y-axis
            s16 z; ///< The half-width of the box along the z-axis
        } box;
    } params;
    u8 _72[0x74 - 0x72]; ///< Unused in Kinoko
};
STATIC_ASSERT(sizeof(SObjectCollisionSet) == 0x74);

/// @brief Parses the `ObjFlow.bin` table which maps an @ref ObjectId to the associated collision
/// parameters and resource file(s)
/// @details This file contains a header and two data sections. The header contains the number of
/// objects stored in the table. The first data section is an array of @ref SObjectCollisionSet.
/// The second section is a lookup table which maps an ObjectId to the index of the corresponding
/// @ref SObjectCollisionSet in the first data section.
class ObjectFlowTable {
public:
    ObjectFlowTable(const char *filename);
    ~ObjectFlowTable();

    [[nodiscard]] const SObjectCollisionSet *set(s16 slot) const;
    [[nodiscard]] s16 slot(ObjectId id) const;
    [[nodiscard]] ObjectId getIdFromName(const char *name) const;

private:
    /// @brief Represents the header and beginning of the first data section of `ObjFlow.bin`
    struct SFile {
        s16 count; ///< The number of @ref SObjectCollisionSet entries in the first data section
        SObjectCollisionSet sets[]; ///< The first data section
    };

    /// @brief Endian-corrected number of @ref SObjectCollisionSet entries in the first data section
    s16 m_count;

    /// @brief Pointer to the start of the first data section of `ObjFlow.bin`, which is an array of
    /// @ref SObjectCollisionSet
    const SObjectCollisionSet *m_sets;

    /// @brief Pointer to the start of the second data section of `ObjFlow.bin`, which is a lookup
    /// table mapping @ref ObjectId to the index of the corresponding @ref SObjectCollisionSet
    const s16 *m_slots;
};

} // namespace Kinoko::Field
