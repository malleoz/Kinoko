#pragma once

#include "game/system/GhostFile.hh"

#include <egg/core/Heap.hh>

#include <functional>

namespace Kinoko {

namespace Host {

class Context;

} // namespace Host

namespace System {

/// @brief Initializes the player with parameters specified in the provided ghost file.
/// @details In the base game, this class is responsible for managing the race and menu scenarios.
/// The menu scenario mostly pertains to character and vehicle selection in the menus prior to
/// starting a race. In Kinoko, we don't have these menus, so we initialize the race directly
/// through this class.
class RaceConfig : EGG::Disposer {
    /// @brief Grants access to the singleton so a @ref Host::Context can restore the instance's
    /// state on context switch
    friend Host::Context;

public:
    /// @brief Describes information about a player's configuration in the race
    struct Player {
        /// @brief Describes the type of player
        enum class Type {
            Local = 0, ///< Inputs managed by direct controller input
            Ghost = 3, ///< Inputs managed by ghost
            None = 5,  ///< No player
        };

        Character character; ///< The character that the player is using
        Vehicle vehicle;     ///< The vehicle that the player is using
        Type type;           ///< The type of player
        bool driftIsAuto;    ///< Whether the player's drift is set to automatic
    };

    /// @brief Describes information about the race configuration
    struct Scenario {
        std::array<Player, MAX_PLAYERS> players; ///< The array of @ref Player objects in the race
        u8 playerCount;                          ///< The number of players in the race
        Course course;                           ///< The course for the race

        void init();
    };

    /// @brief Describes the type of callback function used to initialize the RaceConfig instance
    /// @details The callback function is called after the RaceConfig instance has been initialized
    typedef std::function<void(RaceConfig *, void *)> InitCallback;

    void init();
    void initRace();
    void initControllers();
    void initGhost();

    /// @beginSetters

    void setGhost(const u8 *rkg);

    /// @endSetters

    /// @beginGetters

    [[nodiscard]] Scenario &raceScenario();
    [[nodiscard]] const Scenario &raceScenario() const;

    /// @endGetters

    static void RegisterInitCallback(const InitCallback &callback, void *arg);

    static RaceConfig *CreateInstance();
    static void DestroyInstance();
    [[nodiscard]] static RaceConfig *Instance();

private:
    EGG_NEW_DELETE_FRIEND

    RaceConfig();
    ~RaceConfig() override;

    Scenario m_raceScenario; ///< The race scenario containing course and player information
    RawGhostFile m_ghost;    ///< The ghost data for the race

    /// @addr{0x809BD728}
    /// @brief The singleton instance of the @ref RaceConfig
    static RaceConfig *s_instance;

    /// @brief Host-agnostic way of initializing @ref RaceConfig
    /// @details The type of the first player *must* be set to either Local or Ghost.
    /// - If the type is Ghost, @ref m_ghost must be set to a decompressed ghost file.
    /// - If the type is Local, the race scenario's course and the first player's @ref Character,
    /// @ref Vehicle, and @ref Player::driftIsAuto must be set.
    static InitCallback s_onInitCallback;

    /// @brief The argument to pass to @ref s_onInitCallback
    /// @details This is expected to be reinterpret_casted.
    static void *s_onInitCallbackArg;
};

} // namespace System

} // namespace Kinoko
