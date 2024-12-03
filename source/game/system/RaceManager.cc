#include "RaceManager.hh"

#include "game/system/CourseMap.hh"
#include "game/system/KPadDirector.hh"
#include "game/system/map/MapdataStartPoint.hh"
#include <Logger.hh>
#include <game/kart/KartObjectManager.hh>
#include <game/kart/KartState.hh>
#include <game/system/RaceConfig.hh>
#include <game/system/map/MapdataCheckPath.hh>
#include <game/system/map/MapdataCheckPoint.hh>

namespace System {

/// @addr{0x80532F88}
void RaceManager::init() {
    m_player.init();
}

/// @addr{0x805362DC}
/// @todo When expanding to other gamemodes, we will need to pass the player index
void RaceManager::findKartStartPoint(EGG::Vector3f &pos, EGG::Vector3f &angles) {
    u32 placement = 1;
    u32 playerCount = 1;
    u32 startPointIdx = 0;

    MapdataStartPoint *kartpoint = CourseMap::Instance()->getStartPoint(startPointIdx);

    if (kartpoint) {
        kartpoint->findKartStartPoint(pos, angles, placement - 1, playerCount);
    } else {
        pos.setZero();
        angles = EGG::Vector3f::ex;
    }
}

/// @addr{0x80584334}
const MapdataJugemPoint *RaceManager::jugemPoint() {
    return m_raceMode.jugemPoint();
}

/// @addr{0x805331B4}
void RaceManager::calc() {
    constexpr u16 STAGE_INTRO_DURATION = 172;

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

/// @addr{0x80536230}
bool RaceManager::isStageReached(Stage stage) const {
    return static_cast<std::underlying_type_t<Stage>>(m_stage) >=
            static_cast<std::underlying_type_t<Stage>>(stage);
}

/// @addr{0x80533090}
int RaceManager::getCountdownTimer() const {
    return STAGE_COUNTDOWN_DURATION - m_timer;
}

const RaceManagerPlayer &RaceManager::player() const {
    return m_player;
}

RaceManager::Stage RaceManager::stage() const {
    return m_stage;
}

/// @addr{0x80532084}
RaceManager *RaceManager::CreateInstance() {
    ASSERT(!s_instance);
    s_instance = new RaceManager;
    return s_instance;
}

RaceManager *RaceManager::Instance() {
    return s_instance;
}

/// @addr{0x805320D4}
void RaceManager::DestroyInstance() {
    ASSERT(s_instance);
    auto *instance = s_instance;
    s_instance = nullptr;
    delete instance;
}

/// @addr{0x805327A0}
RaceManager::RaceManager()
    : m_player(0, 3), m_raceMode(this), m_stage(Stage::Intro), m_introTimer(0), m_timer(0) {}

/// @addr{0x80532E3C}
RaceManager::~RaceManager() = default;

/// @addr{0x80534D6C}
void RaceManagerPlayer::decrementLap() {
    if (m_bFinished) {
        return;
    }
    m_maxKcp = CourseMap::Instance()->lastKcpType();
    m_currentLap -= 1;
}

/// @addr{0x805349B8}
void RaceManagerPlayer::endLap() {
    if (m_bFinished) {
        return;
    }
    m_maxKcp = 0;
    m_currentLap += 1;
}

/// @addr{0x80535304}
void RaceManagerPlayer::calc() {
    if (m_bFinished) {
        m_frameCounter++;
    }
    auto *courseMap = CourseMap::Instance();
    auto *kart = Kart::KartObjectManager::Instance()->object(m_playerIdx);

    if (courseMap->getCheckPointCount() == 0 || courseMap->getCheckPathCount() == 0 ||
            kart->state()->isBeforeRespawn() || !m_bInRace) {
        return;
    }

    f32 checkpointCompletion;
    s16 checkpointId =
            courseMap->findSector(kart->pos(), m_checkpointId, checkpointCompletion);

    if (checkpointId == -1) {
        return;
    }

    if (m_checkpointFactor < 0 || m_checkpointId != checkpointId) {
        calcCheckpoint(checkpointId, checkpointCompletion, false);
    }

    m_raceCompletion = static_cast<f32>(m_currentLap) +
            (m_checkpointStartLapCompletion + m_checkpointFactor * checkpointCompletion);
    m_raceCompletion = std::min(m_raceCompletion, m_currentLap + 0.99999f);
    m_raceCompletionMax = std::max(m_raceCompletionMax, m_raceCompletion);
}

/// @brief whether @param nextCheckpointId is directly after @param checkpoint
/// @addr{Inlined in 0x80534DF8}
bool areCheckpointsSubsequent(MapdataCheckPoint *checkpoint, u16 nextCheckpointId) {
    for (size_t i = 0; i < checkpoint->nextCount(); i++) {
        if (nextCheckpointId == checkpoint->nextPoint(i)->id()) {
            return true;
        }
    }

    return false;
}

/// @addr{0x80534DF8}
MapdataCheckPoint *RaceManagerPlayer::calcCheckpoint(u16 checkpointId, f32 checkpointCompletion,
        bool /* isRemote */) {
    auto courseMap = CourseMap::Instance();
    u16 oldCheckpointId = m_checkpointId;
    m_checkpointId = checkpointId;
    f32 lapProportion = courseMap->checkPath()->lapProportion();
    MapdataCheckPath *checkPath = courseMap->checkPath()->findCheckpathForCheckpoint(checkpointId);
    // rougly, the proportion of a lap a checkpoint in `checkPath` is
    f32 checkpointFactor = checkPath->oneOverCount() * lapProportion;
    m_checkpointFactor = checkpointFactor;
    // crude lap completion only respecting checkpaths
    auto ckpthLapCompletion = checkPath->depth() * lapProportion;
    // unlike `depth` which measures depth (in checkpaths) around the course, this measures depth
    // (in chekpoints) through the checkpath.
    auto depthIntoCheckPath = checkpointId - checkPath->start();
    m_checkpointStartLapCompletion = ckpthLapCompletion + (m_checkpointFactor * depthIntoCheckPath);
    f32 newLapCompletion = m_checkpointStartLapCompletion + checkpointCompletion * checkpointFactor;
    f32 dLapCompletion = m_lapCompletion - newLapCompletion;
    m_lapCompletion = newLapCompletion;

    MapdataCheckPoint *newCheckpoint = courseMap->getCheckPoint(checkpointId);
    MapdataCheckPoint *oldCheckpoint = courseMap->getCheckPoint(oldCheckpointId);

    s8 respawn = newCheckpoint->jugemIndex();
    if (respawn >= 0) {
        m_respawn = respawn;
    }

    if (!newCheckpoint->isNormalCheckpoint()) {
        if (newCheckpoint->checkArea() > m_maxKcp) {
            m_maxKcp = newCheckpoint->checkArea();
        } else if (m_maxKcp == courseMap->lastKcpType()) {
            if ((newCheckpoint->isFinishLine() &&
                        areCheckpointsSubsequent(oldCheckpoint, checkpointId)) ||
                    dLapCompletion > 0.95f) {
                endLap();
            }
        }
        m_currentKcp = newCheckpoint->checkArea();
    }
    if ((newCheckpoint->isFinishLine() && areCheckpointsSubsequent(newCheckpoint, checkpointId)) ||
            dLapCompletion < -0.95f) {
        decrementLap();
    }
    return newCheckpoint;
}

/// @addr{0x80534194}
void RaceManagerPlayer::init() {
    auto courseMap = CourseMap::Instance();
    if (courseMap->getCheckPointCount() != 0 && courseMap->getCheckPathCount() != 0) {
        auto pos = Kart::KartObjectManager::Instance()->object(m_playerIdx)->pos();
        f32 checkpointCompletion;
        s16 sector = courseMap->findSector(pos, 0, checkpointCompletion);
        m_checkpointId = std::max<s16>(0, sector);
        auto *ckpt = courseMap->getCheckPoint(m_checkpointId);
        m_respawn = ckpt->jugemIndex();
    } else {
        m_respawn = 0;
    }
    m_frameCounter = 0;
    m_bInRace = true;
}

const KPad *RaceManagerPlayer::inputs() const {
    return m_inputs;
}

/// @addr{0x80533ED8}
RaceManagerPlayer::RaceManagerPlayer(u8 idx, u8 lapCount)
    : m_playerIdx(idx), m_checkpointId(0), m_raceCompletion(0.0f), m_checkpointFactor(-1.0f),
      m_checkpointStartLapCompletion(0.0f), m_lapCompletion(0.999999f), m_currentLap(0),
      m_maxLap(0), m_currentKcp(CourseMap::Instance()->lastKcpType()),
      m_maxKcp(CourseMap::Instance()->lastKcpType()), m_bFinished(false),
      m_lapFinishTimes(lapCount), m_inputs(&KPadDirector::Instance()->playerInput()) {}

RaceManager *RaceManager::s_instance = nullptr; ///< @addr{0x809BD730}

} // namespace System
