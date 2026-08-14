#include "ObjectObakeManager.hh"

#include "game/field/CollisionDirector.hh"

#include "game/system/RaceManager.hh"

namespace Kinoko::Field {

/// @addr{0x8080B0D8}
ObjectObakeManager::ObjectObakeManager(const System::MapdataGeoObj &params)
    : ObjectDrivable(params), m_blockCache({}), m_blocks(MAX_BLOCKS), m_fallingBlocks(MAX_BLOCKS) {
    static constexpr f32 BLOCK_WIDTH = 195.00002f;
    static constexpr f32 BLOCK_HEIGHT = 130.0f;

    m_colBox = EGG::egg_new<ObjectCollisionBox>(BLOCK_WIDTH, BLOCK_HEIGHT, BLOCK_WIDTH,
            EGG::Vector3f::zero);
    m_colSphere = EGG::egg_new<ObjectCollisionSphere>(1.0f, EGG::Vector3f::zero);

    addBlock(params);
}

/// @addr{0x8080BEA4}
ObjectObakeManager::~ObjectObakeManager() {
    EGG::egg_delete(m_colBox);
    EGG::egg_delete(m_colSphere);

    for (auto *&block : m_blocks) {
        EGG::egg_delete(block);
    }
}

/// @addr{0x8080BB28}
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

/// @addr{0x8080B244}
/// @brief Public interface that adds a new block to the manager and caches it for collision checks
void ObjectObakeManager::addBlock(const System::MapdataGeoObj &params) {
    auto *block = EGG::egg_new<ObjectObakeBlock>(params);
    m_blocks.push_back(block);
    auto [spatialX, spatialZ] = SpatialIndex(block->pos());
    m_blockCache[spatialZ][spatialX] = block;
}

/// @addr{0x8080BEE4}
/// @brief Checks collision between a sphere and the cached blocks, writing partial collision info
/// @param radius The radius of the sphere to check
/// @param pos The position of the sphere to check
/// @param mask The KCL flags to check collision against (other types are ignored)
/// @param info Out parameter for retrieving collision information (if any)
/// @param maskOut The KCL flags that were hit during the collision check (if any)
/// @return Whether a collision was detected
/// @details Checks all cells in a 3x3 grid around the sphere's position for potential collisions.
/// Two independent collision checks occur: one for the blocks' wall collision and one for the
/// blocks' road collision (when a player is driving on top of a block).
bool ObjectObakeManager::checkSpherePartialImpl(f32 radius, const EGG::Vector3f &pos,
        const EGG::Vector3f & /*prevPos*/, KCLTypeMask mask, CollisionInfoPartial *info,
        KCLTypeMask *maskOut) {
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
                            info->update(dist);
                        }

                        if (maskOut) {
                            *maskOut |= KCL_TYPE_BIT(COL_TYPE_SPECIAL_WALL);
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
                            info->update(dist);
                        }

                        if (maskOut) {
                            *maskOut |= KCL_TYPE_BIT(COL_TYPE_ROAD);
                        }
                    }
                }

                collision |= collided;
            }
        }
    }

    return collision;
}

/// @addr{0x8080C41C}
/// @brief Checks collision between a sphere and the cached blocks, writing partial collision info
///        Additionally pushes the collision entry into the CollisionDirector's cache.
/// @param radius The radius of the sphere to check
/// @param pos The position of the sphere to check
/// @param mask The KCL flags to check collision against (other types are ignored)
/// @param info Out parameter for retrieving collision information (if any)
/// @param maskOut The KCL flags that were hit during the collision check (if any)
/// @return Whether a collision was detected
/// @details Checks all cells in a 3x3 grid around the sphere's position for potential collisions.
/// Two independent collision checks occur: one for the blocks' wall collision and one for the
/// blocks' road collision (when a player is driving on top of a block).
bool ObjectObakeManager::checkSpherePartialPushImpl(f32 radius, const EGG::Vector3f &pos,
        const EGG::Vector3f & /*prevPos*/, KCLTypeMask mask, CollisionInfoPartial *info,
        KCLTypeMask *maskOut) {
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
                            info->update(dist);
                        }

                        if (maskOut) {
                            auto *colDir = CollisionDirector::Instance();
                            colDir->pushCollisionEntry(dist.length(), maskOut,
                                    KCL_TYPE_BIT(COL_TYPE_SPECIAL_WALL), COL_TYPE_SPECIAL_WALL);
                            colDir->setCurrentCollisionVariant(2);
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
                            info->update(dist);
                        }

                        if (maskOut) {
                            auto *colDir = CollisionDirector::Instance();
                            colDir->pushCollisionEntry(dist.length(), maskOut,
                                    KCL_TYPE_BIT(COL_TYPE_ROAD), COL_TYPE_ROAD);
                        }
                    }
                }

                collision |= collided;
            }
        }
    }

    return collision;
}

/// @addr{0x8080C980}
/// @brief Checks collision between a sphere and the cached blocks, writing full collision info
/// @param radius The radius of the sphere to check
/// @param pos The position of the sphere to check
/// @param mask The KCL flags to check collision against (other types are ignored)
/// @param info Out parameter for retrieving collision information (if any)
/// @param maskOut The KCL flags that were hit during the collision check (if any)
/// @return Whether a collision was detected
/// @details Checks all cells in a 3x3 grid around the sphere's position for potential collisions.
/// Two independent collision checks occur: one for the blocks' wall collision and one for the
/// blocks' road collision (when a player is driving on top of a block).
bool ObjectObakeManager::checkSphereFullImpl(f32 radius, const EGG::Vector3f &pos,
        const EGG::Vector3f & /*prevPos*/, KCLTypeMask mask, CollisionInfo *info,
        KCLTypeMask *maskOut) {
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
                            info->update(dist.length(), dist, distNrm, KCL_TYPE_WALL);
                        }

                        if (maskOut) {
                            *maskOut |= KCL_TYPE_BIT(COL_TYPE_SPECIAL_WALL);
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
                            info->update(dist.length(), dist, distNrm, KCL_TYPE_FLOOR);
                        }

                        if (maskOut) {
                            *maskOut |= KCL_TYPE_BIT(COL_TYPE_ROAD);
                        }
                    }
                }

                collision |= collided;
            }
        }
    }

    return collision;
}

/// @addr{0x8080D12C}
/// @brief Checks collision between a sphere and the cached blocks, writing full collision info
///        Additionally pushes the collision entry into the CollisionDirector's cache.
/// @param radius The radius of the sphere to check
/// @param pos The position of the sphere to check
/// @param mask The KCL flags to check collision against (other types are ignored)
/// @param info Out parameter for retrieving collision information (if any)
/// @param maskOut The KCL flags that were hit during the collision check (if any)
/// @return Whether a collision was detected
/// @details Checks all cells in a 3x3 grid around the sphere's position for potential collisions.
/// Two independent collision checks occur: one for the blocks' wall collision and one for the
/// blocks' road collision (when a player is driving on top of a block).
bool ObjectObakeManager::checkSphereFullPushImpl(f32 radius, const EGG::Vector3f &pos,
        const EGG::Vector3f & /*prevPos*/, KCLTypeMask mask, CollisionInfo *info,
        KCLTypeMask *maskOut) {
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
                            info->update(dist.length(), dist, distNrm, KCL_TYPE_WALL);
                        }

                        if (maskOut) {
                            auto *colDir = CollisionDirector::Instance();
                            colDir->pushCollisionEntry(dist.length(), maskOut,
                                    KCL_TYPE_BIT(COL_TYPE_SPECIAL_WALL), COL_TYPE_SPECIAL_WALL);
                            colDir->setCurrentCollisionVariant(2);
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
                            info->update(dist.length(), dist, distNrm, KCL_TYPE_FLOOR);
                        }

                        if (maskOut) {
                            auto *colDir = CollisionDirector::Instance();
                            colDir->pushCollisionEntry(dist.length(), maskOut,
                                    KCL_TYPE_BIT(COL_TYPE_ROAD), COL_TYPE_ROAD);
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
