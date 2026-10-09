#include "RaceManager.hh"

#include "game/system/CourseMap.hh"
#include "game/system/KPadDirector.hh"

namespace Kinoko::System {

/// @addr{0x80533ED8}
/// @brief Constructor
/// @details Sets @ref m_checkPointId, @ref m_raceCompletion, and @ref
/// m_checkpointStartLapCompletion to zero. Sets @ref m_checkpointFactor to `-1.0f` and @ref
/// m_lapCompletion to `0.999999f`. Sets @ref m_maxKcp based on the last key checkpoint's ID. Sets
/// @ref m_currentLap to zero, @ref m_maxLap to 1, and @ref m_drivingWrongWay to `false`. Finally,
/// gets a point to the player's @ref KPadPlayer instance, storing it to @ref m_inputs.
/// @note The base game implements a failsafe for setting @ref m_maxKcp to `-1` if the course `KMP`
/// file does not have any checkpoints or checkpaths. However, it is not yet known what the overall
/// effect this has on the behavior of the race.
RaceManager::Player::Player() {
    m_checkpointId = 0;
    m_raceCompletion = 0.0f;
    m_checkpointFactor = -1.0f;
    m_checkpointStartLapCompletion = 0.0f;
    m_lapCompletion = 0.999999f;

    auto *courseMap = CourseMap::Instance();

    if (courseMap->getCheckPointCount() > 0 && courseMap->getCheckPathCount() > 0) {
        m_maxKcp = courseMap->checkPoint()->lastKcpType();
    } else {
        m_maxKcp = -1;
    }

    m_currentLap = 0;
    m_maxLap = 1;
    m_drivingWrongWay = false;
    m_inputs = &KPadDirector::Instance()->playerInput();
}

/// @addr{0x80534194}
/// @brief Initializes the player instance
/// @details Sets up the player's starting checkpoint and respawn information based on the @ref
/// CourseMap.
/// @note The base game implements a failsafe for setting @ref m_jugemId to zero if the course `KMP`
/// file does not have any checkpoints or checkpaths. However, it is not yet known what the overall
/// effect this has on the behavior of the race.
void RaceManager::Player::init() {
    auto *courseMap = CourseMap::Instance();

    if (courseMap->getCheckPointCount() != 0 && courseMap->getCheckPathCount() != 0) {
        const EGG::Vector3f &pos = Kart::KartObjectManager::Instance()->object(0)->pos();
        f32 distanceRatio;
        s16 checkpointId = courseMap->findSector(pos, 0, distanceRatio);

        m_checkpointId = std::max<s16>(0, checkpointId);
        m_jugemId = courseMap->getCheckPoint(m_checkpointId)->jugemIndex();
    } else {
        m_jugemId = 0;
    }
}

/// @addr{0x80535304}
/// @brief Every frame, updates the player's race state
/// @details Calculates the player's current checkpoint, lap completion, race completion, and
/// whether they are driving the wrong way.
void RaceManager::Player::calc() {
    auto *courseMap = CourseMap::Instance();
    const auto *kart = Kart::KartObjectManager::Instance()->object(0);

    if (courseMap->getCheckPointCount() == 0 || courseMap->getCheckPathCount() == 0 ||
            kart->status().onBit(Kart::eStatus::BeforeRespawn)) {
        return;
    }

    f32 distanceRatio;
    s16 checkpointId = courseMap->findSector(kart->pos(), m_checkpointId, distanceRatio);

    if (checkpointId == -1) {
        return;
    }

    System::MapdataCheckPoint *checkpoint = nullptr;

    if (m_checkpointFactor < 0.0f || m_checkpointId != checkpointId) {
        checkpoint = calcCheckpoint(checkpointId, distanceRatio);
    } else {
        checkpoint = CourseMap::Instance()->getCheckPoint(m_checkpointId);
    }

    m_raceCompletion = static_cast<f32>(m_currentLap) +
            (m_checkpointStartLapCompletion + m_checkpointFactor * distanceRatio);
    m_raceCompletion = std::min(m_raceCompletion, static_cast<f32>(m_currentLap) + 0.99999f);

    const EGG::Vector3f &bodyFront = kart->bodyForward();
    if (bodyFront.x != 0.0f && bodyFront.z != 0.0f) {
        EGG::Vector2f frontXZ = EGG::Vector2f(bodyFront.x, bodyFront.z);
        frontXZ.normalise();
        m_drivingWrongWay = checkpoint->dir().dot(frontXZ) <= -0.5f;
    }
}

/// @addr{0x8053572C}
/// @brief Gets the lap split, which is the difference between the given lap and the previous one.
/// @param lap One-indexed lap.
/// @return The split timer.
/// @pre The lap index must be within the valid range of lap timers (1, 2, or 3).
Timer RaceManager::Player::getLapSplit(size_t lap) const {
    ASSERT(lap <= m_lapTimers.size());

    if (lap < 2) {
        return m_lapTimers[0];
    }

    const Timer &currentLap = m_lapTimers[lap - 1];
    const Timer &previousLap = m_lapTimers[lap - 2];
    if (!currentLap.valid || !previousLap.valid) {
        return Timer(std::numeric_limits<u16>::max(), 0, 0);
    }

    return currentLap - previousLap;
}

/// @addr{0x80534DF8}
/// @brief Calculates the checkpoint data for the player based on the given checkpoint ID and
/// the percentage of the checkpoint that has been traversed
/// @param checkpointId The ID of the checkpoint the user is current in this frame
/// @param distanceRatio The distance ratio within the checkpoint.
/// @return A pointer to the current checkpoint.
/// @details Computes the updated lap completion, change in lap completion, and the new checkpoint's
/// respawn ID. If the player crosses the finish line, the lap count is adjusted accordingly.
/// @note This function enforces what is referred to as the "95% Rule". If the player has traversed
/// more than 95% of a lap since the last time the lap completion was updated, the lap number will
/// decrement, meaning the lap will not count.
MapdataCheckPoint *RaceManager::Player::calcCheckpoint(u16 checkpointId, f32 distanceRatio) {
    auto *courseMap = CourseMap::Instance();

    u16 oldCheckpointId = m_checkpointId;
    m_checkpointId = checkpointId;

    f32 lapProportion = courseMap->checkPath()->lapProportion();
    MapdataCheckPath *checkPath = courseMap->checkPath()->findCheckpathForCheckpoint(checkpointId);
    m_checkpointFactor = checkPath->invCount() * lapProportion;

    m_checkpointStartLapCompletion = static_cast<f32>(checkPath->depth()) * lapProportion +
            (m_checkpointFactor * static_cast<f32>(checkpointId - checkPath->start()));

    f32 newLapCompletion = m_checkpointStartLapCompletion + distanceRatio * m_checkpointFactor;
    f32 deltaLapCompletion = m_lapCompletion - newLapCompletion;

    MapdataCheckPoint *newCheckpoint = courseMap->getCheckPoint(checkpointId);
    const MapdataCheckPoint *oldCheckpoint = courseMap->getCheckPoint(oldCheckpointId);

    s8 newJugemIdx = newCheckpoint->jugemIndex();
    if (newJugemIdx >= 0) {
        m_jugemId = newJugemIdx;
    }

    if (!newCheckpoint->isNormalCheckpoint()) {
        if (newCheckpoint->type() > m_maxKcp) {
            m_maxKcp = newCheckpoint->type();
        } else if (m_maxKcp == courseMap->checkPoint()->lastKcpType()) {
            if ((newCheckpoint->isFinishLine() &&
                        AreCheckpointsSubsequent(oldCheckpoint, checkpointId)) ||
                    deltaLapCompletion > 0.95f) {
                incrementLap();
            }
        }
    }

    if ((oldCheckpoint->isFinishLine() &&
                AreCheckpointsSubsequent(newCheckpoint, oldCheckpointId)) ||
            deltaLapCompletion < -0.95f) {
        decrementLap();
    }

    m_lapCompletion = newLapCompletion;

    return newCheckpoint;
}

/// @addr{0x80534D6C}
/// @brief Decreases the player's lap by one
/// @details This occurs when the player has driven backwards across the finish line, or if the 95%
/// Rule has been violated. @see RaceManager::Player::calcCheckpoint()
void RaceManager::Player::decrementLap() {
    auto *courseMap = CourseMap::Instance();
    if (courseMap->getCheckPointCount() > 0 && courseMap->getCheckPathCount() > 0) {
        m_maxKcp = courseMap->checkPoint()->lastKcpType();
    } else {
        m_maxKcp = -1;
    }

    --m_currentLap;
}

/// @addr{0x805349B8}
/// @brief Increases the player's lap by one when the player crosses the finish line
/// @details Computes the lap time by determining how far the player passed beyond the checkpoint
/// entry boundary on the current frame. If the player has finished all laps, calls @ref endRace().
void RaceManager::Player::incrementLap() {
    m_maxKcp = 0;
    if (++m_currentLap <= m_maxLap) {
        return;
    }

    const auto *kart = Kart::KartObjectManager::Instance()->object(0);
    u16 addMs = CourseMap::Instance()->getCheckPointEntryOffsetMs(m_checkpointId, kart->pos(),
            kart->prevPos());

    const Timer &currentTimer = RaceManager::Instance()->timerManager().currentTimer();
    Timer timer = currentTimer + static_cast<f32>(addMs);

    // TODO: Handle this case more gracefully
    ASSERT(static_cast<size_t>(m_maxLap - 1) < m_lapTimers.size());
    m_lapTimers[m_maxLap - 1] = timer;

    if (m_maxLap >= 3) {
        endRace(timer);
    } else {
        m_maxLap = m_currentLap;
    }
}

/// @addr{0x805362DC}
/// @brief Fetches the starting position and rotation for the player
/// @param pos Out parameter that will be set to the starting position of the player's kart
/// @param angles Out parameter that will be set to the starting rotation of the player's kart
/// @todo When expanding to other gamemodes, we will need to pass the player index
void RaceManager::findKartStartPoint(EGG::Vector3f &pos, EGG::Vector3f &angles) {
    u32 placement = 1;
    u32 playerCount = 1;
    u32 startPointIdx = 0;

    const MapdataStartPoint *kartpoint = CourseMap::Instance()->getStartPoint(startPointIdx);

    if (kartpoint) {
        kartpoint->findKartStartPoint(pos, angles, placement - 1, playerCount);
    } else {
        pos.setZero();
        angles = EGG::Vector3f::ex;
    }
}

/// @addr{0x805331B4}
/// @brief Every frame, updates the timer, computes the player's race state, and manages the race
/// stage transitions
/// @details When the countdown begins, signals to @ref KPadDirector::startGhostProxies() that
/// inputs should start being read from the ghost `.rkg` input file.
void RaceManager::calc() {
    constexpr u16 STAGE_INTRO_DURATION = 172;

    m_timerManager.calc();
    m_player.calc();

    switch (m_stage) {
    case Stage::Intro:
        if (++m_introTimer >= STAGE_INTRO_DURATION) {
            m_stage = Stage::Countdown;
            KPadDirector::Instance()->startGhostProxies();
        }
        break;
    case Stage::Countdown:
        if (++m_timer >= STAGE_COUNTDOWN_DURATION) {
            m_timerManager.setStarted(true);
            m_stage = Stage::Race;
        }
        break;
    case Stage::Race:
        ++m_timer;
        break;
    default:
        break;
    }
}

/// @addr{0x8053621C}
/// @brief Retrieves the respawn point associated with the player's current respawn ID
/// @return A pointer to the corresponding @ref MapdataJugemPoint, or `nullptr` if no valid respawn
/// point exists.
const MapdataJugemPoint *RaceManager::jugemPoint() const {
    s8 jugemId = std::max<s8>(m_player.jugemId(), 0);
    return System::CourseMap::Instance()->getJugemPoint(static_cast<u16>(jugemId));
}

RaceManager *RaceManager::s_instance = nullptr;

} // namespace Kinoko::System
