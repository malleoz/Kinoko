#pragma once

#include "game/system/Random.hh"
#include "game/system/TimerManager.hh"

#include <egg/core/Heap.hh>

namespace Kinoko {

namespace EGG {

struct Vector3f;

} // namespace EGG

namespace Host {

class Context;

} // namespace Host

namespace System {

class KPad;
class MapdataCheckPoint;
class MapdataJugemPoint;

/// @brief Acts as the interface between the physics engine and CourseMap. Also manages the timers
/// that track the stages of a race.
/// @details The physics engine leverages this class in order to determine what stage of the race
/// we're in, as that affects several things like acceleration. This class also retrieves the player
/// start position from @ref CourseMap and communicates it to the physics engine.
class RaceManager : EGG::Disposer {
    /// @brief Grants access to the singleton so a @ref Host::Context can restore the instance's
    /// state on context switch
    friend Host::Context;

public:
    /// @brief Represents a player participating in the race
    /// @details This class keeps track of the player's checkpoint progress, inputs, and lap timers.
    class Player {
    public:
        Player();
        ~Player();

        void init();
        void calc();

        [[nodiscard]] Timer getLapSplit(size_t idx) const;

        /// @beginGetters

        [[nodiscard]] u16 checkpointId() const;
        [[nodiscard]] f32 raceCompletion() const;
        [[nodiscard]] s8 jugemId() const;
        [[nodiscard]] bool drivingWrongWay() const;
        [[nodiscard]] const std::array<Timer, 3> &lapTimers() const;
        [[nodiscard]] const Timer &lapTimer(size_t idx) const;
        [[nodiscard]] const Timer &raceTimer() const;
        [[nodiscard]] const KPad *inputs() const;

        /// @endGetters

    private:
        MapdataCheckPoint *calcCheckpoint(u16 checkpointId, f32 distanceRatio);

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

        [[nodiscard]] static bool AreCheckpointsSubsequent(const MapdataCheckPoint *checkpoint,
                u16 nextCheckpointId);
    };

    /// @brief Describes the different "phases" of the race
    enum class Stage {
        Intro = 0,        ///< Fade-in and camera pan
        Countdown = 1,    ///< Countdown before the race starts
        Race = 2,         ///< The race has started
        FinishLocal = 3,  ///< The player has finished the race
        FinishGlobal = 4, ///< All players have finished the race
    };

    void init();
    void findKartStartPoint(EGG::Vector3f &pos, EGG::Vector3f &angles);
    void calc();
    void endPlayerRace(u32 idx);

    [[nodiscard]] bool isStageReached(Stage stage) const;
    [[nodiscard]] const MapdataJugemPoint *jugemPoint() const;

    /// @beginGetters

    [[nodiscard]] int getCountdownTimer() const;
    [[nodiscard]] Random &random();
    [[nodiscard]] const Player &player() const;
    [[nodiscard]] const TimerManager &timerManager() const;
    [[nodiscard]] Stage stage() const;
    [[nodiscard]] u32 timer() const;

    /// @endGetters

    static RaceManager *CreateInstance();
    static void DestroyInstance();
    [[nodiscard]] static RaceManager *Instance();

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

    /// @addr{0x809BD730}
    /// @brief The singleton instance of the @ref RaceManager
    static RaceManager *s_instance;
};

} // namespace System

} // namespace Kinoko
