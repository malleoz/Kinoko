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
/// Regardless of whether or not the search succeeds, every frame it increments `@ref m_currentIdx`
/// and wraps around to 0 if it exceeds the number of points in the pull path. This way, the global
/// tracker acts as a fallback so that it can identify the current pull path segment if the kart
/// enters the area at an unexpected location, such as the Koopa Cape turnskip before the red shell.
class KartPullPathTracker : KartObjectProxy {
public:
    /// @brief The type of tracker to use for the pull path
    enum class Type {
        Global,   ///< Exhaustive round-robin scan of all points in the pull path
        Regional, ///< Checks the current segment, the next segment, and the previous segment
    };

    KartPullPathTracker(KartPullPath *handle, Type type);
    ~KartPullPathTracker();

    void calc();

    /// @beginSetters
    void setCurrentIdx(s16 idx) {
        m_currentIdx = idx;
    }

    /// @brief Sets the current point info for the trackers to search through and resets the current
    /// index to 0
    /// @param info The point info to set for the trackers
    void setPointInfo(const System::MapdataPointInfo *info) {
        m_pointInfo = info;
        m_currentIdx = 0;
    }
    /// @endSetters

private:
    enum class SearchDirection {
        Current,
        Next,
        Previous,
    };

    void calcTrackerGlobal();
    void calcTrackerRegional();

    /// @addr{0x80593310}
    /// @brief Gets the distance from the line formed by the point and direction.
    /// @param point A point on the line.
    /// @param dir The direction of the line.
    [[nodiscard]] f32 getDistance(const EGG::Vector3f &point, const EGG::Vector3f &dir) const {
        EGG::Vector3f diff = pos() - point;
        f32 dist = diff.length();
        f32 x = EGG::Mathf::abs(diff.dot(dir));
        return EGG::Mathf::sqrt(dist * dist - x * x);
    }

    [[nodiscard]] bool search(SearchDirection searchDirection, s16 &idx, EGG::Vector3f &point,
            EGG::Vector3f &dir) const;

    Type m_type; ///< Distinguishes between regional and global trackers (to avoid inheritance)
    s16 m_currentIdx;
    const System::MapdataPointInfo *m_pointInfo;
    KartPullPath *const m_handle;
};

/// @brief Manages areas pulling the kart along a given path.
/// @details This implementation merges the base class with the water derived class.
class KartPullPath : KartObjectProxy {
public:
    KartPullPath();
    ~KartPullPath();

    /// @addr{0x805940D4}
    void init() {
        reset();
        m_areaId = -1;
        m_roadSpeedDecay = 1.0f;
    }

    void reset();
    void calc();
    void changePoint(s16 idx, f32 distance);

    /// @addr{0x80593E08}
    void resetDistance() {
        m_distance = -1.0f;
    }

    /// @beginGetters
    [[nodiscard]] s16 incomingIdx() const {
        return m_incomingIdx;
    }

    [[nodiscard]] const EGG::Vector3f &pullDirection() const {
        return m_pullDirection;
    }

    [[nodiscard]] f32 pullSpeed() const {
        return m_pullSpeed;
    }

    [[nodiscard]] f32 maxPullSpeed() const {
        return m_maxPullSpeed;
    }

    [[nodiscard]] f32 roadSpeedDecay() const {
        return m_roadSpeedDecay;
    }
    /// @endGetters

private:
    bool calcArea();
    void calcPointChange();

    /// @addr{0x80593D54}
    void calcTrackers() {
        m_globalTracker.calc();
        m_regionalTracker.calc();
    }

    [[nodiscard]] EGG::Vector3f getPullUnitNormal() const;

    /// @addr{0x80593D1C}
    void setTrackerPointInfo(const System::MapdataPointInfo *info) {
        m_globalTracker.setPointInfo(info);
        m_regionalTracker.setPointInfo(info);
    }

    f32 m_distance;
    const System::MapdataPointInfo *m_pointInfo;
    s16 m_incomingIdx;
    s16 m_currentIdx;
    EGG::Vector3f m_pullDirection;
    f32 m_pullSpeed;
    f32 m_maxPullSpeed;
    KartPullPathTracker m_globalTracker;
    KartPullPathTracker m_regionalTracker;
    f32 m_roadSpeedDecay;
    s16 m_areaId;
};

} // namespace Kinoko::Kart
