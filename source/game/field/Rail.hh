#pragma once

#include "game/system/map/MapdataPointInfo.hh"

namespace Field {

struct RailLineTransition {
    f32 m_length;
    f32 m_lengthInv;
    EGG::Vector3f m_dir;
};

struct RailSplineTransition {
    EGG::Vector3f m_p0;
    EGG::Vector3f m_p1;
    EGG::Vector3f m_p2;
    EGG::Vector3f m_p3;
    f32 m_length;
    f32 m_lengthInv;
};

class Rail {
public:
    Rail(u16 idx, System::MapdataPointInfo *info);
    ~Rail();

    void addPoint(f32 scale, const EGG::Vector3f &point);
    void checkSphereFull();

    [[nodiscard]] const EGG::Vector3f &pointPos(u16 idx) const;

protected:
    virtual f32 getPathLength() const = 0;
    virtual const std::vector<RailLineTransition> &getLinearTransitions() = 0;
    virtual const std::vector<RailSplineTransition> &getSplineTransitions() = 0;
    virtual s32 getEstimatorSampleCount() const = 0;
    virtual f32 getEstimatorStep() const = 0;
    virtual const std::vector<f32> &getPathPercentages() const = 0;
    virtual void onPointsChanged() = 0;
    virtual void onPointAdded() = 0;

    u16 m_pointCount;
    bool m_isOscillating;
    std::vector<System::MapdataPointInfo::Point> m_points;
    f32 m_someScale;

private:
    u16 m_idx;
    u16 m_pointCapacity;
    bool m_hasCheckedCol;
    std::vector<EGG::Vector3f> m_floorNrms;
};

class RailLine : public Rail {
public:
    RailLine(u16 idx, System::MapdataPointInfo *info);
    ~RailLine();

private:
    f32 getPathLength() const override;
    const std::vector<RailLineTransition> &getLinearTransitions() override;
    const std::vector<RailSplineTransition> &getSplineTransitions() override;
    s32 getEstimatorSampleCount() const override;
    f32 getEstimatorStep() const override;
    const std::vector<f32> &getPathPercentages() const override;
    void onPointsChanged() override {}
    void onPointAdded() override {}

    u16 m_dirCount;
    std::vector<RailLineTransition> m_transitions;
    f32 m_pathLength;
};

class RailSpline : public Rail {
public:
    RailSpline(u16 idx, System::MapdataPointInfo *info);
    ~RailSpline();

private:
    f32 getPathLength() const override;
    const std::vector<RailLineTransition> &getLinearTransitions() override;
    const std::vector<RailSplineTransition> &getSplineTransitions() override;
    s32 getEstimatorSampleCount() const override;
    f32 getEstimatorStep() const override;
    const std::vector<f32> &getPathPercentages() const override;
    void onPointsChanged() override;
    void onPointAdded() override;

    void invalidateTransitions(bool lastOnly);
    void calcCubicBezierControlPoints(const EGG::Vector3f &p0, const EGG::Vector3f &p1,
            const EGG::Vector3f &p2, const EGG::Vector3f &p3, u32 count,
            RailSplineTransition &transition);
    f32 estimateLength(const RailSplineTransition &transition, u32 count);
    EGG::Vector3f calcCubicBezierP1(const EGG::Vector3f &p0, const EGG::Vector3f &p1,
            const EGG::Vector3f &p2) const;
    EGG::Vector3f calcCubicBezierP2(const EGG::Vector3f &p0, const EGG::Vector3f &p1,
            const EGG::Vector3f &p2) const;
    EGG::Vector3f cubicBezier(f32 t, const RailSplineTransition &transition) const;

    u16 m_transitionCount;
    std::vector<RailSplineTransition> m_transitions;
    u32 m_estimatorSampleCount;
    f32 m_estimatorStep;
    std::vector<f32> m_pathPercentages;
    u32 m_segmentCount;
    f32 m_pathLength;
    bool m_doNotAllocatePathPercentages;
};

} // namespace Field
