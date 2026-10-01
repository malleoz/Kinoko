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
    /// @brief Grants access to the singleton so a @ref Host::Context can restore the instance's
    /// state on context switch
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

        /// @beginGetters

        /// @brief Gets the base type of the tri (bits 0-4)
        /// @return The @ref KColType of the tri as a `u16`
        [[nodiscard]] u16 baseType() const {
            return attribute & 0x1F;
        }

        /// @brief Gets the variant of the tri (bits 5-7)
        /// @return The variant of the tri
        [[nodiscard]] u16 variant() const {
            return (attribute >> 5) & 7;
        }

        /// @brief Gets the intensity of the wheel "sinking" effect (bits 11-12)
        /// @return The intensity of the wheel "sinking" effect
        [[nodiscard]] u16 intensity() const {
            return (attribute >> 11) & 3;
        }

        /// @endGetters

        /// @beginSetters

        /// @brief Updates the variant bits in the collision attribute field
        /// @param variant The new variant value to set
        void setVariant(u16 variant) {
            u16 current = static_cast<u16>(attribute);
            attribute = static_cast<CollisionAttribute>((current & ~0xE0) | ((variant & 7) << 5));
        }

        /// @endSetters
    };

    /// @addr{0x8078E4F0}
    /// @brief Narrows the spatial cache of the @ref CourseColMgr and @ref ObjectDrivableDirector to
    /// only include KCL tris defined by the provided mask within a certain radius of the given
    /// position.
    /// @param radius The radius within which to narrow the spatial cache
    /// @param pos The position around which to narrow the spatial cache
    /// @param mask The KCL type mask to filter the tris
    /// @param timeOffset The time offset for the operation
    void checkCourseColNarrScLocal(f32 radius, const EGG::Vector3f &pos, KCLTypeMask mask,
            u32 timeOffset) {
        CourseColMgr::Instance()->scaledNarrowScopeLocal(1.0f, radius, nullptr, pos, mask);
        ObjectDrivableDirector::Instance()->colNarScLocal(radius, pos, mask, timeOffset);
    }

    /// @addr{0x8078F320}
    /// @brief Checks collision between a sphere and course KCL and object collision, writing
    /// partial collision info. Additionally pushes the collision entry into the @ref
    /// CollisionDirector cache.
    /// @param radius The radius of the sphere to check
    /// @param pos The position of the sphere to check
    /// @param prevPos The previous position of the sphere, used for calculating collision depth
    /// @param mask The KCL flags to check collision against (other types are ignored)
    /// @param info Out parameter for retrieving partial collision information (if any)
    /// @param maskOut The KCL flags that were hit during the collision check (if any)
    /// @param timeOffset Optional time delta
    /// @return Whether a collision was detected
    [[nodiscard]] bool checkSpherePartialPush(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
            KCLTypeMask *maskOut, u32 timeOffset) {
        return checkSphereImpl<CollisionInfoPartial>(radius, pos, prevPos, mask, info, maskOut,
                timeOffset, &CourseColMgr::checkSpherePartialPush,
                &ObjectDrivableDirector::checkSpherePartialPush, false, true);
    }

    /// @addr{0x8078F500}
    /// @brief Checks collision between a sphere and course KCL and object collision, writing out
    /// full collision info
    /// @param radius The radius of the sphere to check
    /// @param pos The position of the sphere to check
    /// @param prevPos The previous position of the sphere, used for calculating collision depth
    /// @param mask The KCL flags to check collision against (other types are ignored)
    /// @param info Out parameter for retrieving collision information (if any)
    /// @param maskOut The KCL flags that were hit during the collision check (if any)
    /// @param timeOffset Optional time delta
    /// @return Whether a collision was detected
    [[nodiscard]] bool checkSphereFull(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info,
            KCLTypeMask *maskOut, u32 timeOffset) {
        return checkSphereImpl<CollisionInfo>(radius, pos, prevPos, mask, info, maskOut, timeOffset,
                &CourseColMgr::checkSphereFull, &ObjectDrivableDirector::checkSphereFull, false,
                false);
    }

    /// @addr{0x8078F784}
    /// @brief Checks collision between a sphere and course KCL and object collision, writing out
    /// full collision info. Additionally pushes the collision entry into the @ref CollisionDirector
    /// cache.
    /// @param radius The radius of the sphere to check
    /// @param pos The position of the sphere to check
    /// @param prevPos The previous position of the sphere, used for calculating collision depth
    /// @param mask The KCL flags to check collision against (other types are ignored)
    /// @param info Out parameter for retrieving collision information (if any)
    /// @param maskOut The KCL flags that were hit during the collision check (if any)
    /// @param timeOffset Optional time delta
    /// @return Whether a collision was detected
    [[nodiscard]] bool checkSphereFullPush(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info,
            KCLTypeMask *maskOut, u32 timeOffset) {
        return checkSphereImpl<CollisionInfo>(radius, pos, prevPos, mask, info, maskOut, timeOffset,
                &CourseColMgr::checkSphereFullPush, &ObjectDrivableDirector::checkSphereFullPush,
                false, true);
    }

    /// @addr{0x807901F0}
    /// @brief Checks collision between a sphere and course KCL and object collision by using the
    /// collision director's local spatial cache, writing only partial collision info
    /// @param radius The radius of the sphere to check
    /// @param pos The position of the sphere to check
    /// @param prevPos The previous position of the sphere, used for calculating collision depth
    /// @param mask The KCL flags to check collision against (other types are ignored)
    /// @param info Out parameter for retrieving partial collision information (if any)
    /// @param maskOut The KCL flags that were hit during the collision check (if any)
    /// @param timeOffset Optional time delta
    /// @return Whether a collision was detected
    [[nodiscard]] bool checkSphereCachedPartial(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
            KCLTypeMask *maskOut, u32 timeOffset) {
        return checkSphereImpl<CollisionInfoPartial>(radius, pos, prevPos, mask, info, maskOut,
                timeOffset, &CourseColMgr::checkSphereCachedPartial,
                &ObjectDrivableDirector::checkSphereCachedPartial, true, false);
    }

    /// @addr{0x807903BC}
    /// @brief Checks collision between a sphere and course KCL and object collision by using the
    /// collision director's local spatial cache, writing partial collision info. Additionally
    /// pushes the collision entry into the @ref CollisionDirector cache.
    /// @param radius The radius of the sphere to check
    /// @param pos The position of the sphere to check
    /// @param prevPos The previous position of the sphere, used for calculating collision depth
    /// @param mask The KCL flags to check collision against (other types are ignored)
    /// @param info Out parameter for retrieving partial collision information (if any)
    /// @param maskOut The KCL flags that were hit during the collision check (if any)
    /// @param timeOffset Optional time delta
    /// @return Whether a collision was detected
    [[nodiscard]] bool checkSphereCachedPartialPush(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
            KCLTypeMask *maskOut, u32 timeOffset) {
        return checkSphereImpl<CollisionInfoPartial>(radius, pos, prevPos, mask, info, maskOut,
                timeOffset, &CourseColMgr::checkSphereCachedPartialPush,
                &ObjectDrivableDirector::checkSphereCachedPartialPush, true, true);
    }

    /// @addr{0x807907F8}
    /// @brief Checks collision between a sphere and course KCL and object collision by using the
    /// collision director's local spatial cache, writing out full collision info. Additionally
    /// pushes the collision entry into the @ref CollisionDirector cache.
    /// @param radius The radius of the sphere to check
    /// @param pos The position of the sphere to check
    /// @param prevPos The previous position of the sphere, used for calculating collision depth
    /// @param mask The KCL flags to check collision against (other types are ignored)
    /// @param info Out parameter for retrieving collision information (if any)
    /// @param maskOut The KCL flags that were hit during the collision check (if any)
    /// @param timeOffset Optional time delta
    /// @return Whether a collision was detected
    [[nodiscard]] bool checkSphereCachedFullPush(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info,
            KCLTypeMask *maskOut, u32 timeOffset) {
        return checkSphereImpl<CollisionInfo>(radius, pos, prevPos, mask, info, maskOut, timeOffset,
                &CourseColMgr::checkSphereCachedFullPush,
                &ObjectDrivableDirector::checkSphereCachedFullPush, true, true);
    }

    /// @addr{0x807BDA7C}
    /// @brief Clears the collision cache count, effectively emptying the cache
    /// @param maskOut Out parameter that will be reset to 0
    void resetCollisionEntries(KCLTypeMask *maskOut) {
        *maskOut = 0;
        m_collisionEntryCount = 0;
        m_closestCollisionEntry = nullptr;
    }

    /// @addr{0x807BDA9C}
    /// @brief Called upon finding collision that should be saved to the @ref m_entries cache
    /// temporarily
    /// @param dist Distance from player to the KCL triangle center
    /// @param typeMask Out parameter that is updated to include `kclTypeBit`
    /// @param kclTypeBit The base type of the tri we are colliding with
    /// @param attribute The attribute and additional info about the tri we are colliding with
    void pushCollisionEntry(f32 dist, KCLTypeMask *typeMask, KCLTypeMask kclTypeBit,
            CollisionAttribute attribute) {
        *typeMask = *typeMask | kclTypeBit;
        if (m_collisionEntryCount >= m_entries.size()) {
            m_collisionEntryCount = m_entries.size() - 1;
        }

        m_entries[m_collisionEntryCount++] = CollisionEntry(kclTypeBit, attribute, dist);
    }

    /// @addr{0x807BDB5C}
    /// @brief Updates the variant for the currently colliding tri
    /// @param variant The new variant to set for the currently colliding tri
    /// @details As an example, SNES Ghost Valley 2 blocks (managed by @ref ObjectObakeManager) are
    /// updated to have special wall variant 2 (bouncy wall) when colliding with the side of the
    /// block so that you bounce off of them when colliding at slow speeds.
    void setCurrentCollisionVariant(u16 variant) {
        ASSERT(m_collisionEntryCount > 0);
        m_entries[m_collisionEntryCount - 1].setVariant(variant);
    }

    /// @addr{0x807BDBC4}
    /// @brief Toggles whether the currently colliding tri is trickable
    /// @param trickable Whether the currently colliding tri should be trickable
    /// @details This is used by dynamic objects to determine whether the player can trick. For
    /// example, the geyser mounds at the end of Bowser's Castle (@ref ObjectFlamePoleFoot) are only
    /// trickable if their scale is 2x or larger.
    void setCurrentCollisionTrickable(bool trickable) {
        ASSERT(m_collisionEntryCount > 0);
        CollisionEntry &entry = m_entries[m_collisionEntryCount - 1];
        entry.attribute.changeBit(trickable, eCollisionAttribute::Trickable);
    }

    bool findClosestCollisionEntry(KCLTypeMask *typeMask, KCLTypeMask type);

    /// @beginGetters

    /// @brief Gets a pointer to the closest cached collision information
    /// @return A const pointer to the closest cached collision information, or `nullptr` if there
    /// is none.
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

    /// @brief Gets the singleton instance of the @ref CollisionDirector
    /// @return A pointer to the singleton instance of the @ref CollisionDirector
    [[nodiscard]] static CollisionDirector *Instance() {
        return s_instance;
    }

private:
    EGG_NEW_DELETE_FRIEND

    /// @addr{0x8078E33C}
    /// @brief Private constructor
    /// @details Initializes @ref m_collisionEntryCount to zero and @ref m_closestCollisionEntry to
    /// `nullptr`. Finally, creates and initializes the @ref CourseColMgr singleton.
    CollisionDirector() {
        m_collisionEntryCount = 0;
        m_closestCollisionEntry = nullptr;
        CourseColMgr::CreateInstance()->init();
    }

    /// @addr{0x8078E454}
    /// @brief Private virtual destructor that also destroys the @ref CourseColMgr singleton
    ~CollisionDirector() override {
        if (s_instance) {
            s_instance = nullptr;
            WARN("CollisionDirector instance not explicitly handled!");
        }

        CourseColMgr::DestroyInstance();
    }

    /// @brief Function pointer type for a @ref CourseColMgr sphere collision check
    /// @tparam T The collision info object type, either @ref CollisionInfo or @ref
    /// CollisionInfoPartial
    template <typename T>
    using CourseColCheckFunc = bool (CourseColMgr::*)(f32 scale, f32 radius, KColData *data,
            const EGG::Vector3f &pos, const EGG::Vector3f &prevPos, KCLTypeMask mask, T *info,
            KCLTypeMask *maskOut);

    /// @brief Function pointer type for an @ref ObjectDrivableDirector sphere collision check
    /// @tparam T The collision info object type, either @ref CollisionInfo or @ref
    /// CollisionInfoPartial
    template <typename T>
    using DrivableColCheckFunc = bool (ObjectDrivableDirector::*)(f32 radius,
            const EGG::Vector3f &pos, const EGG::Vector3f &prevPos, KCLTypeMask mask, T *info,
            KCLTypeMask *maskOut, u32 timeOffset);

    template <typename T>
        requires std::is_same_v<T, CollisionInfo> || std::is_same_v<T, CollisionInfoPartial>
    [[nodiscard]] bool checkSphereImpl(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, T *info, KCLTypeMask *maskOut,
            u32 timeOffset, CourseColCheckFunc<T> courseCheckFunc,
            DrivableColCheckFunc<T> drivableCheckFunc, bool cached, bool push);

    /// @brief The maximum number of collision entries that can be cached
    static constexpr size_t COLLISION_ARR_LENGTH = 0x40;

    const CollisionEntry *m_closestCollisionEntry; ///< Pointer to the closest colliding tri
    std::array<CollisionEntry, COLLISION_ARR_LENGTH> m_entries; ///< Array of all colliding tris
    size_t m_collisionEntryCount;                               ///< Number of valid colliding tris

    /// @addr{0x809C2F44}
    /// @brief The pointer to the singleton instance of the class
    static CollisionDirector *s_instance;
};

} // namespace Field

} // namespace Kinoko
