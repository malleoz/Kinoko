#include "BoxColManager.hh"

#include <numeric>

namespace Kinoko::Field {

/// @addr{0x80786ED0}
BoxColUnit::BoxColUnit() : m_pos(nullptr), m_radius(0.0f), m_range(0.0f), m_userData(nullptr) {}

/// @addr{0x80786EF4}
BoxColUnit::~BoxColUnit() = default;

/// @addr{0x80786F34}
/// @brief Initializes the collision unit with the given parameters
void BoxColUnit::init(f32 radius, f32 maxSpeed, const EGG::Vector3f *pos, const BoxColFlag &flag,
        void *userData) {
    m_pos = pos;
    m_radius = radius;
    m_range = radius + maxSpeed;
    m_flag = flag;
    m_flag.setBit(eBoxColFlag::Active);
    m_userData = userData;
    m_xMax = pos->x + m_range;
    m_xMin = pos->x - m_range;
}

/// @addr{0x80786F98}
/// @brief Removes and re-inserts the collision unit into the @ref BoxColManager's spatial index
void BoxColUnit::reinsert() {
    BoxColManager::Instance()->reinsertUnit(this);
}

/// @addr{0x80786FA8}
/// @brief Searches for collisions involving this unit with the specified flags
void BoxColUnit::search(const BoxColFlag &flag) {
    BoxColManager::Instance()->search(this, flag);
}

/// @addr{0x807856E0}
/// @brief Creates two intangible units to represent the hard boundaries of the spatial index
BoxColManager::BoxColManager() {
    constexpr f32 SPATIAL_BOUND = 999999.9f;

    static const EGG::Vector3f upperBound(SPATIAL_BOUND, SPATIAL_BOUND, SPATIAL_BOUND);
    static const EGG::Vector3f lowerBound(-SPATIAL_BOUND, -SPATIAL_BOUND, -SPATIAL_BOUND);

    std::iota(m_unitIDs.begin(), m_unitIDs.end(), 1);

    m_unitCount = 0;
    m_nextUnitID = 0;

    clear();

    BoxColFlag flags;
    insert(1.0f, 0.0f, &upperBound, flags, nullptr)->m_flag.setBit(eBoxColFlag::Intangible);
    insert(1.0f, 0.0f, &lowerBound, flags, nullptr)->m_flag.setBit(eBoxColFlag::Intangible);
}

/// @addr{0x807854E4}
BoxColManager::~BoxColManager() {
    if (s_instance) {
        s_instance = nullptr;
        WARN("BoxColManager instance not explicitly handled!");
    }
}

/// @addr{0x8078597C}
/// @brief Clears the result cache and resets the iterator indices
void BoxColManager::clear() {
    m_nextObjectID = MAX_UNIT_COUNT;
    m_nextDrivableID = MAX_UNIT_COUNT;
    m_maxID = 0;
    m_cacheUnit = nullptr;
    m_cacheRadius = -1.0f;
    m_cacheFlag.makeAllZero();
}

/// @addr{0x807859B0}
/// @brief Recalculates the bounds of all active units having @enum eBoxColFlag::PermRecalcAABB or
/// @enum eBoxColFlag::TempRecalcAABB flag, and then updates the low and high points accordingly.
void BoxColManager::calc() {
    clear();

    // Update the AABB if necessary
    int activeUnitIdx = 0;
    for (auto &unit : m_unitPool) {
        // Unit is not active, so it's not factored into mUnitCount, skip it
        if (unit.m_flag.offBit(eBoxColFlag::Active)) {
            continue;
        }

        // Unit is active, but we don't need to recalc its AABB, move to the next unit
        if (unit.m_flag.onBit(eBoxColFlag::PermRecalcAABB, eBoxColFlag::TempRecalcAABB)) {
            unit.m_xMax = unit.m_pos->x + unit.m_range;
            unit.m_xMin = unit.m_pos->x - unit.m_range;
            m_highPoints[unit.m_highPointIdx].z = unit.m_pos->z + unit.m_range;
            m_lowPoints[unit.m_lowPointIdx].z = unit.m_pos->z - unit.m_range;

            unit.m_flag.resetBit(eBoxColFlag::TempRecalcAABB);
        }

        // We're done with all of the active units, so we avoid iterating over the remaining
        // inactive units
        if (++activeUnitIdx >= m_unitCount) {
            break;
        }
    }

    // The loops use insertion sort.
    // We assume the units in the spatial index only change in small portions at a time per frame.
    // The time complexity is closer to O(n) as a result rather than O(n^2).

    // Reorganize the high points
    for (size_t i = 1; i < static_cast<size_t>(m_unitCount); ++i) {
        for (size_t j = i; j >= 1 && m_highPoints[j - 1].z > m_highPoints[j].z; --j) {
            BoxColHighPoint &upper = m_highPoints[j];
            BoxColHighPoint &lower = m_highPoints[j - 1];

            std::swap(upper, lower);

            BoxColLowPoint &upperLow = m_lowPoints[upper.lowPoint];
            BoxColLowPoint &lowerLow = m_lowPoints[lower.lowPoint];
            ++upperLow.highPoint;
            --lowerLow.highPoint;

            ++m_unitPool[upperLow.unitID].m_highPointIdx;
            --m_unitPool[lowerLow.unitID].m_highPointIdx;

            u8 &nextMinLowPoint = upper.minLowPoint;
            if (nextMinLowPoint == lower.lowPoint) {
                do {
                    ++nextMinLowPoint;
                } while (m_lowPoints[nextMinLowPoint].highPoint < j);
            }

            lower.minLowPoint = std::min(lower.minLowPoint, upper.lowPoint);
        }
    }

    // Reorganize the low points
    for (size_t i = 1; i < static_cast<size_t>(m_unitCount); ++i) {
        for (size_t j = i; j >= 1 && m_lowPoints[j - 1].z > m_lowPoints[j].z; --j) {
            BoxColLowPoint &upper = m_lowPoints[j];
            BoxColLowPoint &lower = m_lowPoints[j - 1];

            std::swap(upper, lower);

            ++m_highPoints[upper.highPoint].lowPoint;
            --m_highPoints[lower.highPoint].lowPoint;

            ++m_unitPool[upper.unitID].m_lowPointIdx;
            --m_unitPool[lower.unitID].m_lowPointIdx;

            if (upper.highPoint > lower.highPoint) {
                int k = upper.highPoint;

                while (k > lower.highPoint && m_highPoints[k].minLowPoint == j - 1) {
                    ++m_highPoints[k--].minLowPoint;
                }
            } else {
                int k = lower.highPoint;

                while (k > upper.highPoint && m_highPoints[k].minLowPoint == j) {
                    --m_highPoints[k--].minLowPoint;
                }
            }
        }
    }
}

/// @addr{0x80786DBC}
/// @brief Reinserts an existing collision unit into the spatial index, updating its position and
/// other properties as necessary
/// @unused
void BoxColManager::reinsertUnit(BoxColUnit *unit) {
    f32 radius = unit->m_radius;
    f32 maxSpeed = unit->m_range - radius;
    const EGG::Vector3f *pos = unit->m_pos;
    BoxColFlag flag = unit->m_flag;
    void *userData = unit->m_userData;

    remove(unit);
    insert(radius, maxSpeed, pos, BoxColFlag(), userData)->m_flag = flag;
}

/// @addr{0x80786578}
/// @brief Removes a collision unit from the spatial index and updates the relevant data structures
void BoxColManager::remove(BoxColUnit *&unit) {
    if (!unit || unit->m_flag.offBit(eBoxColFlag::Active)) {
        return;
    }

    int highPointIdx = unit->m_highPointIdx;
    int lowPointIdx = unit->m_lowPointIdx;

    // Update high points
    for (int i = highPointIdx; i < m_unitCount - 1; ++i) {
        BoxColHighPoint &high = m_highPoints[i];
        m_highPoints[i] = m_highPoints[i + 1];
        BoxColLowPoint &low = m_lowPoints[high.lowPoint];
        --low.highPoint;
        --m_unitPool[low.unitID].m_highPointIdx;

        if (high.minLowPoint > lowPointIdx) {
            --high.minLowPoint;
        }
    }

    // Update low points
    for (int i = lowPointIdx; i < m_unitCount - 1; ++i) {
        BoxColLowPoint &low = m_lowPoints[i];
        m_lowPoints[i] = m_lowPoints[i + 1];
        BoxColHighPoint &high = m_highPoints[low.highPoint];
        --high.lowPoint;
        --m_unitPool[low.unitID].m_lowPointIdx;

        if (low.highPoint >= highPointIdx) {
            continue;
        }

        int minLowPoint = m_highPoints[low.highPoint].minLowPoint;

        if (minLowPoint != lowPointIdx) {
            continue;
        }

        for (BoxColLowPoint *pLowPoint = &m_lowPoints[minLowPoint];
                pLowPoint->highPoint < low.highPoint; ++minLowPoint) {
            ++pLowPoint;
        }

        m_highPoints[low.highPoint].minLowPoint = minLowPoint;
    }

    unit->makeInactive();
    int nextID = unit - m_unitPool.data();
    m_unitIDs[nextID] = m_nextUnitID;
    m_nextUnitID = nextID;
    --m_unitCount;
    unit = nullptr;
}

/// @addr{0x80786E60}
/// @brief Checks if a sphere is within the spatial cache based on its radius and position, only if
/// all bits in the provided flag mask were set in the cached query
bool BoxColManager::isSphereInSpatialCache(f32 radius, const EGG::Vector3f &pos,
        const BoxColFlag &flag) const {
    if (m_cacheRadius == -1.0f) {
        return false;
    }

    if (!m_cacheFlag.onAll(flag)) {
        return false;
    }

    f32 radiusDiff = m_cacheRadius - radius;
    EGG::Vector3f posDiff = pos - m_cachePoint;

    return EGG::Mathf::abs(posDiff.x) <= radiusDiff && EGG::Mathf::abs(posDiff.z) <= radiusDiff;
}

/// @addr{0x80786134}
/// @brief Creates a new collision unit with the specified parameters and inserts it into the
/// spatial index
/// @param radius The radius of the sphere
/// @param maxSpeed The maximum speed of the sphere
/// @param pos Pointer to the collision unit's position
/// @param flag The collision flag to assign to the unit
/// @param userData Pointer to the object to be associated with the newly created BoxColUnit
/// @return A pointer to the newly inserted @ref BoxColUnit
BoxColUnit *BoxColManager::insert(f32 radius, f32 maxSpeed, const EGG::Vector3f *pos,
        const BoxColFlag &flag, void *userData) {
    if (m_unitCount >= static_cast<s32>(MAX_UNIT_COUNT)) {
        return nullptr;
    }

    s32 unitID = m_nextUnitID;
    BoxColUnit &unit = m_unitPool[unitID];
    unit.init(radius, maxSpeed, pos, flag, userData);
    m_nextUnitID = m_unitIDs[unitID];
    f32 range = radius + maxSpeed;
    f32 zHigh = pos->z + range;
    f32 zLow = pos->z - range;

    if (m_unitCount == 0) {
        m_highPoints[0].lowPoint = 0;
        m_highPoints[0].minLowPoint = 0;
        m_lowPoints[0].unitID = unitID;
        m_highPoints[0].z = zHigh;
        m_lowPoints[0].highPoint = 0;
        m_lowPoints[0].unitID = unitID;
        m_lowPoints[0].z = zLow;
        m_unitPool[0].m_highPointIdx = 0;
        m_unitPool[0].m_lowPointIdx = 0;
        m_unitCount = 1;

        return &unit;
    }

    // Binary search
    int highPointIdx = 0;
    int lowPointIdx = 0;
    int i = m_unitCount;

    while (true) {
        int highSearch = highPointIdx + i;
        int lowSearch = lowPointIdx + i;

        if (highSearch <= m_unitCount && zHigh > m_highPoints[highSearch - 1].z) {
            highPointIdx = highSearch;
        }

        if (lowSearch <= m_unitCount && zLow > m_lowPoints[lowSearch - 1].z) {
            lowPointIdx = lowSearch;
        }

        if (i == 1) {
            break;
        }

        i = (i + 1) / 2;
    }

    unit.m_highPointIdx = highPointIdx;
    unit.m_lowPointIdx = lowPointIdx;

    // Update high points
    for (int i = m_unitCount; i > highPointIdx; --i) {
        BoxColHighPoint &high = m_highPoints[i];
        m_highPoints[i] = m_highPoints[i - 1];
        BoxColLowPoint &low = m_lowPoints[high.lowPoint];

        ++low.highPoint;
        ++m_unitPool[low.unitID].m_highPointIdx;

        if (high.minLowPoint >= lowPointIdx) {
            ++high.minLowPoint;
        }
    }

    m_highPoints[highPointIdx].lowPoint = lowPointIdx;
    m_highPoints[highPointIdx].z = zHigh;

    // Update min low point
    if (highPointIdx == m_unitCount || m_highPoints[highPointIdx + 1].minLowPoint > lowPointIdx) {
        m_highPoints[highPointIdx].minLowPoint = lowPointIdx;

        for (int i = highPointIdx - 1; i >= 0 && m_highPoints[i].minLowPoint > lowPointIdx; --i) {
            m_highPoints[i].minLowPoint = lowPointIdx;
        }
    } else {
        m_highPoints[highPointIdx].minLowPoint = m_highPoints[highPointIdx + 1].minLowPoint;
    }

    // Update low points
    for (int i = m_unitCount; i > lowPointIdx; --i) {
        BoxColLowPoint &low = m_lowPoints[i];
        m_lowPoints[i] = m_lowPoints[i - 1];
        BoxColHighPoint &high = m_highPoints[low.highPoint];
        ++high.lowPoint;
        ++m_unitPool[low.unitID].m_lowPointIdx;
    }

    m_lowPoints[lowPointIdx].highPoint = highPointIdx;
    m_lowPoints[lowPointIdx].unitID = unitID;
    m_lowPoints[lowPointIdx].z = zLow;
    ++m_unitCount;

    return &unit;
}

/// @addr{0x807868C0}
/// @brief Searches for collision units that intersect with the specified unit and match the given
/// flag
/// @details First, if the collision unit is not active, the search is aborted. This function then
/// computes the X and Z-axis bounds of the provided BoxColUnit. It then calculates the range of
/// m_lowPoints in the spatial index to consider for potential collisions. Iterating across the
/// m_lowPoints, it checks for intersections with the specified unit and filters them based on the
/// provided collision flag. Colliding units are stored in m_units and the count of colliding units
/// is tracked via m_maxID.
void BoxColManager::searchImpl(BoxColUnit *unit, const BoxColFlag &flag) {
    if (unit->m_flag.offBit(eBoxColFlag::Active)) {
        return;
    }

    int highPointIdx = unit->m_highPointIdx;
    int lowPointIdx = unit->m_lowPointIdx;
    int origLowPointIdx = unit->m_lowPointIdx;

    f32 highZPos = m_highPoints[highPointIdx].z;
    f32 lowZPos = m_lowPoints[origLowPointIdx].z;

    f32 xMax = unit->m_xMax;
    f32 xMin = unit->m_xMin;

    const EGG::Vector3f *pos = unit->m_pos;
    f32 radius = unit->m_radius;

    f32 zHigh = pos->z + radius;
    f32 zLow = pos->z - radius;
    f32 xHigh = pos->x + radius;
    f32 xLow = pos->x - radius;

    int maxIdx = m_unitCount - 1;

    m_maxID = 0;
    m_cacheUnit = unit;
    m_cacheRadius = -1.0f;
    m_cacheFlag = flag;

    for (; highPointIdx > 7 && m_highPoints[highPointIdx - 8].z >= lowZPos;) {
        highPointIdx -= 8;
    }

    for (; highPointIdx > 0 && m_highPoints[highPointIdx - 1].z >= lowZPos;) {
        --highPointIdx;
    }

    for (; lowPointIdx < maxIdx - 7 && m_lowPoints[lowPointIdx + 8].z <= highZPos;) {
        lowPointIdx += 8;
    }

    for (; lowPointIdx < maxIdx && m_lowPoints[lowPointIdx + 1].z <= highZPos;) {
        ++lowPointIdx;
    }

    u8 minLowPoint = m_highPoints[highPointIdx].minLowPoint;

    for (int i = lowPointIdx; i >= minLowPoint; --i, --lowPointIdx) {
        BoxColLowPoint &low = m_lowPoints[i];

        if (low.highPoint >= highPointIdx && lowPointIdx != origLowPointIdx) {
            BoxColUnit &lowUnit = m_unitPool[low.unitID];

            if (lowUnit.m_xMax < xMin || lowUnit.m_xMin > xMax) {
                continue;
            }

            if (lowUnit.m_flag.off(flag) || lowUnit.m_flag.onBit(eBoxColFlag::Intangible)) {
                continue;
            }

            f32 radius = lowUnit.m_radius;
            if (lowUnit.m_pos->z + radius < zLow || lowUnit.m_pos->z - radius > zHigh) {
                continue;
            }

            if (lowUnit.m_pos->x + radius < xLow || lowUnit.m_pos->x - radius > xHigh) {
                continue;
            }

            m_units[m_maxID++] = &lowUnit;

            if (m_maxID == MAX_UNIT_COUNT) {
                break;
            }
        }

        if (lowPointIdx == 0) {
            break;
        }
    }
}

/// @addr{0x80786C60}
/// @brief Searches for collision units intersecting with a sphere having a given center and radius
/// @details Computes the X and Z-axis bounds of the provided sphere. It then calculates the range
/// of m_lowPoints in the spatial index to consider for potential collisions. Iterating across the
/// m_lowPoints, it checks for intersections with the sphere and filters them based on the provided
/// collision flag. Colliding units are stored in m_units and the count of colliding units is
/// tracked via m_maxID.
void BoxColManager::searchImpl(f32 radius, const EGG::Vector3f &pos, const BoxColFlag &flag) {
    // Binary search
    int highPointIdx = 0;
    int lowPointIdx = 0;
    f32 zHigh = pos.z + radius;
    f32 zLow = pos.z - radius;
    f32 xHigh = pos.x + radius;
    f32 xLow = pos.x - radius;

    m_maxID = 0;
    m_cacheUnit = nullptr;
    m_cachePoint = pos;
    m_cacheRadius = radius;
    m_cacheFlag = flag;

    int i = m_unitCount - 1;
    while (true) {
        int highSearch = highPointIdx + i;
        int lowSearch = lowPointIdx + i;
        if (highSearch <= m_unitCount && zLow > m_highPoints[highSearch - 1].z) {
            highPointIdx = highSearch;
        }

        if (lowSearch <= m_unitCount && zHigh >= m_lowPoints[lowSearch].z) {
            lowPointIdx = lowSearch;
        }

        if (i == 1) {
            break;
        }

        i = (i + 1) / 2;
    }

    u8 minLowPoint = m_highPoints[highPointIdx].minLowPoint;

    for (i = lowPointIdx; i >= minLowPoint; --i, --lowPointIdx) {
        BoxColLowPoint &low = m_lowPoints[i];
        if (low.highPoint >= highPointIdx) {
            BoxColUnit &unit = m_unitPool[low.unitID];

            if (unit.m_xMax < xLow || unit.m_xMin > xHigh) {
                continue;
            }

            if (unit.m_flag.off(flag) || unit.m_flag.onBit(eBoxColFlag::Intangible)) {
                continue;
            }

            m_units[m_maxID++] = &unit;

            if (m_maxID == MAX_UNIT_COUNT) {
                break;
            }
        }

        if (lowPointIdx == 0) {
            break;
        }
    }
}

BoxColManager *BoxColManager::s_instance = nullptr; ///< @addr{0x809C2EF0}

} // namespace Kinoko::Field
