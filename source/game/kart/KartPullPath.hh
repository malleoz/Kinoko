#pragma once

#include "game/kart/KartObjectProxy.hh"

namespace Kinoko {

namespace System {

class MapdataPointInfo;

} // namespace System

namespace Kart {

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

    KartPullPathTracker(KartPullPath *handle, Type type);
    ~KartPullPathTracker() override;

    void calc();

    /// @beginSetters

    void setCurrentIdx(s16 idx);
    void setPointInfo(const System::MapdataPointInfo *info);

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

    [[nodiscard]] f32 getDistance(const EGG::Vector3f &point, const EGG::Vector3f &dir) const;
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
    KartPullPath();
    ~KartPullPath() override;

    void init();
    void reset();
    void calc();
    void changePoint(s16 idx, f32 distance);
    void resetDistance();

    /// @beginGetters

    [[nodiscard]] s16 incomingIdx() const;
    [[nodiscard]] const EGG::Vector3f &pullDirection() const;
    [[nodiscard]] f32 pullSpeed() const;
    [[nodiscard]] f32 maxPullSpeed() const;
    [[nodiscard]] f32 roadSpeedDecay() const;

    /// @endGetters

private:
    bool calcArea();
    void calcPointChange();
    void calcTrackers();
    void setTrackerPointInfo(const System::MapdataPointInfo *info);
    [[nodiscard]] EGG::Vector3f getPullSideDirection() const;

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

} // namespace Kart

} // namespace Kinoko
