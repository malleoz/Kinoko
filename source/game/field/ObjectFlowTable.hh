#pragma once

#include "game/field/obj/ObjectId.hh"

#include "game/system/ResourceManager.hh"

#include <cstring>

namespace Kinoko::Field {

/// @brief Maps to SObjectCollisionSet::mode. Determines what type of collision an object has.
/// @note In the base game, the @ref ObjectDirector will construct @ref ObjectKCL objects for course
/// objects having a collision mode of @ref CollisionMode::Ground. However, since @ref
/// ObjectDirector::createObject() is implemented in Kinoko as a switch statement, we add an
/// explicit case for the one object that has this collision mode: WLDokanGC (the giant pipes on GCN
/// Waluigi Stadium).
enum class CollisionMode {
    None = 0,     ///< The object does not have any collision
    Sphere = 1,   ///< Maps to @ref ObjectCollisionSphere
    Cylinder = 2, ///< Maps to @ref ObjectCollisionCylinder
    Box = 3,      ///< Maps to @ref ObjectCollisionBox
    Ground = 4,   ///< Primitive KCL object
    Original = 5, ///< Unused
};

/// @brief Structure of the ObjFlow.bin table entries
/// @details Defines the associated @ref ObjectId, the name of the object, the name of the resource
/// file(s) to load for this object, and the collision mode and parameters.
struct SObjectCollisionSet {
    u16 id;              ///< Maps to the @ref ObjectId of the object
    char name[32];       ///< The name of the object, used to fetch the associated @ref ObjectId
    char resources[64];  ///< The name of the resource file(s) to load for this object
    u8 _62[0x6a - 0x62]; ///< Omitted for the purposes of Kinoko
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
    u8 _72[0x74 - 0x72]; ///< Omitted for the purposes of Kinoko
};
STATIC_ASSERT(sizeof(SObjectCollisionSet) == 0x74);

/// @brief Parses the ObjFlow.bin table which maps an @ref ObjectId to the associated collision
/// parameters and resource file(s)
/// @details This file contains a header and two data sections. The header contains the number of
/// objects stored in the table. The first data section is an array of @ref SObjectCollisionSet.
/// Following this section is a lookup table which maps an ObjectId to the index of the
/// corresponding @ref SObjectCollisionSet in the first data section.
class ObjectFlowTable {
public:
    /// @addr{0x8082C10C}
    /// @brief Obtains a pointer to the provided filename (ObjFlow.bin), parses the count, and
    /// obtains pointers to the two data sections
    ObjectFlowTable(const char *filename) {
        SFile *file = reinterpret_cast<SFile *>(System::ResourceManager::Instance()->getFile(
                filename, nullptr, System::ArchiveId::Core));

        m_count = parse<s16>(file->count);
        m_sets = file->sets;
        m_slots = reinterpret_cast<const s16 *>(m_sets + m_count);
    }

    /// @addr{0x8082C1F4}
    ~ObjectFlowTable() = default;

    /// @brief Returns a pointer to the @ref SObjectCollisionSet at the provided index
    /// @param slot Index of the @ref SObjectCollisionSet to retrieve
    /// @return Pointer to the @ref SObjectCollisionSet, or nullptr if the index is invalid
    [[nodiscard]] const SObjectCollisionSet *set(s16 slot) const {
        return slot == -1 ? nullptr : slot < m_count ? &m_sets[slot] : nullptr;
    }

    /// @brief Gets the index of the @ref SObjectCollisionSet for the provided @ref ObjectId
    /// @param id The @ref ObjectId to get the index for
    /// @return The index of the @ref SObjectCollisionSet, or -1 if the @ref ObjectId is invalid
    [[nodiscard]] s16 slot(ObjectId id) const {
        constexpr size_t SLOT_COUNT = 0x2f4;

        size_t i = static_cast<size_t>(id);
        return i < SLOT_COUNT ? parse<s16>(m_slots[i]) : -1;
    }

    /// @addr{0x8082C178}
    /// @brief Iterates the @ref SObjectCollisionSet entries and returns the @ref ObjectId of the
    /// entry with a matching name
    [[nodiscard]] ObjectId getIdFromName(const char *name) const {
        for (s16 i = 0; i < m_count; ++i) {
            const auto *curSet = set(i);
            ASSERT(curSet);

            if (strncmp(name, curSet->name, sizeof(curSet->name)) == 0) {
                return static_cast<ObjectId>(parse<u16>(curSet->id));
            }
        }

        return ObjectId::None;
    }

private:
    /// @brief Represents the header and beginning of the first data section of ObjFlow.bin
    struct SFile {
        s16 count; ///< The number of @ref SObjectCollisionSet entries in the first data section
        SObjectCollisionSet sets[]; ///< The first data section
    };

    /// @brief Endian-corrected number of @ref SObjectCollisionSet entries in the first data section
    s16 m_count;

    /// @brief Pointer to the start of the first data section of ObjFlow.bin, which is an array of
    /// @ref SObjectCollisionSet
    const SObjectCollisionSet *m_sets;

    /// @brief Pointer to the start of the second data section of ObjFlow.bin, which is a lookup
    /// table mapping @ref ObjectId to the index of the corresponding @ref SObjectCollisionSet
    const s16 *m_slots;
};

} // namespace Kinoko::Field
