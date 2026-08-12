#pragma once

#include "game/field/RailInterpolator.hh"

#include <egg/core/Allocator.hh>

#include <vector>

namespace Kinoko {

namespace Host {

class Context;

} // namespace Host

namespace Field {

/// @brief Creates and exposes access to the rails from @ref CourseMap
/// @details For each rail, determines whether it represents a linear rail (@ref RailLine) or a
/// curved rail (@ref RailSpline).
class RailManager {
    friend class Host::Context;

public:
    /// @beginGetters
    /// @brief Fetches the rail at the given index
    /// @param idx The index of the rail to fetch
    /// @return A pointer to the rail at the given index
    [[nodiscard]] Rail *rail(size_t idx) {
        ASSERT(idx < m_rails.size());
        return m_rails[idx];
    }

    /// @brief  Fetches the rail at the given index
    /// @param idx The index of the rail to fetch
    /// @return A const pointer to the rail at the given index
    [[nodiscard]] const Rail *rail(size_t idx) const {
        ASSERT(idx < m_rails.size());
        return m_rails[idx];
    }
    /// @endGetters

    static RailManager *CreateInstance();
    static void DestroyInstance();

    [[nodiscard]] static RailManager *Instance() {
        return s_instance;
    }

private:
    EGG_NEW_DELETE_FRIEND

    RailManager();
    ~RailManager();

    void createPaths();

    /// @brief The rails parsed from @ref CourseMap
    fixed_vector<Rail *> m_rails;

    static RailManager *s_instance; ///< @addr{0x809C22B0}
};

} // namespace Field

} // namespace Kinoko
