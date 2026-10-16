#include "ObjectObakeManager.hh"

#include "game/field/CollisionDirector.hh"

#include "game/system/RaceManager.hh"

namespace Kinoko::Field {

/// @addr{0x8080B0D8}
/// @copybrief Constructor that is called when the first @ref ObjectObakeBlock is added
/// @param params The parameters used to initialize the object
/// @details Sizes @ref m_blocks and @ref m_fallingBlocks to hold up to @ref MAX_BLOCKS
/// elements. Creates @ref m_colBox with a length and width of `195.00002f` and a height of
/// `130.0f`. Creates @ref m_colSphere with a radius of `1.0f` and a center at the origin.
/// Finally, adds the block defined by the `params` to the manager.
ObjectObakeManager::ObjectObakeManager(const System::MapdataGeoObj &params)
    : ObjectDrivable(params),
      m_blockCache({}),
      m_blocks(MAX_BLOCKS),
      m_fallingBlocks(MAX_BLOCKS) {
    static constexpr f32 BLOCK_WIDTH = 195.00002f;
    static constexpr f32 BLOCK_HEIGHT = 130.0f;

    m_colBox = EGG::egg_new<ObjectCollisionBox>(BLOCK_WIDTH, BLOCK_HEIGHT, BLOCK_WIDTH,
            EGG::Vector3f::zero);
    m_colSphere = EGG::egg_new<ObjectCollisionSphere>(1.0f, EGG::Vector3f::zero);

    addBlock(params);
}

/// @addr{0x8080BEA4}
/// @brief Virtual destructor that destroys the associated collision objects and blocks
ObjectObakeManager::~ObjectObakeManager() {
    EGG::egg_delete(m_colBox);
    EGG::egg_delete(m_colSphere);

    for (auto *&block : m_blocks) {
        EGG::egg_delete(block);
    }
}

/// @addr{0x8080BB28}
/// @copybrief ObjectBase::calc()
/// @details Checks if any blocks should start falling and updates the state of any falling blocks.
void ObjectObakeManager::calc() {
    u32 frame = System::RaceManager::Instance()->timer();

    for (auto *&block : m_blocks) {
        u32 fallFrame = block->fallFrame();

        // Block is starting to fall
        if (fallFrame > 0 && fallFrame <= frame &&
                block->fallState() == ObjectObakeBlock::FallState::Rest) {
            block->setFallState(ObjectObakeBlock::FallState::Falling);
            block->calc();
            m_fallingBlocks.push_back(block);
        }
    }

    for (auto *&block : m_fallingBlocks) {
        block->calc();
    }
}

/// @addr{0x8080BE98}
/// @copybrief ObjectBase::createCollision()
/// @details This is a no-op because the manager itself does not have any collision
void ObjectObakeManager::createCollision() {}

/// @addr{0x8080BE94}
/// @copybrief ObjectBase::calcCollisionTransform()
/// @details This is a no-op because the manager itself does not have any collision
void ObjectObakeManager::calcCollisionTransform() {}

/// @addr{0x8080BE9C}
/// @copybrief ObjectBase::loadFlags()
/// @return Returns @ref eLoadFlags::Calc and @ref eLoadFlags::Draw so that the object is
/// calculated every frame
ObjectBase::LoadFlags ObjectObakeManager::loadFlags() const {
    return LoadFlags().setBit(eLoadFlags::Calc, eLoadFlags::Draw);
}

/// @addr{0x8080BE84}
/// @copybrief ObjectBase::getCollisionRadius()
/// @return The collision radius of the manager, `100000.0f`.
/// @details Uses a large radius to ensure the manager is always considered for collision checks
f32 ObjectObakeManager::getCollisionRadius() const {
    return 100000.0f;
}

/// @addr{0x8080BE44}
/// @copydoc ObjectDrivable::checkPointPartial()
bool ObjectObakeManager::checkPointPartial(const EGG::Vector3f &pos, const EGG::Vector3f &prevPos,
        KCLTypeMask mask, CollisionInfoPartial *info, KCLTypeMask *maskOut) {
    return checkSpherePartialImpl(0.0f, pos, prevPos, mask, info, maskOut);
}

/// @addr{0x8080BE54}
/// @copydoc ObjectDrivable::checkPointPartialPush()
bool ObjectObakeManager::checkPointPartialPush(const EGG::Vector3f &pos,
        const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
        KCLTypeMask *maskOut) {
    return checkSpherePartialPushImpl(0.0f, pos, prevPos, mask, info, maskOut);
}

/// @addr{0x8080BE64}
/// @copydoc ObjectDrivable::checkPointFull()
bool ObjectObakeManager::checkPointFull(const EGG::Vector3f &pos, const EGG::Vector3f &prevPos,
        KCLTypeMask mask, CollisionInfo *info, KCLTypeMask *maskOut) {
    return checkSphereFullImpl(0.0f, pos, prevPos, mask, info, maskOut);
}

/// @addr{0x8080BE74}
/// @copydoc ObjectDrivable::checkPointFullPush()
bool ObjectObakeManager::checkPointFullPush(const EGG::Vector3f &pos, const EGG::Vector3f &prevPos,
        KCLTypeMask mask, CollisionInfo *info, KCLTypeMask *maskOut) {
    return checkSphereFullPushImpl(0.0f, pos, prevPos, mask, info, maskOut);
}

/// @addr{0x8080BE34}
/// @copydoc ObjectDrivable::checkSpherePartial()
bool ObjectObakeManager::checkSpherePartial(f32 radius, const EGG::Vector3f &pos,
        const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
        KCLTypeMask *maskOut, u32 /*timeOffset*/) {
    return checkSpherePartialImpl(radius, pos, prevPos, mask, info, maskOut);
}

/// @addr{0x8080BE38}
/// @copydoc ObjectDrivable::checkSpherePartialPush()
bool ObjectObakeManager::checkSpherePartialPush(f32 radius, const EGG::Vector3f &pos,
        const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
        KCLTypeMask *maskOut, u32 /*timeOffset*/) {
    return checkSpherePartialPushImpl(radius, pos, prevPos, mask, info, maskOut);
}

/// @addr{0x8080BE3C}
/// @copydoc ObjectDrivable::checkSphereFull()
bool ObjectObakeManager::checkSphereFull(f32 radius, const EGG::Vector3f &pos,
        const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info, KCLTypeMask *maskOut,
        u32 /*timeOffset*/) {
    return checkSphereFullImpl(radius, pos, prevPos, mask, info, maskOut);
}

/// @addr{0x8080BE40}
/// @copydoc ObjectDrivable::checkSphereFullPush()
bool ObjectObakeManager::checkSphereFullPush(f32 radius, const EGG::Vector3f &pos,
        const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info, KCLTypeMask *maskOut,
        u32 /*timeOffset*/) {
    return checkSphereFullPushImpl(radius, pos, prevPos, mask, info, maskOut);
}

/// @addr{0x8080BDF4}
/// @copydoc ObjectDrivable::checkPointCachedPartial()
bool ObjectObakeManager::checkPointCachedPartial(const EGG::Vector3f &pos,
        const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
        KCLTypeMask *maskOut) {
    return checkSpherePartialImpl(0.0f, pos, prevPos, mask, info, maskOut);
}

/// @addr{0x8080BE04}
/// @copydoc ObjectDrivable::checkPointCachedPartialPush()
bool ObjectObakeManager::checkPointCachedPartialPush(const EGG::Vector3f &pos,
        const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
        KCLTypeMask *maskOut) {
    return checkSpherePartialPushImpl(0.0f, pos, prevPos, mask, info, maskOut);
}

/// @addr{0x8080BE14}
/// @copydoc ObjectDrivable::checkPointCachedFull()
bool ObjectObakeManager::checkPointCachedFull(const EGG::Vector3f &pos,
        const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info, KCLTypeMask *maskOut) {
    return checkSphereFullImpl(0.0f, pos, prevPos, mask, info, maskOut);
}

/// @addr{0x8080BE24}
/// @copydoc ObjectDrivable::checkPointCachedFullPush()
bool ObjectObakeManager::checkPointCachedFullPush(const EGG::Vector3f &pos,
        const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info, KCLTypeMask *maskOut) {
    return checkSphereFullPushImpl(0.0f, pos, prevPos, mask, info, maskOut);
}

/// @addr{0x8080BDE4}
/// @copydoc ObjectDrivable::checkSphereCachedPartial()
bool ObjectObakeManager::checkSphereCachedPartial(f32 radius, const EGG::Vector3f &pos,
        const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
        KCLTypeMask *maskOut, u32 /*timeOffset*/) {
    return checkSpherePartialImpl(radius, pos, prevPos, mask, info, maskOut);
}

/// @addr{0x8080BDE8}
/// @copydoc ObjectDrivable::checkSphereCachedPartialPush()
bool ObjectObakeManager::checkSphereCachedPartialPush(f32 radius, const EGG::Vector3f &pos,
        const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
        KCLTypeMask *maskOut, u32 /*timeOffset*/) {
    return checkSpherePartialPushImpl(radius, pos, prevPos, mask, info, maskOut);
}

/// @addr{0x8080BDEC}
/// @copydoc ObjectDrivable::checkSphereCachedFull()
bool ObjectObakeManager::checkSphereCachedFull(f32 radius, const EGG::Vector3f &pos,
        const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info, KCLTypeMask *maskOut,
        u32 /*timeOffset*/) {
    return checkSphereFullImpl(radius, pos, prevPos, mask, info, maskOut);
}

/// @addr{0x8080BDF0}
/// @copydoc ObjectDrivable::checkSphereCachedFullPush()
bool ObjectObakeManager::checkSphereCachedFullPush(f32 radius, const EGG::Vector3f &pos,
        const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info, KCLTypeMask *maskOut,
        u32 /*timeOffset*/) {
    return checkSphereFullPushImpl(radius, pos, prevPos, mask, info, maskOut);
}

/// @addr{0x8080B244}
/// @brief Public interface that adds a new block to the existing manager and places the block
/// in the spatial cache for collision checks
/// @param params The parameters used to initialize the new block
void ObjectObakeManager::addBlock(const System::MapdataGeoObj &params) {
    auto *block = EGG::egg_new<ObjectObakeBlock>(params);
    m_blocks.push_back(block);
    auto [spatialX, spatialZ] = SpatialIndex(block->pos());
    m_blockCache[spatialZ][spatialX] = block;
}

/// @addr{0x8080BEE4}
/// @copydoc ObjectDrivable::checkSpherePartial()
bool ObjectObakeManager::checkSpherePartialImpl(f32 radius, const EGG::Vector3f &pos,
        const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
        KCLTypeMask *maskOut) {
    return checkSphereImpl(radius, pos, prevPos, mask, info, maskOut, false);
}

/// @addr{0x8080C41C}
/// @copydoc ObjectDrivable::checkSpherePartialPush()
bool ObjectObakeManager::checkSpherePartialPushImpl(f32 radius, const EGG::Vector3f &pos,
        const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfoPartial *info,
        KCLTypeMask *maskOut) {
    return checkSphereImpl(radius, pos, prevPos, mask, info, maskOut, true);
}

/// @addr{0x8080C980}
/// @copydoc ObjectDrivable::checkSphereFull()
bool ObjectObakeManager::checkSphereFullImpl(f32 radius, const EGG::Vector3f &pos,
        const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info, KCLTypeMask *maskOut) {
    return checkSphereImpl(radius, pos, prevPos, mask, info, maskOut, false);
}

/// @addr{0x8080D12C}
/// @copydoc ObjectDrivable::checkSphereFullPush()
bool ObjectObakeManager::checkSphereFullPushImpl(f32 radius, const EGG::Vector3f &pos,
        const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info, KCLTypeMask *maskOut) {
    return checkSphereImpl(radius, pos, prevPos, mask, info, maskOut, true);
}

/// @brief Helper function shared by the four checkSphere*Impl() overrides
/// @tparam T The collision info object type, either @ref CollisionInfoPartial or @ref
/// CollisionInfo.
/// @param radius The radius of the sphere to check
/// @param pos The position of the sphere to check
/// @param mask The KCL flags to check collision against (other types are ignored)
/// @param info Out parameter for retrieving collision information (if any)
/// @param maskOut The KCL flags that were hit during the collision check (if any)
/// @param push Whether to push a collision entry into the @ref CollisionDirector cache
/// @return Whether a collision was detected
/// @details Checks all cells in a 3x3 grid around the sphere's position for potential collisions.
/// Two independent collision checks occur: one for the blocks' wall collision and one for the
/// blocks' road collision (when a player is driving on top of a block). Updates `info` and
/// `maskOut` accordingly.
template <typename T>
    requires std::is_same_v<T, CollisionInfo> || std::is_same_v<T, CollisionInfoPartial>
bool ObjectObakeManager::checkSphereImpl(f32 radius, const EGG::Vector3f &pos,
        const EGG::Vector3f & /*prevPos*/, KCLTypeMask mask, T *info, KCLTypeMask *maskOut,
        bool push) {
    bool collision = false;
    auto [spatialX, spatialZ] = SpatialIndex(pos);

    EGG::Matrix34f t;
    t.makeT(pos);
    m_colSphere->transform(t, EGG::Vector3f(radius, radius, radius), EGG::Vector3f::zero);

    for (s32 i = spatialZ - 1; i <= spatialZ + 1; ++i) {
        for (s32 j = spatialX - 1; j <= spatialX + 1; ++j) {
            // Make sure we're in bounds of the cache
            if (j < 0 || static_cast<size_t>(j) >= CACHE_SIZE_X || i < 0 ||
                    static_cast<size_t>(i) >= CACHE_SIZE_Z) {
                continue;
            }

            auto *block = m_blockCache[i][j];
            if (!block) {
                continue;
            }

            // Bonking on top of block
            if (mask & KCL_TYPE_BIT(COL_TYPE_SPECIAL_WALL)) {
                t.makeT(block->pos());
                m_colBox->setBoundingRadius(WALL_BOUNDING_RADIUS);
                m_colBox->transform(t, WALL_SCALE, EGG::Vector3f::zero);

                EGG::Vector3f dist;
                bool collided = m_colSphere->check(*m_colBox, dist);

                if (collided) {
                    EGG::Vector3f distNrm = dist;
                    distNrm.normalise();

                    if (0.0f > distNrm.y || distNrm.y > 0.9f) {
                        collided = false;
                    } else {
                        if (info) {
                            if constexpr (std::is_same_v<T, CollisionInfo>) {
                                info->update(dist.length(), dist, distNrm, KCL_TYPE_WALL);
                            } else {
                                info->updateBBox(dist);
                            }
                        }

                        if (maskOut) {
                            if (push) {
                                auto *colDir = CollisionDirector::Instance();
                                colDir->pushCollisionEntry(dist.length(), maskOut,
                                        KCL_TYPE_BIT(COL_TYPE_SPECIAL_WALL), COL_TYPE_SPECIAL_WALL);
                                colDir->setCurrentCollisionVariant(2);
                            } else {
                                *maskOut |= KCL_TYPE_BIT(COL_TYPE_SPECIAL_WALL);
                            }
                        }
                    }
                }

                collision |= collided;
            }

            if (mask & KCL_TYPE_BIT(COL_TYPE_ROAD)) {
                t.makeT(block->pos());
                m_colBox->transform(t, ROAD_SCALE, EGG::Vector3f::zero);

                EGG::Vector3f dist;
                bool collided = m_colSphere->check(*m_colBox, dist);

                if (collided) {
                    EGG::Vector3f distNrm = dist;
                    distNrm.normalise();

                    if (0.9f >= distNrm.y) {
                        collided = false;
                    } else {
                        if (info) {
                            if constexpr (std::is_same_v<T, CollisionInfo>) {
                                info->update(dist.length(), dist, distNrm, KCL_TYPE_FLOOR);
                            } else {
                                info->updateBBox(dist);
                            }
                        }

                        if (maskOut) {
                            if (push) {
                                auto *colDir = CollisionDirector::Instance();
                                colDir->pushCollisionEntry(dist.length(), maskOut,
                                        KCL_TYPE_BIT(COL_TYPE_ROAD), COL_TYPE_ROAD);
                            } else {
                                *maskOut |= KCL_TYPE_BIT(COL_TYPE_ROAD);
                            }
                        }
                    }
                }

                collision |= collided;
            }
        }
    }

    return collision;
}

/// @brief Helper function to return the spatial index of a given block
/// @param pos The position of the block to compute the spatial index for
/// @return A pair of integers representing the (x, z) indices in the spatial grid
std::pair<s32, s32> ObjectObakeManager::SpatialIndex(const EGG::Vector3f &pos) {
    constexpr f32 ORIGIN_OFFSET_X = -30647.498f;
    constexpr f32 ORIGIN_OFFSET_Z = -21092.5f;
    constexpr f32 GRID_WIDTH = 325.0f; // The "width" of each cell in the spatial grid
    constexpr f32 GRID_HALF_WIDTH = 162.5f;

    s32 x = (pos.x - ORIGIN_OFFSET_X + GRID_HALF_WIDTH) / GRID_WIDTH;
    s32 z = (pos.z - ORIGIN_OFFSET_Z + GRID_HALF_WIDTH) / GRID_WIDTH;

    return std::make_pair(x, z);
}

} // namespace Kinoko::Field
