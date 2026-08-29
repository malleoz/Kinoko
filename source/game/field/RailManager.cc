#include "RailManager.hh"

#include "game/system/CourseMap.hh"

namespace Kinoko::Field {

/// @addr{0x806F0A3C}
/// @brief Private constructor
RailManager::RailManager() = default;

/// @addr{0x806F0A98}
/// @brief Private destructor
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
/// @brief Parses all rails from the @ref System::CourseMap, distinguishing between linear and
/// curved rails
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
