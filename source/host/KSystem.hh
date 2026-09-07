#pragma once

#include <Common.hh>

#include "host/Context.hh"

namespace Kinoko {

/// @brief Base interface for a Kinoko system, which manages host-side initialization, playback life
/// cycle, command-line parsing, and frame stepping
class KSystem : EGG::Disposer {
    /// @brief Grants access to the singleton so a @ref Host::Context can restore the instance's
    /// state on context switch
    friend class Context;

public:
    /// @brief Default constructor
    KSystem() = default;

    /// @brief Default virtual destructor
    virtual ~KSystem() {}

    /// @brief Initializes the system
    virtual void init() = 0;

    /// @brief Executes a frame
    virtual void calc() = 0;

    /// @brief Executes a run
    /// @return Whether the run was successful or not.
    virtual bool run() = 0;

    /// @brief Parses non-generic command line options.
    /// @param argc The number of arguments.
    /// @param argv The arguments.
    /// @details It is recommended to not pass the executable, mode flag, and mode argument as these
    /// are handled in main.cc
    virtual void parseOptions(int argc, char **argv) = 0;

protected:
    static KSystem *s_instance; ///< Pointer to the singleton instance of the system
};

} // namespace Kinoko
