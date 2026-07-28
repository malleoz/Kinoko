#pragma once

#include "game/field/CourseColMgr.hh"

#include <egg/math/Vector.hh>

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
    /// | BaseType | Variant  | (unused)  | Intensity  | Trickable | Reject road | Soft surface  |
    enum class eCollisionAttribute {
        Trickable = 13,  ///< Whether the player can perform a trick on this surface
        RejectRoad = 14, ///< A surface that tries to push you back towards the road
        Soft = 15,
    };
    typedef EGG::TBitFlag<u16, eCollisionAttribute> CollisionAttribute;

    struct CollisionEntry {
        KCLTypeMask typeMask;
        CollisionAttribute attribute;
        f32 dist;

        u16 baseType() const {
            return attribute & 0x1F;
        }

        u16 variant() const {
            return (attribute >> 5) & 7;
        }

        u16 intensity() const {
            return (attribute >> 11) & 3;
        }

        void setVariant(u16 variant) {
            u16 current = static_cast<u16>(attribute);
            attribute = static_cast<CollisionAttribute>((current & ~0xE0) | ((variant & 7) << 5));
        }
    };

    void checkCourseColNarrScLocal(f32 radius, const EGG::Vector3f &pos, KCLTypeMask mask,
            u32 timeOffset);

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

    void resetCollisionEntries(KCLTypeMask *ptr);
    void pushCollisionEntry(f32 dist, KCLTypeMask *typeMask, KCLTypeMask kclTypeBit,
            CollisionAttribute attribute);

    /// @addr{0x807BDB5C}
    void setCurrentCollisionVariant(u16 attribute) {
        ASSERT(m_collisionEntryCount > 0);
        m_entries[m_collisionEntryCount - 1].setVariant(attribute);
    }

    /// @addr{0x807BDBC4}
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

    static CollisionDirector *CreateInstance();
    static void DestroyInstance();

    [[nodiscard]] static CollisionDirector *Instance() {
        return s_instance;
    }

private:
    EGG_NEW_DELETE_FRIEND

    CollisionDirector();
    ~CollisionDirector() override;

    static constexpr size_t COLLISION_ARR_LENGTH = 0x40;

    const CollisionEntry *m_closestCollisionEntry;
    std::array<CollisionEntry, COLLISION_ARR_LENGTH> m_entries;
    size_t m_collisionEntryCount;

    static CollisionDirector *s_instance; ///< @addr{0x809C2F44}
};

} // namespace Field

} // namespace Kinoko
