#pragma once

#include "game/system/KPadController.hh"
#include "game/system/RaceMode.hh"
#include <game/system/map/MapdataCheckPoint.hh>

#include <egg/math/Vector.hh>
#include <vector>

namespace System {

class RaceManagerPlayer {
public:
    RaceManagerPlayer(u8 idx, u8 lapCount);
    virtual ~RaceManagerPlayer() {}

    void init();
    void calc();
    [[nodiscard]] const KPad *inputs() const;

    f32 raceCompletion() const {
        return m_raceCompletion;
    }
    u16 checkpointId() const {
        return m_checkpointId;
    }
    u16 currentLap() const {
        return m_currentLap;
    }
    s8 respawn() const {
        return m_respawn;
    }

private:
    void decrementLap();
    void endLap();
    MapdataCheckPoint *calcCheckpoint(u16 checkpointId, f32 checkpointCompletion, bool isRemote);

    s8 m_playerIdx;
    u16 m_checkpointId;
    f32 m_raceCompletion;
    f32 m_raceCompletionMax;
    f32 m_checkpointFactor;
    f32 m_checkpointStartLapCompletion;
    f32 m_lapCompletion;
    s8 m_respawn;
    u16 m_battleScore;
    s16 m_currentLap;
    s8 m_maxLap;
    s8 m_currentKcp;
    s8 m_maxKcp;
    u32 m_frameCounter;
    /// @name raceManagerPlayerFlags
    /// The bitfield at offset 0x38.
    /// @{
    bool m_bInRace;          ///< field 0x01
    bool m_bFinished;        ///< field 0x02
    bool m_bDrivingWrongWay; ///< field 0x04
    bool m_bStopped;         ///< field 0x20
    /// @}
    std::vector<Timer> m_lapFinishTimes;
    Timer *m_raceFinishTime;
    const KPad *m_inputs;
};

/// @addr{0x809BD730}
/// @brief Manages the timers that track the stages of a race.
/// Also acts as the interface between the physics engine and CourseMap.
/// @details The physics engine leverages the RaceManager in order to determine what stage of the
/// race we're in, as that affects several things like acceleration. This class also retrieves the
/// player start position from CourseMap and communicates it to the physics engine.
/// @nosubgrouping
class RaceManager : EGG::Disposer {
public:
    enum class Stage {
        Intro = 0,
        Countdown = 1,
        Race = 2,
        FinishLocal = 3,
        FinishGlobal = 4,
    };

    void init();

    void findKartStartPoint(EGG::Vector3f &pos, EGG::Vector3f &angles);
    const MapdataJugemPoint *jugemPoint();

    void calc();

    [[nodiscard]] bool isStageReached(Stage stage) const;

    /// @beginGetters
    [[nodiscard]] int getCountdownTimer() const;
    [[nodiscard]] const RaceManagerPlayer &player() const;
    [[nodiscard]] Stage stage() const;
    /// @endGetters

    static RaceManager *CreateInstance();
    [[nodiscard]] static RaceManager *Instance();
    static void DestroyInstance();

private:
    RaceManager();
    ~RaceManager() override;

    RaceManagerPlayer m_player;
    RaceMode m_raceMode;
    Stage m_stage;
    u16 m_introTimer;
    u32 m_timer;

    static constexpr u16 STAGE_COUNTDOWN_DURATION = 240;

    static RaceManager *s_instance; ///< @addr{0x809BD730}
};

} // namespace System
