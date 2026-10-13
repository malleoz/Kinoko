#pragma once

#include "game/kart/KartObjectProxy.hh"

#include "game/system/map/MapdataPointInfo.hh"

namespace Kinoko::Kart {

class KartPullPath;

/// @brief Tracks the kart's progress along a pull path.
/// @details This implementation merges the global and regional tracker into one class. Regional
/// pull paths only run once a segment has already been established (identified via
/// `m_handle->incomingIdx() >= 0`). Every frame, it checks the current segment, the next segment,
/// and the previous segment, stopping at the first hit. This regional check exists because it is a
/// cheap search, because it assumes that the kart has only moved a small distance since the last
/// frame. The global tracker is an exhaustive round-robin scan of all points in the pull path. Like
/// with the regional tracker, it also runs every frame, but it only checks the current segment.
/// Regardless of whether or not the search succeeds, every frame it increments @ref m_currentIdx
/// and wraps around to 0 if it exceeds the number of points in the pull path. This way, the global
/// tracker acts as a fallback so that it can identify the current pull path segment if the kart
/// enters the area at an unexpected location, such as the Koopa Cape turnskip before the red shell.
class KartPullPathTracker : private KartObjectProxy {
public:
    /// @brief The type of tracker to use for the pull path
    enum class Type {
        Global,   ///< Exhaustive round-robin scan of all points in the pull path
        Regional, ///< Checks the current segment, the next segment, and the previous segment
    };

    /// @addr{0x8059308C}
    /// @brief Constructor
    /// @param handle Pointer to the owning @ref KartPullPath subsystem
    /// @param type The type of tracker to use for the pull path
    /// @details Sets @ref m_type and @ref m_handle to the provided `type` and `handle`. Initializes
    /// @ref m_currentIdx to zero and @ref m_pointInfo to `nullptr`.
    KartPullPathTracker(KartPullPath *handle, Type type)
        : m_type(type),
          m_currentIdx(0),
          m_pointInfo(nullptr),
          m_handle(handle) {}

    /// @addr{0x805930E4}
    /// @brief Default virtual destructor
    ~KartPullPathTracker() override = default;

    /// @brief Calls into the appropriate tracker type's calc function to find the current segment
    /// of the pull path
    /// @details If this is a @ref Type::Global tracker, calls @ref calcTrackerGlobal() to perform
    /// an exhaustive round-robin scan of all points in the pyll path. Otherwise if this is a @ref
    /// Type::Regional tracker, calls @ref calcTrackerRegional() to check the current, next, and
    /// previous segments.
    /// @pre Requires that @ref setPointInfo() has been set to initialize @ref m_pointInfo.
    void calc() {
        ASSERT(m_pointInfo);
        if (m_type == Type::Global) {
            calcTrackerGlobal();
        } else if (m_type == Type::Regional) {
            calcTrackerRegional();
        }
    }

    /// @beginSetters

    /// @brief Sets the current index into the pull path's array of points
    /// @param idx The index to set as the current point in the pull path
    void setCurrentIdx(s16 idx) {
        m_currentIdx = idx;
    }

    /// @brief Sets the current point info for the tracker to search through
    /// @param info The point info to set for the tracker
    /// @details Sets @ref m_pointInfo to the provided `info` and resets @ref m_currentIdx to zero.
    void setPointInfo(const System::MapdataPointInfo *info) {
        m_pointInfo = info;
        m_currentIdx = 0;
    }

    /// @endSetters

private:
    /// @brief The direction to search for the current point in the pull path
    enum class SearchDirection {
        Current,  ///< Search within the current segment
        Next,     ///< Search within the next segment
        Previous, ///< Search within the previous segment
    };

    void calcTrackerGlobal();
    void calcTrackerRegional();

    /// @addr{0x80593310}
    /// @brief Gets the distance from the kart to the line formed by the point and direction.
    /// @param point A point on the line
    /// @param dir The direction of the line
    /// @return The distance from the kart to the line formed by the point and direction
    [[nodiscard]] f32 getDistance(const EGG::Vector3f &point, const EGG::Vector3f &dir) const {
        EGG::Vector3f diff = pos() - point;
        f32 dist = diff.length();
        f32 x = EGG::Mathf::abs(diff.dot(dir));
        return EGG::Mathf::sqrt(dist * dist - x * x);
    }

    [[nodiscard]] bool search(SearchDirection searchDirection, s16 &idx, EGG::Vector3f &point,
            EGG::Vector3f &dir) const;

    const Type m_type; ///< Distinguishes between regional and global trackers
    s16 m_currentIdx;  ///< Current index into the pull path's point array
    const System::MapdataPointInfo *m_pointInfo; ///< Pointer to the pull path's point info
    KartPullPath *const m_handle; ///< Pointer to the owning @ref KartPullPath subsytem
};

/// @brief Manages areas pulling the kart along a given path.
/// @details This implementation merges the base class with the water derived class.
class KartPullPath : public KartObjectProxy {
public:
    /// @addr{0x80593FA4}
    /// @brief Constructor
    /// @details Constructs @ref m_globalTracker and @ref m_regionalTracker and calls @ref init().
    KartPullPath()
        : m_globalTracker(this, KartPullPathTracker::Type::Global),
          m_regionalTracker(this, KartPullPathTracker::Type::Regional) {
        init();
    }

    /// @addr{0x80594094}
    /// @brief Default virtual destructor
    ~KartPullPath() override = default;

    /// @addr{0x805940D4}
    /// @brief Initializes the pull path subsystem
    /// @details Calls @reset(). Initializes @ref m_areaId to `-1` and @ref m_roadSpeedDecay to
    /// `1.0f`.
    void init() {
        reset();
        m_areaId = -1;
        m_roadSpeedDecay = 1.0f;
    }

    void reset();

    /// @addr{0x80594134}
    /// @brief Every frame, updates the @ref KartPullPathTracker objects to find the current segment
    /// of the pull path
    /// @details If there is no active pull path area, it bails out. Otherwise, as long as the pull
    /// path area has more than 2 points, calls @ref calcTrackers() to update the trackers.
    /// @pre If there is an active pull path area, requires that @ref m_pointInfo is valid.
    void calc() {
        if (calcArea()) {
            ASSERT(m_pointInfo);
            if (m_pointInfo->pointCount() > 2) {
                calcTrackers();
            }
        }
    }

    /// @addr{0x80593DBC}
    /// @brief Changes the current point in the pull path to the given index and distance, if it's
    /// closer than what's already been found this frame
    /// @param idx The index of the point in the pull path to change to
    /// @param distance The distance from the line formed by the point and direction
    /// @details This is called whenever the trackers' search functions succeed. If the distance is
    /// absurdly large or is further away than the current best found this frame, it bails out.
    /// Otherwise, it updates @ref m_incomingIdx to the provided `idx`, @ref m_distance to the
    /// provided `distance`, sets the provided `idx` in @ref m_regionalTracker, and calculates the
    /// pull direction via @ref calcPointChange().
    void changePoint(s16 idx, f32 distance) {
        if ((m_distance >= 0.0f || distance >= 3000.0f) && distance >= m_distance) {
            return;
        }

        m_incomingIdx = idx;
        m_distance = distance;
        m_regionalTracker.setCurrentIdx(idx);
        calcPointChange();
    }

    /// @addr{0x80593E08}
    /// @brief Clears the current distance, so that the next call to @ref changePoint will succeed
    /// @details Sets @ref m_distance to `-1.0f` to indicate that no valid distance has been found
    /// this frame.
    void resetDistance() {
        m_distance = -1.0f;
    }

    /// @beginGetters

    /// @brief Gets the index of the closest candidate segment found this frame
    /// @return The index of the closest candidate segment found this frame, @ref m_incomingIdx
    [[nodiscard]] s16 incomingIdx() const {
        return m_incomingIdx;
    }

    /// @brief Gets the current pull direction
    /// @return The current pull direction, @ref m_pullDirection
    [[nodiscard]] const EGG::Vector3f &pullDirection() const {
        return m_pullDirection;
    }

    /// @brief Gets the current pull speed
    /// @return The current pull speed, @ref m_pullSpeed
    [[nodiscard]] f32 pullSpeed() const {
        return m_pullSpeed;
    }

    /// @brief Gets the maximum pull speed
    /// @return The maximum pull speed, @ref m_maxPullSpeed
    [[nodiscard]] f32 maxPullSpeed() const {
        return m_maxPullSpeed;
    }

    /// @brief Gets the road speed decay factor
    /// @return The road speed decay factor, @ref m_roadSpeedDecay
    [[nodiscard]] f32 roadSpeedDecay() const {
        return m_roadSpeedDecay;
    }

    /// @endGetters

private:
    bool calcArea();
    void calcPointChange();

    /// @addr{0x80593D54}
    /// @brief Runs the global and regional trackers' per-frame calc functions
    void calcTrackers() {
        m_globalTracker.calc();
        m_regionalTracker.calc();
    }

    /// @addr{0x805AEAD8}
    /// @brief Computes the unit vector perpendicular to the pull direction in the XZ plane
    /// @return The sideways direction of the pull direction
    /// @details This is used to redirect the pull path sideways instead of forward. This isn't a
    /// part of KartPullPath, but is only called from this class.
    [[nodiscard]] EGG::Vector3f getPullSideDirection() const {
        // Dot product of two unit vectors being 1.0f => angle between them is 0
        // They're the same vector, just return zero
        if (EGG::Vector3f::ey.dot(m_pullDirection) == 1.0f) {
            return EGG::Vector3f::zero;
        }

        EGG::Vector3f nrm = EGG::Vector3f::ey.cross(m_pullDirection);
        nrm.normalise();
        return nrm;
    }

    /// @addr{0x80593D1C}
    /// @brief Sets the current point info for the trackers to search through
    /// @param info The point info to set for the trackers
    void setTrackerPointInfo(const System::MapdataPointInfo *info) {
        m_globalTracker.setPointInfo(info);
        m_regionalTracker.setPointInfo(info);
    }

    f32 m_distance; ///< Best distance found this frame from a candidate segment's line
    const System::MapdataPointInfo *m_pointInfo; ///< Pointer to the pull path's point info
    s16 m_incomingIdx;             ///< Index of the closest candidate segment found this frame
    s16 m_currentIdx;              ///< Index of the current segment in the pull path
    EGG::Vector3f m_pullDirection; ///< Current direction the pull path pushes the kart
    f32 m_pullSpeed;               ///< The speed at which the pull path pushes the kart
    f32 m_maxPullSpeed;            ///< Maximum speed at which the pull path can push the kart
    KartPullPathTracker m_globalTracker;   ///< The global round-robin tracker instance
    KartPullPathTracker m_regionalTracker; ///< The regional tracker instance
    f32 m_roadSpeedDecay; ///< Speed decay factor while the kart is inside the pull area
    s16 m_areaId;         ///< The ID of the currently active @ref System::MapdataAreaBase
};

} // namespace Kinoko::Kart
