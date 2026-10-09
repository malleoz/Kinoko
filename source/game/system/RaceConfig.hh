#pragma once

#include "game/system/GhostFile.hh"

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
    friend class Host::Context;

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
        void init();

        std::array<Player, MAX_PLAYERS> players; ///< The array of @ref Player objects in the race
        u8 playerCount;                          ///< The number of players in the race
        Course course;                           ///< The course for the race
    };

    /// @brief Describes the type of callback function used to initialize the RaceConfig instance
    /// @details The callback function is called after the RaceConfig instance has been initialized
    typedef std::function<void(RaceConfig *, void *)> InitCallback;

    /// @addr{0x8052DD40}
    /// @brief Initializes the @ref Scenario data for the race
    /// @details Simply calls @ref Scenario::init() on @ref m_raceScenario.
    void init() {
        m_raceScenario.init();
    }

    void initRace();
    void initControllers();
    void initGhost();

    /// @beginSetters

    /// @brief Sets the ghost data for the race
    /// @param rkg Pointer to the raw (possibly uncompressed) ghost `.rkg` file buffer
    /// @details Creates a @ref RawGhostFile instance from the provided raw ghost file buffer and
    /// stores it in @ref m_ghost.
    void setGhost(const u8 *rkg) {
        m_ghost = RawGhostFile(rkg);
    }

    /// @endSetters

    /// @beginGetters

    /// @brief Gets the current race scenario
    /// @return A non-const ref to the current race scenario, @ref m_raceScenario
    [[nodiscard]] Scenario &raceScenario() {
        return m_raceScenario;
    }

    /// @brief Gets the current race scenario
    /// @return A const ref to the current race scenario, @ref m_raceScenario
    [[nodiscard]] const Scenario &raceScenario() const {
        return m_raceScenario;
    }

    /// @endGetters

    /// @brief Sets the initialization callback for the RaceConfig instance
    /// @param callback The callback function to call after initialization
    /// @param arg The argument(s) to pass to the callback function
    static void RegisterInitCallback(const InitCallback &callback, void *arg) {
        s_onInitCallback = callback;
        s_onInitCallbackArg = arg;
    }

    /// @addr{0x8052FE58}
    /// @brief Creates the singleton instance of the @ref RaceConfig
    /// @return A pointer to the newly created @ref RaceConfig instance
    /// @pre The singleton instance of the RaceConfig must not already exist.
    static RaceConfig *CreateInstance() {
        ASSERT(!s_instance);
        s_instance = EGG::egg_new<RaceConfig>();
        return s_instance;
    }

    /// @addr{0x8052FFE8}
    /// @brief Destroys the singleton instance of the @ref RaceConfig
    /// @pre The singleton instance of the RaceConfig must already exist.
    static void DestroyInstance() {
        ASSERT(s_instance);
        auto *instance = s_instance;
        s_instance = nullptr;
        EGG::egg_delete(instance);
    }

    /// @brief Returns the singleton instance of the @ref RaceConfig
    /// @return A pointer to the singleton instance of the @ref RaceConfig
    [[nodiscard]] static RaceConfig *Instance() {
        return s_instance;
    }

private:
    EGG_NEW_DELETE_FRIEND

    /// @addr{0x8053015C}
    /// @brief Private default constructor
    RaceConfig() = default;

    /// @addr{0x80530038}
    /// @brief Private virtual destructor
    ~RaceConfig() override {
        if (s_instance) {
            s_instance = nullptr;
            WARN("RaceConfig instance not explicitly handled!");
        }
    }

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
