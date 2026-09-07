#pragma once

#include "game/field/ObjectCollisionBox.hh"
#include "game/field/ObjectCollisionSphere.hh"
#include "game/field/obj/ObjectDrivable.hh"
#include "game/field/obj/ObjectObakeBlock.hh"

namespace Kinoko::Field {

/// @brief The manager class for SNES Ghost Valley 2 blocks
/// @details Blocks are constructed via this manager. When blocks are added, they're placed in a
/// spatially-indexed cache, which reduces the number of collision checks the game would have to
/// perform if blocks were managed like normal objects. When a collision check occurs, the manager
/// uses the cache to find the blocks that require collision checks.
class ObjectObakeManager final : public ObjectDrivable {
public:
    ObjectObakeManager(const System::MapdataGeoObj &params);
    ~ObjectObakeManager() override;

    void calc() override;

    /// @addr{0x8080BE9C}
    /// @copybrief ObjectBase::loadFlags()
    /// @return Returns @ref eLoadFlags::Calc and @ref eLoadFlags::Draw so that the object is
    /// calculated every frame
    /// @copybrief ObjectBase::loadFlags()
    /// @return Returns @ref eLoadFlags::Calc and @ref eLoadFlags::Draw so that the object is
    /// calculated every frame
    [[nodiscard]] LoadFlags loadFlags() const override {
        return LoadFlags().setBit(eLoadFlags::Calc, eLoadFlags::Draw);
    }

    /// @addr{0x8080BE98}
    /// @copybrief ObjectBase::createCollision()
    /// @details This is a no-op because the manager itself does not have any collision
    void createCollision() override {}

    /// @addr{0x8080BE94}
    /// @copybrief ObjectBase::calcCollisionTransform()
    /// @details This is a no-op because the manager itself does not have any collision
    void calcCollisionTransform() override {}

    /// @addr{0x8080BE84}
    /// @details Uses a large radius to ensure the manager is always considered for collision checks
    [[nodiscard]] f32 getCollisionRadius() const override {
        return 100000.0f;
    }

    /// @addr{0x8080BE44}
    [[nodiscard]] bool checkPointPartial(const EGG::Vector3f &pos, const EGG::Vector3f &prevPos,
            KCLTypeMask mask, CollisionInfoPartial *info, KCLTypeMask *maskOut) override {
        return checkSpherePartialImpl(0.0f, pos, prevPos, mask, info, maskOut);
    }

    /// @addr{0x8080BE54}
    [[nodiscard]] bool checkPointPartialPush(const EGG::Vector3f &pos, const EGG::Vector3f &prevPos,
            KCLTypeMask mask, CollisionInfoPartial *info, KCLTypeMask *maskOut) override {
        return checkSpherePartialPushImpl(0.0f, pos, prevPos, mask, info, maskOut);
    }

    /// @addr{0x8080BE64}
    [[nodiscard]] bool checkPointFull(const EGG::Vector3f &pos, const EGG::Vector3f &prevPos,
            KCLTypeMask mask, CollisionInfo *info, KCLTypeMask *maskOut) override {
        return checkSphereFullImpl(0.0f, pos, prevPos, mask, info, maskOut);
    }

    /// @addr{0x8080BE74}
    [[nodiscard]] bool checkPointFullPush(const EGG::Vector3f &pos, const EGG::Vector3f &prevPos,
            KCLTypeMask mask, CollisionInfo *info, KCLTypeMask *maskOut) override {
        return checkSphereFullPushImpl(0.0f, pos, prevPos, mask, info, maskOut);
    }

    /// @addr{0x8080BE34}
    [[nodiscard]] bool checkSpherePartial(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
            KCLTypeMask *maskOut, u32 /*timeOffset*/) override {
        return checkSpherePartialImpl(radius, pos, prevPos, mask, info, maskOut);
    }

    /// @addr{0x8080BE38}
    [[nodiscard]] bool checkSpherePartialPush(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
            KCLTypeMask *maskOut, u32 /*timeOffset*/) override {
        return checkSpherePartialPushImpl(radius, pos, prevPos, mask, info, maskOut);
    }

    /// @addr{0x8080BE3C}
    [[nodiscard]] bool checkSphereFull(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info,
            KCLTypeMask *maskOut, u32 /*timeOffset*/) override {
        return checkSphereFullImpl(radius, pos, prevPos, mask, info, maskOut);
    }

    /// @addr{0x8080BE40}
    [[nodiscard]] bool checkSphereFullPush(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info,
            KCLTypeMask *maskOut, u32 /*timeOffset*/) override {
        return checkSphereFullPushImpl(radius, pos, prevPos, mask, info, maskOut);
    }

    /// @addr{0x8080BDF4}
    [[nodiscard]] bool checkPointCachedPartial(const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
            KCLTypeMask *maskOut) override {
        return checkSpherePartialImpl(0.0f, pos, prevPos, mask, info, maskOut);
    }

    /// @addr{0x8080BE04}
    [[nodiscard]] bool checkPointCachedPartialPush(const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
            KCLTypeMask *maskOut) override {
        return checkSpherePartialPushImpl(0.0f, pos, prevPos, mask, info, maskOut);
    }

    /// @addr{0x8080BE14}
    [[nodiscard]] bool checkPointCachedFull(const EGG::Vector3f &pos, const EGG::Vector3f &prevPos,
            KCLTypeMask mask, CollisionInfo *info, KCLTypeMask *maskOut) override {
        return checkSphereFullImpl(0.0f, pos, prevPos, mask, info, maskOut);
    }

    /// @addr{0x8080BE24}
    [[nodiscard]] bool checkPointCachedFullPush(const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info,
            KCLTypeMask *maskOut) override {
        return checkSphereFullPushImpl(0.0f, pos, prevPos, mask, info, maskOut);
    }

    /// @addr{0x8080BDE4}
    [[nodiscard]] bool checkSphereCachedPartial(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
            KCLTypeMask *maskOut, u32 /*timeOffset*/) override {
        return checkSpherePartialImpl(radius, pos, prevPos, mask, info, maskOut);
    }

    /// @addr{0x8080BDE8}
    [[nodiscard]] bool checkSphereCachedPartialPush(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
            KCLTypeMask *maskOut, u32 /*timeOffset*/) override {
        return checkSpherePartialPushImpl(radius, pos, prevPos, mask, info, maskOut);
    }

    /// @addr{0x8080BDEC}
    [[nodiscard]] bool checkSphereCachedFull(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info,
            KCLTypeMask *maskOut, u32 /*timeOffset*/) override {
        return checkSphereFullImpl(radius, pos, prevPos, mask, info, maskOut);
    }

    /// @addr{0x8080BDF0}
    [[nodiscard]] bool checkSphereCachedFullPush(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info,
            KCLTypeMask *maskOut, u32 /*timeOffset*/) override {
        return checkSphereFullPushImpl(radius, pos, prevPos, mask, info, maskOut);
    }

    /// @addr{0x8080B244}
    /// @brief Public interface that adds a new block to the manager and caches it for collision
    /// checks
    void addBlock(const System::MapdataGeoObj &params) {
        auto *block = EGG::egg_new<ObjectObakeBlock>(params);
        m_blocks.push_back(block);
        auto [spatialX, spatialZ] = SpatialIndex(block->pos());
        m_blockCache[spatialZ][spatialX] = block;
    }

private:
    static constexpr size_t CACHE_SIZE_X = 122;        ///< Width of the spatial cache
    static constexpr size_t CACHE_SIZE_Z = 116;        ///< Depth of the spatial cache
    static constexpr f32 WALL_BOUNDING_RADIUS = 85.0f; ///< Radius of a block's wall collision

    /// @brief Scale of a block's wall collision
    static constexpr EGG::Vector3f WALL_SCALE = EGG::Vector3f(1.0f, 1.1f, 1.0f);

    /// @brief Scale of a block's road collision
    static constexpr EGG::Vector3f ROAD_SCALE = EGG::Vector3f(1.0f, 0.95f, 1.0f);

    [[nodiscard]] bool checkSpherePartialImpl(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
            KCLTypeMask *maskOut);
    [[nodiscard]] bool checkSpherePartialPushImpl(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
            KCLTypeMask *maskOut);
    [[nodiscard]] bool checkSphereFullImpl(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info,
            KCLTypeMask *maskOut);
    [[nodiscard]] bool checkSphereFullPushImpl(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info,
            KCLTypeMask *maskOut);

    /// @brief Helper function to return the spatial index of a given block
    [[nodiscard]] std::pair<s32, s32> static SpatialIndex(const EGG::Vector3f &pos) {
        constexpr f32 ORIGIN_OFFSET_X = -30647.498f;
        constexpr f32 ORIGIN_OFFSET_Z = -21092.5f;
        constexpr f32 GRID_WIDTH = 325.0f; // The "width" of each cell in the spatial grid
        constexpr f32 GRID_HALF_WIDTH = 162.5f;

        s32 x = (pos.x - ORIGIN_OFFSET_X + GRID_HALF_WIDTH) / GRID_WIDTH;
        s32 z = (pos.z - ORIGIN_OFFSET_Z + GRID_HALF_WIDTH) / GRID_WIDTH;

        return std::make_pair(x, z);
    }

    ObjectCollisionBox *m_colBox;       ///< The hitbox of the block to check collision against
    ObjectCollisionSphere *m_colSphere; ///< The kart hitbox to check collision against

    /// @brief Spatially-indexed 122-by-116 array of blocks for faster collision lookups
    std::array<std::array<ObjectObakeBlock *, CACHE_SIZE_X>, CACHE_SIZE_Z> m_blockCache;

    fixed_vector<ObjectObakeBlock *> m_blocks;        ///< Owning vector of all blocks
    fixed_vector<ObjectObakeBlock *> m_fallingBlocks; ///< Pointers to all actively falling blocks

    static constexpr size_t MAX_BLOCKS = 656; ///< Allocated size of m_blocks and m_calcBlocks
};

} // namespace Kinoko::Field
