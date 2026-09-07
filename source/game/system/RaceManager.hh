#pragma once

#include "game/system/KPadController.hh"
#include "game/system/Random.hh"
#include "game/system/map/MapdataCheckPoint.hh"
#include "game/system/map/MapdataJugemPoint.hh"

namespace Kinoko {

namespace Host {

class Context;

} // namespace Host

namespace System {

/// @addr{0x809BD730}
/// @brief Acts as the interface between the physics engine and CourseMap. Also manages the timers
/// that track the stages of a race.
/// @details The physics engine leverages this class in order to determine what stage of the race
/// we're in, as that affects several things like acceleration. This class also retrieves the player
/// start position from @ref CourseMap and communicates it to the physics engine.
class RaceManager : EGG::Disposer {
    /// @brief Grants access to the singleton so a @ref Host::Context can restore the instance's
    /// state on context switch
    friend class Host::Context;

public:
    /// @brief Represents a player participating in the race
    /// @details This class keeps track of the player's checkpoint progress, inputs, and lap timers.
    class Player {
    public:
        Player();

        /// @brief Default destructor
        ~Player() = default;

        void init();
        void calc();

        [[nodiscard]] Timer getLapSplit(size_t idx) const;

        /// @beginGetters
        [[nodiscard]] u16 checkpointId() const {
            return m_checkpointId;
        }

        [[nodiscard]] f32 raceCompletion() const {
            return m_raceCompletion;
        }

        [[nodiscard]] s8 jugemId() const {
            return m_jugemId;
        }

        [[nodiscard]] bool drivingWrongWay() const {
            return m_drivingWrongWay;
        }

        [[nodiscard]] const std::array<Timer, 3> &lapTimers() const {
            return m_lapTimers;
        }

        [[nodiscard]] const Timer &lapTimer(size_t idx) const {
            ASSERT(idx < m_lapTimers.size());
            return m_lapTimers[idx];
        }

        [[nodiscard]] const Timer &raceTimer() const {
            return m_raceTimer;
        }

        [[nodiscard]] const KPad *inputs() const {
            return m_inputs;
        }
        /// @endGetters

    private:
        MapdataCheckPoint *calcCheckpoint(u16 checkpointId, f32 distanceRatio);

        /// @addr{Inlined in 0x80534DF8}
        /// @brief Checks if two checkpoints are adjacent to one another
        /// @param checkpoint The current checkpoint
        /// @param nextCheckpointId The ID of the next checkpoint
        /// @return `true` if the next checkpoint is adjacent to the current checkpoint, `false`
        /// otherwise
        [[nodiscard]] static bool AreCheckpointsSubsequent(const MapdataCheckPoint *checkpoint,
                u16 nextCheckpointId) {
            for (size_t i = 0; i < checkpoint->nextCount(); ++i) {
                if (nextCheckpointId == checkpoint->nextPoint(i)->id()) {
                    return true;
                }
            }

            return false;
        }

        void decrementLap();
        void incrementLap();
        void endRace(const Timer &finishTime);

        u16 m_checkpointId;                 ///< The ID of the current checkpoint the player is in
        f32 m_raceCompletion;               ///< Represents the player's progress through the race
        f32 m_checkpointFactor;             ///< The proportion of a lap for the current checkpoint
        f32 m_checkpointStartLapCompletion; ///< Lap completion at the current checkpoint's start
        f32 m_lapCompletion;                ///< The percentage of the lap completed by the player
        s8 m_jugemId;                       ///< Respawn ID associated with the current checkpoint
        s16 m_currentLap;                   ///< The current lap the player is on
        s8 m_maxLap;                        ///< The number of total laps in the race
        s8 m_maxKcp;                        ///< The last key checkpoint's ID
        bool m_drivingWrongWay;             ///< Set if the player is driving the wrong way
        std::array<Timer, 3> m_lapTimers;   ///< Array of @ref Timer objects for each lap
        Timer m_raceTimer;                  ///< The overall race timer
        const KPad *m_inputs;               ///< Pointer to the player's input state manager
    };

    /// @brief Describes the different "phases"" of the race
    enum class Stage {
        Intro = 0,        ///< Fade-in and camera pan
        Countdown = 1,    ///< Countdown before the race starts
        Race = 2,         ///< The race has started
        FinishLocal = 3,  ///< The player has finished the race
        FinishGlobal = 4, ///< All players have finished the race
    };

    /// @addr{0x80532F88}
    /// @brief Initializes all player instances
    /// @details For Kinoko, we only have one player, so this function initializes that single
    /// player instance.
    void init() {
        m_player.init();
    }

    void findKartStartPoint(EGG::Vector3f &pos, EGG::Vector3f &angles);

    /// @addr{0x80533C6C}
    /// @brief Ends the race for the specified player
    /// @details For Kinoko, we only have one player, so this function simply sets the race stage to
    /// @ref Stage::FinishGlobal.
    void endPlayerRace(u32 /*idx*/) {
        m_stage = Stage::FinishGlobal;
    }

    void calc();

    /// @addr{0x80536230}
    /// @brief Checks if the specified race stage has been reached
    /// @param stage The race stage to check
    /// @return `true` if the current race stage is greater than or equal to the specified stage,
    /// `false` otherwise
    [[nodiscard]] bool isStageReached(Stage stage) const {
        return static_cast<std::underlying_type_t<Stage>>(m_stage) >=
                static_cast<std::underlying_type_t<Stage>>(stage);
    }

    [[nodiscard]] MapdataJugemPoint *jugemPoint() const;

    /// @beginGetters
    /// @addr{0x80533090}
    /// @brief Returns the frames remaining for the @ref Stage::Countdown phase of the race
    /// @return The number of frames remaining for the countdown phase
    [[nodiscard]] int getCountdownTimer() const {
        return STAGE_COUNTDOWN_DURATION - m_timer;
    }

    [[nodiscard]] Random &random() {
        return m_random;
    }

    [[nodiscard]] const Player &player() const {
        return m_player;
    }

    [[nodiscard]] const TimerManager &timerManager() const {
        return m_timerManager;
    }

    [[nodiscard]] Stage stage() const {
        return m_stage;
    }

    [[nodiscard]] u32 timer() const {
        return m_timer;
    }
    /// @endGetters

    /// @addr{0x80532084}
    /// @brief Creates the singleton instance of the @ref RaceManager
    /// @return A pointer to the newly created @ref RaceManager instance
    static RaceManager *CreateInstance() {
        ASSERT(!s_instance);
        s_instance = EGG::egg_new<RaceManager>();
        return s_instance;
    }

    /// @addr{0x805320D4}
    /// @brief Destroys the singleton instance of the @ref RaceManager
    static void DestroyInstance() {
        ASSERT(s_instance);
        auto *instance = s_instance;
        s_instance = nullptr;
        EGG::egg_delete(instance);
    }

    /// @brief Returns the singleton instance of the @ref RaceManager
    /// @return A pointer to the singleton instance of the @ref RaceManager
    [[nodiscard]] static RaceManager *Instance() {
        return s_instance;
    }

private:
    EGG_NEW_DELETE_FRIEND

    RaceManager();
    ~RaceManager() override;

    Random m_random;             ///< The state of the random number generator
    Player m_player;             ///< The player object representing the local player
    TimerManager m_timerManager; ///< The manager object responsible for handling race timers
    Stage m_stage;               ///< The current "phase" of the race
    u16 m_introTimer;            ///< Number of frames elapsed before the countdown
    u32 m_timer;                 ///< Number of frames elapsed since the countdown began

    static constexpr u16 STAGE_COUNTDOWN_DURATION = 240; ///< Duration of the countdown in frames
    static constexpr u32 RNG_SEED = 0x74A1B095; ///< Initial value used for RNG (see @ref Random)

    static RaceManager *s_instance; ///< @addr{0x809BD730}
};

} // namespace System

} // namespace Kinoko
