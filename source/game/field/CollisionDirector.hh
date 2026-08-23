#pragma once

#include "game/field/ObjectDrivableDirector.hh"

namespace Kinoko {

namespace Host {

class Context;

} // namespace Host

/// @brief Pertains to collision.
namespace Field {

/// @brief Manages the caching of colliding KCL triangles and exposes queries for collision checks
/// @details Stores up to 64 cached collision entries which can be used by classes like @ref
/// Kart::KartCollide to fetch tris without having to perform another collision query. Exposes
/// public interfaces for collision checks, including full vs. partial checks (whether resulting
/// collisions return just a subset of collision information), cached vs. uncached checks (whether
/// we can leverage the cache), and push variants (whether we want to add collision entries to the
/// cache). Also exposes functionality to find the closest colliding entry that matches a provided
/// @ref KCLTypeMask.
class CollisionDirector : EGG::Disposer {
    friend class Host::Context;

public:
    /// @brief Collision Entry Attribute fields
    /// @details Each KCL triangle is associated with a 16-bit attribute field which determines the
    /// effect of that triangle on the player that is colliding with it.\n\n
    /// Bit layout:
    /// | Bits 0-4 | Bits 5-7 | Bits 8-10 | Bits 11-12 | Bit 13    | Bit 14      | Bit 15        |
    /// |----------|----------|-----------|------------|-----------|-------------|---------------|
    /// | @ref KColType | Variant  | (unused)  | Intensity  | Trickable | Reject road | Soft surface
    /// |
    /// - **@ref KColType**: (0-31) Represents the main category of effect (road, offroad, etc.)
    /// - **Variant**: (0-7) Represents variations in behavior that are specific to the base type
    /// - **Intensity**: (0-3) Defines how deep vehicle wheels "sink" into the road
    /// - **Trickable**: (0/1) Enables tricking on this surface
    /// - **Reject Road**: (0/1) The surface will try to redirect the player's direction
    /// - **Soft Surface**: (0/1) A barrel roll wall, used to prevent players from hanging on ledges
    enum class eCollisionAttribute {
        Trickable = 13,  ///< Whether the player can perform a trick on this surface
        RejectRoad = 14, ///< A surface that tries to push you back towards the road
        Soft = 15,       ///< Barrel roll walls
    };

    /// @brief A bitfield of @ref eCollisionAttribute that represents the attributes field of a
    /// colliding KCL triangle
    typedef EGG::TBitFlag<u16, eCollisionAttribute> CollisionAttribute;

    /// @brief Represnts traits of a colliding KCL triangle
    /// @todo We should be able to get rid of typeMask as it is never accessed directly and can
    /// instead be identically retrieved via baseType() (it is always in parity)
    struct CollisionEntry {
        KCLTypeMask typeMask;         ///< The base type
        CollisionAttribute attribute; ///< The full 16-bit attribute field
        f32 dist;                     ///< Distance between the tri and the colliding body

        /// @brief Returns the base type of the tri (bits 0-4)
        [[nodiscard]] u16 baseType() const {
            return attribute & 0x1F;
        }

        /// @brief Returns the variant of the tri (bits )
        [[nodiscard]] u16 variant() const {
            return (attribute >> 5) & 7;
        }

        /// @brief Returns the intensity of the wheel "sinking" effect
        [[nodiscard]] u16 intensity() const {
            return (attribute >> 11) & 3;
        }

        /// @brief Updates the variant bits in the collision attribute field
        void setVariant(u16 variant) {
            u16 current = static_cast<u16>(attribute);
            attribute = static_cast<CollisionAttribute>((current & ~0xE0) | ((variant & 7) << 5));
        }
    };

    /// @addr{0x8078E4F0}
    /// @brief Narrows the spatial cache of the @ref CourseColMgr and @ref ObjectDrivableDirector to
    /// only include KCL tris defined by the provided mask within a certain radius of the given
    /// position.
    void checkCourseColNarrScLocal(f32 radius, const EGG::Vector3f &pos, KCLTypeMask mask,
            u32 timeOffset) {
        CourseColMgr::Instance()->scaledNarrowScopeLocal(1.0f, radius, nullptr, pos, mask);
        ObjectDrivableDirector::Instance()->colNarScLocal(radius, pos, mask, timeOffset);
    }

    [[nodiscard]] bool checkSpherePartialPush(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask flags, CollisionInfoPartial *info,
            KCLTypeMask *typeMaskOut, u32 timeOffset);
    [[nodiscard]] bool checkSphereFull(f32 radius, const EGG::Vector3f &v0, const EGG::Vector3f &v1,
            KCLTypeMask flags, CollisionInfo *pInfo, KCLTypeMask *pFlagsOut, u32 timeOffset);
    [[nodiscard]] bool checkSphereFullPush(f32 radius, const EGG::Vector3f &v0,
            const EGG::Vector3f &v1, KCLTypeMask flags, CollisionInfo *pInfo,
            KCLTypeMask *pFlagsOut, u32 timeOffset);

    [[nodiscard]] bool checkSphereCachedPartial(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask typeMask, CollisionInfoPartial *info,
            KCLTypeMask *typeMaskOut, u32 timeOffset);
    [[nodiscard]] bool checkSphereCachedPartialPush(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask typeMask, CollisionInfoPartial *info,
            KCLTypeMask *typeMaskOut, u32 timeOffset);
    [[nodiscard]] bool checkSphereCachedFullPush(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask typeMask, CollisionInfo *info,
            KCLTypeMask *typeMaskOut, u32 timeOffset);

    /// @addr{0x807BDA7C}
    /// @brief Clears the collision cache count, effectively emptying the cache
    void resetCollisionEntries(KCLTypeMask *ptr) {
        *ptr = 0;
        m_collisionEntryCount = 0;
        m_closestCollisionEntry = nullptr;
    }

    void pushCollisionEntry(f32 dist, KCLTypeMask *typeMask, KCLTypeMask kclTypeBit,
            CollisionAttribute attribute);

    /// @addr{0x807BDB5C}
    /// @brief Updates the variant for the currently colliding tri
    /// @details As an example, SNES Ghost Valley 2 blocks (managed by @ref ObjectObakeManager) are
    /// updated to have special wall variant 2 (bouncy wall) when colliding with the side of the
    /// block so that you bounce off of them when colliding at slow speeds.
    void setCurrentCollisionVariant(u16 attribute) {
        ASSERT(m_collisionEntryCount > 0);
        m_entries[m_collisionEntryCount - 1].setVariant(attribute);
    }

    /// @addr{0x807BDBC4}
    /// @brief Designates that the currently colliding tri is now trickable
    /// @details This is used by dynamic objects to determine whether the player can trick. For
    /// example, the geysers at the end of Bowser's Castle (represented with @ref
    /// ObjectFlamePoleFoot) are only trickable if their scale is 2x or larger.
    void setCurrentCollisionTrickable(bool trickable) {
        ASSERT(m_collisionEntryCount > 0);
        CollisionEntry &entry = m_entries[m_collisionEntryCount - 1];
        entry.attribute.changeBit(trickable, eCollisionAttribute::Trickable);
    }

    bool findClosestCollisionEntry(KCLTypeMask *typeMask, KCLTypeMask type);

    /// @beginGetters
    [[nodiscard]] const CollisionEntry *closestCollisionEntry() const {
        return m_closestCollisionEntry;
    }
    /// @endGetters

    /// @addr{0x8078DFE8}
    /// @brief Creates the singleton instance of the @ref CollisionDirector
    /// @return A pointer to the newly created @ref CollisionDirector instance
    static CollisionDirector *CreateInstance() {
        ASSERT(!s_instance);
        s_instance = EGG::egg_new<CollisionDirector>();
        return s_instance;
    }

    /// @addr{0x8078E124}
    /// @brief Destroys the singleton instance of the @ref CollisionDirector
    static void DestroyInstance() {
        ASSERT(s_instance);
        auto *instance = s_instance;
        s_instance = nullptr;
        EGG::egg_delete(instance);
    }

    /// @brief Returns the singleton instance of the @ref CollisionDirector
    /// @return A pointer to the singleton instance of the @ref CollisionDirector
    [[nodiscard]] static CollisionDirector *Instance() {
        return s_instance;
    }

private:
    EGG_NEW_DELETE_FRIEND

    CollisionDirector();
    ~CollisionDirector() override;

    /// @brief The maximum number of collision entries that can be cached
    static constexpr size_t COLLISION_ARR_LENGTH = 0x40;

    const CollisionEntry *m_closestCollisionEntry; ///< Pointer to the closest colliding tri
    std::array<CollisionEntry, COLLISION_ARR_LENGTH> m_entries; ///< Array of all colliding tris
    size_t m_collisionEntryCount;                               ///< Number of valid colliding tris

    static CollisionDirector *s_instance; ///< @addr{0x809C2F44}
};

} // namespace Field

} // namespace Kinoko
