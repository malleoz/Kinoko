#pragma once

#include <Types.hh>

namespace Kinoko {

namespace Host {

class Context;

} // namespace Host

namespace Field {

class Rail;

/// @brief Creates and exposes access to the rails from @ref System::CourseMap
/// @details For each rail, determines whether it represents a linear rail (@ref RailLine) or a
/// curved rail (@ref RailSpline).
class RailManager {
    /// @brief Grants access to the singleton so a @ref Host::Context can restore the instance's
    /// state on context switch
    friend class Host::Context;

public:
    /// @beginGetters

    [[nodiscard]] Rail *rail(size_t idx);
    [[nodiscard]] const Rail *rail(size_t idx) const;

    /// @endGetters

    static RailManager *CreateInstance();
    static void DestroyInstance();
    [[nodiscard]] static RailManager *Instance();

private:
    EGG_NEW_DELETE_FRIEND

    RailManager();
    ~RailManager();

    void createPaths();

    /// @brief The rails parsed from @ref System::CourseMap
    fixed_vector<Rail *> m_rails;

    /// @addr{0x809C22B0}
    /// @brief The pointer to the singleton instance of the class
    static RailManager *s_instance;
};

} // namespace Field

} // namespace Kinoko
