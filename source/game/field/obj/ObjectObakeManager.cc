#include "ObjectObakeManager.hh"

#include "game/field/CollisionDirector.hh"

#include "game/system/RaceManager.hh"

namespace Kinoko::Field {

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

// Explicit instantiation, since callers of checkSphereImpl() live in the header and would
// otherwise be unable to see this definition when the class's vtable is emitted.
template bool ObjectObakeManager::checkSphereImpl<CollisionInfo>(f32 radius,
        const EGG::Vector3f &pos, const EGG::Vector3f &prevPos, KCLTypeMask mask,
        CollisionInfo *info, KCLTypeMask *maskOut, bool push);
template bool ObjectObakeManager::checkSphereImpl<CollisionInfoPartial>(f32 radius,
        const EGG::Vector3f &pos, const EGG::Vector3f &prevPos, KCLTypeMask mask,
        CollisionInfoPartial *info, KCLTypeMask *maskOut, bool push);

} // namespace Kinoko::Field
