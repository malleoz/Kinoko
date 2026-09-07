#pragma once

#include <egg/math/Vector.hh>

namespace Kinoko {

namespace Host {

class Context;

} // namespace Host

namespace Kart {

class KartObject;

} // namespace Kart

namespace Field {

class ObjectCollidable;
class ObjectDrivable;

/// @brief A bitfield that represents the state and type of a given BoxColUnit.
/// @details The lower 8 bits represent the type, while the remaining bits represent the state.
/// There are originally two flags for objects, but one is for CPUs, which we can ignore for now.
enum class eBoxColFlag {
    Driver = 0,          ///< Indicates the unit is a player
    Object = 3,          ///< Indicates the unit is a collidable object
    Drivable = 4,        ///< Indicates the unit is a drivable object
    PermRecalcAABB = 8,  ///< Recalculate this unit's spatial indexing every frame
    Intangible = 9,      ///< @ref BoxColManager should ignore collision with the unit
    Active = 10,         ///< Indicates the unit is active
    TempRecalcAABB = 11, ///< Only recalculate this unit's spatial indexing once
};

/// @brief A bitfield of @ref eBoxColFlag that represents the state and type of a given BoxColUnit
typedef EGG::TBitFlag<u32, eBoxColFlag> BoxColFlag;

/// @brief A representation of the boundaries of an entity that has dynamic collision
/// @details Used by the @ref BoxColManager which spatially indexes these units to allow for more
/// efficient collision checks.
struct BoxColUnit {
    BoxColUnit();
    ~BoxColUnit();

    void init(f32 radius, f32 maxSpeed, const EGG::Vector3f *pos, const BoxColFlag &flag,
            void *userData);

    /// @addr{0x80786F6C}
    /// @brief Marks the collision unit as inactive by resetting its Active flag
    void makeInactive() {
        m_flag.resetBit(eBoxColFlag::Active);
    }

    /// @addr{0x80786F7C}
    /// @brief Updates the radius and range of the unit
    /// @details This effectively adjusts how close the player needs to be to the object before
    /// collision checks are performed.
    /// @param radius The new radius of the collision box
    /// @param maxSpeed The maximum speed for the collision box
    void resize(f32 radius, f32 maxSpeed) {
        m_radius = radius;
        m_range = radius + maxSpeed;
        m_flag.setBit(eBoxColFlag::TempRecalcAABB);
    }

    void reinsert();
    void search(const BoxColFlag &flag);

    const EGG::Vector3f *m_pos; ///< Pointer to the position of the unit in 3D space
    f32 m_radius;               ///< The radius of the unit's bounding sphere
    f32 m_range;                ///< Expanded radius accounting for max speed
    BoxColFlag m_flag;          ///< The flags representing the state and type of the unit
    void *m_userData;           ///< Pointer to the object that this BoxColUnit represents
    s16 m_highPointIdx;         ///< Index of the unit's high point in the spatial index
    s16 m_lowPointIdx;          ///< Index of the unit's low point in the spatial index
    f32 m_xMax;                 ///< Maximum X coordinate of the unit's bounding box
    f32 m_xMin;                 ///< Minimum X coordinate of the unit's bounding box
};

/// @brief Represents the lower Z-axis boundary of a collision unit
struct BoxColLowPoint {
    f32 z;        ///< Z-coordinate of the unit's lower bound
    u8 highPoint; ///< The associated high point index
    u8 unitID;    ///< The ID of the unit associated with this low point
};

/// @brief Represents the upper Z-axis boundary of a collision unit
struct BoxColHighPoint {
    f32 z;          ///< Z-coordinate of the unit's upper bound
    u8 lowPoint;    ///< The associated low point index
    u8 minLowPoint; ///< Minimum low point index for this unit
};

/// @brief Spatial indexing manager for entities with dynamic collision
/// @details Implements a sweep-and-prune algorithm for efficient collision detection. Each unit has
/// two boundary points: the high point (Z-pos + range) and the low point (Z-pos - range), which are
/// sorted in @ref m_highPoints and @ref m_lowPoints respectively. When searching for collisions
/// with a unit, the manager first determines the range of low point indices to search within,
/// eliminating candidates which lie entirely outside the unit's Z-axis range. It then checks the
/// X-axis bounds to eliminate candidates which lie entirely outside the unit's X-axis range. It
/// then references the unit's radius to determine if it lies within a candidate's collision sphere.
/// Also implements result caching so that searches with the same parameters can be quickly
/// resolved. Also checks each unit's @ref BoxColFlag bits to determine if it should be considered
/// in collision checks. Once the collision checks have been completed, resulting collision objects
/// can be iterated using the @ref getNextObject and @ref getNextDrivable methods.
class BoxColManager : EGG::Disposer {
    /// @brief Grants access to the singleton so a @ref Host::Context can restore the instance's
    /// state on context switch
    friend class Host::Context;

public:
    void clear();
    void calc();

    /// @addr{0x80785E5C}
    /// @brief Retrieves the next collidable object in the iteration sequence
    /// @return A pointer to the next @ref ObjectCollidable, or nullptr if there are no more objects
    [[nodiscard]] ObjectCollidable *getNextObject() {
        return reinterpret_cast<ObjectCollidable *>(
                getNextImpl(m_nextObjectID, eBoxColFlag::Object));
    }

    /// @addr{0x80785EC4}
    /// @brief Retrieves the next drivable object in the iteration sequence
    /// @return A pointer to the next @ref ObjectDrivable, or nullptr if there are no more objects
    [[nodiscard]] ObjectDrivable *getNextDrivable() {
        return reinterpret_cast<ObjectDrivable *>(
                getNextImpl(m_nextDrivableID, eBoxColFlag::Drivable));
    }

    /// @addr{0x80785F2C}
    /// @brief Resets the iteration sequence for both collidable and drivable objects, setting the
    /// iterators to the first object in each sequence
    void resetIterators() {
        m_nextObjectID = -1;
        iterate(m_nextObjectID, eBoxColFlag::Object);

        m_nextDrivableID = -1;
        iterate(m_nextDrivableID, eBoxColFlag::Drivable);
    }

    /// @addr{0x80786050}
    /// @brief Inserts a new driver unit into the spatial index with the specified parameters
    /// @param radius The radius of the driver unit
    /// @param maxSpeed The maximum speed of the driver unit
    /// @param pos Pointer to the driver unit's position
    /// @param alwaysRecalc Whether the unit should always recalculate its AABB
    /// @param kartObject The associated Kart object for the driver unit
    /// @return A pointer to the newly inserted @ref BoxColUnit
    [[nodiscard]] BoxColUnit *insertDriver(f32 radius, f32 maxSpeed, const EGG::Vector3f *pos,
            bool alwaysRecalc, Kart::KartObject *kartObject) {
        BoxColFlag flag = BoxColFlag(eBoxColFlag::Driver);

        if (alwaysRecalc) {
            flag.setBit(eBoxColFlag::PermRecalcAABB);
        }

        return insert(radius, maxSpeed, pos, flag, kartObject);
    }

    /// @addr{0x80786078}
    /// @brief Inserts a new collidable object unit into the spatial index with the specified
    /// parameters
    /// @param radius The radius of the object unit
    /// @param maxSpeed The maximum speed of the object unit
    /// @param pos Pointer to the object unit's position
    /// @param alwaysRecalc Whether the unit should always recalculate its AABB
    /// @param userData The ObjectCollidable pointer associated with this BoxColUnit
    /// @return A pointer to the newly inserted @ref BoxColUnit
    [[nodiscard]] BoxColUnit *insertObject(f32 radius, f32 maxSpeed, const EGG::Vector3f *pos,
            bool alwaysRecalc, void *userData) {
        BoxColFlag flag = BoxColFlag(eBoxColFlag::Object);

        if (alwaysRecalc) {
            flag.setBit(eBoxColFlag::PermRecalcAABB);
        }

        return insert(radius, maxSpeed, pos, flag, userData);
    }

    /// @addr{0x80786120}
    /// @brief Inserts a new drivable object unit into the spatial index with the specified
    /// parameters
    /// @param radius The radius of the drivable unit
    /// @param maxSpeed The maximum speed of the drivable unit
    /// @param pos Pointer to the drivable unit's position
    /// @param alwaysRecalc Whether the unit should always recalculate its AABB
    /// @param userData The ObjectDrivable pointer associated with this BoxColUnit
    /// @return A pointer to the newly inserted @ref BoxColUnit
    [[nodiscard]] BoxColUnit *insertDrivable(f32 radius, f32 maxSpeed, const EGG::Vector3f *pos,
            bool alwaysRecalc, void *userData) {
        BoxColFlag flag = BoxColFlag(eBoxColFlag::Drivable);

        if (alwaysRecalc) {
            flag.setBit(eBoxColFlag::PermRecalcAABB);
        }

        return insert(radius, maxSpeed, pos, flag, userData);
    }

    void reinsertUnit(BoxColUnit *unit);
    void remove(BoxColUnit *&unit);

    /// @addr{0x80786774}
    /// @brief Checks if the provided BoxColUnit collides with any units in the spatial index having
    /// the provided @ref BoxColFlag
    /// @param unit The collision unit to check for collisions
    /// @param flag The collision flag to filter which units to check against
    void search(BoxColUnit *unit, const BoxColFlag &flag) {
        searchImpl(unit, flag);
        resetIterators();
    }

    /// @addr{0x80786B14}
    /// @brief Checks if a sphere collides with any units in the spatial index having
    /// the provided @ref BoxColFlag
    /// @param radius The radius of the sphere to check for collisions
    /// @param pos The center position of the sphere
    /// @param flag The collision flag to filter which units to check against
    void search(f32 radius, const EGG::Vector3f &pos, const BoxColFlag &flag) {
        searchImpl(radius, pos, flag);
        resetIterators();
    }

    [[nodiscard]] bool isSphereInSpatialCache(f32 radius, const EGG::Vector3f &pos,
            const BoxColFlag &flag) const;

    /// @addr{0x807855DC}
    /// @brief Creates the singleton instance of the @ref BoxColManager
    /// @return A pointer to the newly created @ref BoxColManager instance
    static BoxColManager *CreateInstance() {
        ASSERT(!s_instance);
        s_instance = EGG::egg_new<BoxColManager>();
        return s_instance;
    }

    /// @addr{0x8078562C}
    /// @brief Destroys the singleton instance of the @ref BoxColManager
    static void DestroyInstance() {
        ASSERT(s_instance);
        auto *instance = s_instance;
        s_instance = nullptr;
        EGG::egg_delete(instance);
    }

    /// @brief Returns the singleton instance of the @ref BoxColManager
    /// @return A pointer to the singleton instance of the @ref BoxColManager
    [[nodiscard]] static BoxColManager *Instance() {
        return s_instance;
    }

private:
    EGG_NEW_DELETE_FRIEND

    BoxColManager();
    ~BoxColManager() override;

    /// @brief Helper function since the getters share all code except the flag
    [[nodiscard]] void *getNextImpl(s32 &id, const BoxColFlag &flag) {
        if (id == MAX_UNIT_COUNT) {
            return nullptr;
        }

        BoxColUnit *unit = m_units[id];
        iterate(id, flag);

        return unit->m_userData;
    }

    /// @addr{Inlined}
    /// @brief Finds the next collision unit in the spatial index that matches the specified flag
    void iterate(s32 &iter, const BoxColFlag &flag) {
        while (++iter < m_maxID) {
            if (m_units[iter]->m_flag.on(flag)) {
                return;
            }
        }

        iter = MAX_UNIT_COUNT;
    }

    [[nodiscard]] BoxColUnit *insert(f32 radius, f32 maxSpeed, const EGG::Vector3f *pos,
            const BoxColFlag &flag, void *userData);
    void searchImpl(BoxColUnit *unit, const BoxColFlag &flag);
    void searchImpl(f32 radius, const EGG::Vector3f &pos, const BoxColFlag &flag);

    /// @brief The maximum number of collision units that can be managed
    static constexpr size_t MAX_UNIT_COUNT = 0x100;

    std::array<BoxColHighPoint, MAX_UNIT_COUNT> m_highPoints; ///< Upper Z-axis boundary per unit
    std::array<BoxColLowPoint, MAX_UNIT_COUNT> m_lowPoints;   ///< Lower Z-axis boundary per unit
    std::array<BoxColUnit, MAX_UNIT_COUNT> m_unitPool;        ///< Pool of all allocated units
    std::array<BoxColUnit *, MAX_UNIT_COUNT> m_units;         ///< Units within our search bounds

    /// @brief Free list of available pool indices; each entry points to the next free slot
    std::array<u32, MAX_UNIT_COUNT> m_unitIDs;

    s32 m_unitCount;         ///< Number of currently active units
    s32 m_nextUnitID;        ///< Head of the free list; next pool index to allocate
    s32 m_nextObjectID;      ///< Iterator index into @ref m_units for object traversal
    s32 m_nextDrivableID;    ///< Iterator index into @ref m_units for drivable traversal
    s32 m_maxID;             ///< Number of valid entries in @ref m_units after a search
    BoxColUnit *m_cacheUnit; ///< The unit from the last unit-based search; null for sphere searches
    EGG::Vector3f m_cachePoint; ///< Center position of the last sphere search
    f32 m_cacheRadius;          ///< Radius of the last sphere search; -1 if no cache is valid
    BoxColFlag m_cacheFlag;     ///< Flag filter used in the last search

    static BoxColManager *s_instance; ///< @addr{0x809C2EF0}
};

} // namespace Field

} // namespace Kinoko
