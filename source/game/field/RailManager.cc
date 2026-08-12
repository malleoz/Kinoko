#include "RailManager.hh"

#include "game/system/CourseMap.hh"
#include "game/system/map/MapdataGeoObj.hh"

namespace Kinoko::Field {

/// @addr{0x806F09C8}
RailManager *RailManager::CreateInstance() {
    ASSERT(!s_instance);
    s_instance = EGG::egg_new<RailManager>();
    s_instance->createPaths();
    return s_instance;
}

/// @addr{0x806F0A4C}
void RailManager::DestroyInstance() {
    ASSERT(s_instance);
    auto *instance = s_instance;
    s_instance = nullptr;
    EGG::egg_delete(instance);
}

/// @addr{0x806F0A3C}
RailManager::RailManager() = default;

/// @addr{0x806F0A98}
RailManager::~RailManager() {
    if (s_instance) {
        s_instance = nullptr;
        WARN("RailManager instance not explicitly handled!");
    }

    for (auto *&rail : m_rails) {
        EGG::egg_delete(rail);
    }
}

/// @addr{0x806F0AD8}
/// @brief Parses all rails from the @ref CourseMap, distinguishing between linear and curved rails
/// @details In the base game, this function differentiates between object routes and camera routes.
/// Since we do not implement camera rail functionality in Kinoko, we can simplify the logic in this
/// function a bit by not checking object rail ids.
void RailManager::createPaths() {
    auto *courseMap = System::CourseMap::Instance();
    u16 railCount = courseMap->getPointInfoCount();
    m_rails.reserve(railCount);

    for (u16 i = 0; i < railCount; ++i) {
        auto *pointInfo = courseMap->getPointInfo(i);
        bool isSpline = pointInfo->setting(0);

        if (isSpline) {
            m_rails.push_back(EGG::egg_new<RailSpline>(i, pointInfo));
        } else {
            m_rails.push_back(EGG::egg_new<RailLine>(i, pointInfo));
        }
    }
}

RailManager *RailManager::s_instance = nullptr; ///> @addr{0x809C22B0}

} // namespace Kinoko::Field
