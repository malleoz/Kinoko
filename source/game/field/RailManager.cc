#include "RailManager.hh"

#include "game/field/Rail.hh"

#include "game/system/CourseMap.hh"

namespace Kinoko::Field {

/// @brief Fetches the rail at the given index
/// @param idx The index of the rail to fetch
/// @return A pointer to the rail at the given index in @ref m_rails
/// @pre Requires that `idx` is a valid index within @ref m_rails
Rail *RailManager::rail(size_t idx) {
    ASSERT(idx < m_rails.size());
    return m_rails[idx];
}

/// @brief  Fetches the rail at the given index
/// @param idx The index of the rail to fetch
/// @return A const pointer to the rail at the given index in @ref m_rails
/// @pre Requires that `idx` is a valid index within @ref m_rails
const Rail *RailManager::rail(size_t idx) const {
    ASSERT(idx < m_rails.size());
    return m_rails[idx];
}

/// @addr{0x806F09C8}
/// @brief Creates the singleton instance of @ref RailManager and parses all rails from the
/// @ref System::CourseMap
/// @return A pointer to the newly created singleton instance of @ref RailManager
RailManager *RailManager::CreateInstance() {
    ASSERT(!s_instance);
    s_instance = EGG::egg_new<RailManager>();
    s_instance->createPaths();
    return s_instance;
}

/// @addr{0x806F0A4C}
/// @brief Destroys the singleton instance of the @ref RailManager
void RailManager::DestroyInstance() {
    ASSERT(s_instance);
    auto *instance = s_instance;
    s_instance = nullptr;
    EGG::egg_delete(instance);
}

/// @brief Returns the singleton instance of the @ref RailManager
/// @return A pointer to the singleton instance of the @ref RailManager
RailManager *RailManager::Instance() {
    return s_instance;
}

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

RailManager *RailManager::s_instance = nullptr;

} // namespace Kinoko::Field
