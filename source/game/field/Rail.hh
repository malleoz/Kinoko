#pragma once

#include <game/system/map/MapdataPointInfo.hh>

namespace Field {

class Rail {
public:
    Rail(u16 idx, const System::MapdataPointInfo &pointInfo);
    virtual ~Rail();

    void checkSphereFull();
    void checkSpherePartialPush();

private:
    u16 m_idx;
    u16 m_pointCount1;                                     // offset 0x2
    u16 m_pointCount2;                                     // offset 0x4
    std::vector<System::MapdataPointInfo::Point> m_points; // offset 0x8
    std::vector<EGG::Vector3f> m_floorNrms;                // 0ffset 0x10
    std::vector<s32> m_floorAttrs;                         // offset 0x18
};

} // namespace Field
