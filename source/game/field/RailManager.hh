#pragma once

#include "game/field/RailInterpolator.hh"

namespace Kinoko {

namespace Host {

class Context;

} // namespace Host

namespace Field {

/// @brief Creates and exposes access to the rails from @ref System::CourseMap
/// @details For each rail, determines whether it represents a linear rail (@ref RailLine) or a
/// curved rail (@ref RailSpline).
class RailManager {
    /// @brief Grants access to the singleton so a @ref Host::Context can restore the instance's
    /// state on context switch
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

    /// @addr{0x806F09C8}
    /// @brief Creates the singleton instance of @ref RailManager and parses all rails from the
    /// @ref System::CourseMap
    /// @return A pointer to the newly created singleton instance of @ref RailManager
    static RailManager *CreateInstance() {
        ASSERT(!s_instance);
        s_instance = EGG::egg_new<RailManager>();
        s_instance->createPaths();
        return s_instance;
    }

    /// @addr{0x806F0A4C}
    /// @brief Destroys the singleton instance of the @ref RailManager
    static void DestroyInstance() {
        ASSERT(s_instance);
        auto *instance = s_instance;
        s_instance = nullptr;
        EGG::egg_delete(instance);
    }

    /// @brief Returns the singleton instance of the @ref RailManager
    /// @return A pointer to the singleton instance of the @ref RailManager
    [[nodiscard]] static RailManager *Instance() {
        return s_instance;
    }

private:
    EGG_NEW_DELETE_FRIEND

    RailManager();
    ~RailManager();

    void createPaths();

    /// @brief The rails parsed from @ref System::CourseMap
    fixed_vector<Rail *> m_rails;

    static RailManager *s_instance; ///< @addr{0x809C22B0}
};

} // namespace Field

} // namespace Kinoko
