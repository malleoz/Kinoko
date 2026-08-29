#include "KartMove.hh"

#include "game/kart/KartSub.hh"
#include "game/kart/KartSuspension.hh"

#include "game/field/CollisionDirector.hh"
#include "game/field/ObjectDirector.hh"

#include "game/item/ItemDirector.hh"

#include "game/system/CourseMap.hh"
#include "game/system/RaceManager.hh"

namespace Kinoko::Kart {

/// @addr{0x80577FC4}
/// @brief Constructor
KartMove::KartMove() : m_smoothedUp(EGG::Vector3f::ey), m_scale(1.0f, 1.0f, 1.0f) {
    m_totalScale = 1.0f;
    m_hitboxScale = 1.0f;
    m_shockSpeedMultiplier = 1.0f;
    m_invScale = 1.0f;
    m_padType.makeAllZero();
    m_flags.makeAllZero();
    m_jump = nullptr;
}

/// @addr{0x80587B78}
/// @brief Virtual destructor that destroys the underlying kart subsystems
KartMove::~KartMove() {
    EGG::egg_delete(m_jump);
    EGG::egg_delete(m_halfPipe);
    EGG::egg_delete(m_kartScale);
}

/// @addr{0x8057A8B4}
void KartMove::calcTurn() {
    m_realTurn = 0.0f;
    m_rawTurn = 0.0f;

    auto &status = KartObjectProxy::status();

    if (status.onBit(eStatus::InAction, eStatus::CannonStart, eStatus::InCannon,
                eStatus::OverZipper)) {
        return;
    }

    if (status.onBit(eStatus::BeforeRespawn)) {
        return;
    }

    if (status.offBit(eStatus::Hop) || m_hopStickX == 0) {
        m_rawTurn = -state()->stickX();
        if (status.onBit(eStatus::JumpPadMushroom)) {
            m_rawTurn *= 0.35f;
        } else if (status.onBit(eStatus::AirtimeOver20)) {
            m_rawTurn *= 0.01f;
        }
    } else {
        m_rawTurn = static_cast<f32>(m_hopStickX);
    }

    f32 reactivity;
    if (state()->isDrifting()) {
        reactivity = param()->stats().driftReactivity;
    } else {
        reactivity = param()->stats().handlingReactivity;
    }

    m_weightedTurn = m_rawTurn * reactivity + m_weightedTurn * (1.0f - reactivity);
    m_weightedTurn = std::max(-1.0f, std::min(1.0f, m_weightedTurn));

    m_realTurn = m_weightedTurn;

    if (!state()->isDrifting()) {
        return;
    }

    m_realTurn = (m_weightedTurn + static_cast<f32>(m_hopStickX)) * 0.5f;
    m_realTurn = m_realTurn * 0.8f + 0.2f * static_cast<f32>(m_hopStickX);
    m_realTurn = std::max(-1.0f, std::min(1.0f, m_realTurn));
}

/// @addr{0x8057829C}
void KartMove::setTurnParams() {
    static constexpr std::array<DriftingParameters, 3> DRIFTING_PARAMS_ARRAY = {{
            {10.0f, 0.5f, 0.5f, 1.0f},
            {10.0f, 0.5f, 0.5f, 0.2f},
            {10.0f, 0.22f, 0.5f, 0.2f},
    }};

    reset(false, false);
    m_dir = bodyForward();
    m_lastDir = m_dir;
    m_intVelDir = m_dir;
    m_landingDir = m_dir;
    m_smoothedForward = m_dir;
    m_outsideDriftLastDir = m_dir;
    m_driftingParams = &DRIFTING_PARAMS_ARRAY[static_cast<u32>(vehicleType())];
    m_kartScale->reset();
}

/// @addr{0x805784D4}
/// @brief Resets the kart's movement state to its initial conditions
/// @param preserveScale Whether to preserve the kart's current scale
/// @param preserveFloorCount Whether to preserve the number of colliding floors
void KartMove::reset(bool preserveScale, bool preserveFloorCount) {
    m_lastSpeed = 0.0f;
    m_baseSpeed = param()->stats().speed;
    m_jumpPadSoftSpeedLimit = m_softSpeedLimit = param()->stats().speed;
    m_speed = 0.0f;
    setKartSpeedLimit();
    m_acceleration = 0.0f;
    m_speedDragMultiplier = 1.0f;
    m_up = EGG::Vector3f::ey;
    m_smoothedUp = EGG::Vector3f::ey;
    m_smoothedForward = EGG::Vector3f::ez;
    m_intVelDir = EGG::Vector3f::ez;
    m_lastDir = EGG::Vector3f::ez;
    m_dir = EGG::Vector3f::ez;
    m_landingDir = EGG::Vector3f::ez;
    m_dirDiff = EGG::Vector3f::zero;
    m_hasLandingDir = false;
    m_outsideDriftAngle = 0.0f;
    m_landingAngle = 0.0f;
    m_outsideDriftLastDir = EGG::Vector3f::ez;
    m_speedRatio = 0.0f;
    m_speedRatioCapped = 0.0f;
    m_kclSpeedFactor = 1.0f;
    m_kclRotFactor = 1.0f;
    m_kclWheelSpeedFactor = 1.0f;
    m_kclWheelRotFactor = 1.0f;

    if (!preserveFloorCount) {
        m_floorCollisionCount = 0;
    }

    m_hopStickX = 0;
    m_hopFrame = 0;
    m_hopUp = EGG::Vector3f::ey;
    m_hopDir = EGG::Vector3f::ez;
    m_divingRot = 0.0f;
    m_standStillBoostRot = 0.0f;
    m_driftState = DriftState::NotDrifting;
    m_smtCharge = 0;
    m_mtCharge = 0;
    m_outsideDriftBonus = 0.0f;
    m_boost.init();
    m_zipperBoostTimer = 0;
    m_zipperBoostDuration = 0;
    m_reject.reset();
    m_offroadInvincibilityTimer = 0;
    m_ssmtCharge = 0;
    m_ssmtLeewayTimer = 0;
    m_ssmtDisableAccelTimer = 0;
    m_nonZipperAirtime = 0;
    m_realTurn = 0.0f;
    m_weightedTurn = 0.0f;

    if (!preserveScale) {
        m_scale.set(1.0f);
        m_totalScale = 1.0f;
        m_hitboxScale = 1.0f;
        m_shockSpeedMultiplier = 1.0f;
        m_invScale = 1.0f;
        m_mushroomBoostTimer = 0;
        m_shockTimer = 0;
        m_crushTimer = 0;
    }

    m_jumpPadMinSpeed = 0.0f;
    m_jumpPadMaxSpeed = 0.0f;
    m_jumpPadBoostMultiplier = 0.0f;
    m_jumpPadProperties = nullptr;
    m_rampBoostTimer = 0;
    m_autoDriftAngle = 0.0f;
    m_autoDriftStartFrameCounter = 0;

    m_cannonLength = 0.0f;
    m_cannonEntryPos.setZero();
    m_cannonDir.setZero();
    m_cannonOrthog.setZero();
    m_cannonProgress.setZero();

    m_hopVelY = 0.0f;
    m_hopPosY = 0.0f;
    m_hopGravity = 0.0f;
    m_timeInRespawn = 0;
    m_respawnPreLandTimer = 0;
    m_respawnPostLandTimer = 0;
    m_respawnBoostTimer = 0;
    m_bumpTimer = 0;
    m_drivingDirection = DrivingDirection::Forwards;
    m_padType.makeAllZero();
    m_flags.makeAllZero();
    m_jump->reset();
    m_halfPipe->reset();
    m_rawTurn = 0.0f;
}

/// @addr{0x8058348C}
void KartMove::clear() {
    auto &status = KartObjectProxy::status();

    if (status.onBit(eStatus::OverZipper)) {
        status.setBit(eStatus::ActionMidZipper);
    }

    clearBoost();
    clearJumpPad();
    clearRampBoost();
    clearZipperBoost();
    clearSsmt();
    clearOffroadInvincibility();
    m_halfPipe->end(false);
    m_jump->end();
    clearRejectRoad();
}

/// @addr{0x80584044}
/// @brief Initializes the kart's position and rotation. Calls tire suspension initializers.
void KartMove::setInitialPhysicsValues(const EGG::Vector3f &position, const EGG::Vector3f &angles) {
    EGG::Quatf quaternion = EGG::Quatf::FromRPY(angles * DEG2RAD);
    EGG::Vector3f newPos = position;
    Field::CollisionInfo info;
    Field::KCLTypeMask kcl_flags = KCL_NONE;

    bool bColliding = Field::CollisionDirector::Instance()->checkSphereFullPush(100.0f, newPos,
            EGG::Vector3f::inf, KCL_ANY, &info, &kcl_flags, 0);

    if (bColliding && (kcl_flags & KCL_TYPE_FLOOR)) {
        newPos = newPos + info.tangentOff + (info.floorNrm * -100.0f);
        newPos += info.floorNrm * bsp().offsetY;
    }

    setPos(newPos);
    setRot(quaternion);

    sub()->initPhysicsValues();

    physics()->setPos(pos());
    physics()->setVelocity(dynamics()->velocity());

    m_landingDir = bodyForward();
    m_dir = bodyForward();
    m_smoothedForward = bodyForward();
    m_up = bodyUp();
    dynamics()->setUp(m_up);

    for (u16 tireIdx = 0; tireIdx < suspCount(); ++tireIdx) {
        suspension(tireIdx)->setInitialState();
    }
}

/// @addr{0x805788DC}
/// @brief Each frame, calculates the kart's movement.
/// @details Calls various functions to handle drifts, hops, boosts, scale modifiers, etc.
/// Afterwards, calculates the kart's speed and rotation.
void KartMove::calc() {
    auto &status = KartObjectProxy::status();

    if (status.onBit(eStatus::InRespawn)) {
        calcInRespawn();
        return;
    }

    dynamics()->resetInternalVelocity();
    m_burnout.calc();
    calcSsmt();
    m_halfPipe->calc();
    calcTop();
    tryEndJumpPad();
    calcRespawnBoost();
    calcSpecialFloor();
    m_jump->calc();

    m_bumpTimer = std::max(m_bumpTimer - 1, 0);

    calcAutoDrift();
    calcDirs();
    calcStickyRoad();
    calcOffroad();
    calcTurn();

    if (status.offBit(eStatus::AutoDrift)) {
        calcManualDrift();
    }

    calcWheelie();
    calcSsmtCharge();
    calcBoost();
    calcMushroomBoost();
    calcZipperBoost();
    calcShock();
    calcCrushed();
    calcScale();

    if (status.onBit(eStatus::InCannon)) {
        calcCannon();
    }

    calcOffroadInvincibility();
    calcVehicleSpeed();
    calcSpeed();
    calcRotation();
}

/// @addr{0x80584334}
/// @brief Called when the screen wipes to black during a respawn
/// @details Snaps the kart's position and rotation to the respawn point above the track and clears
/// the kart's item inventory. Also transitions from @enum eStatus::TriggerRespawn to @enum
/// eStatus::InRespawn.
void KartMove::calcRespawnTrigger() {
    constexpr float RESPAWN_HEIGHT = 700.0f;

    const auto *jugemPoint = System::RaceManager::Instance()->jugemPoint();
    const EGG::Vector3f &jugemPos = jugemPoint->pos();
    const EGG::Vector3f &jugemRot = jugemPoint->rot();

    EGG::Vector3f respawnPos = jugemPos;
    respawnPos.y += RESPAWN_HEIGHT;
    EGG::Vector3f respawnRot = EGG::Vector3f(0.0f, jugemRot.y, 0.0f);

    setInitialPhysicsValues(respawnPos, respawnRot);

    Item::ItemDirector::Instance()->kartItem(0).clear();

    status().resetBit(eStatus::TriggerRespawn).setBit(eStatus::InRespawn);
}

/// @addr{0x80579A50}
/// @brief Called while the kart is held by Lakitu after a respawn
/// @details The kart gradually lowers by 1.5 units per frame while held by Lakitu. When the 110
/// frame respawn timer expires, Lakitu lets go the kart and gravity takes effect again.
void KartMove::calcInRespawn() {
    constexpr f32 LAKITU_VELOCITY = 1.5f;
    constexpr u16 RESPAWN_DURATION = 110;

    auto &status = KartObjectProxy::status();

    if (status.offBit(eStatus::InRespawn)) {
        return;
    }

    EGG::Vector3f newPos = pos();
    newPos.y -= LAKITU_VELOCITY;
    dynamics()->setPos(newPos);
    dynamics()->setNoGravity(true);

    if (++m_timeInRespawn > RESPAWN_DURATION) {
        status.resetBit(eStatus::InRespawn).setBit(eStatus::AfterRespawn, eStatus::RespawnKillY);
        m_timeInRespawn = 0;
        m_flags.setBit(eFlags::Respawned);
        dynamics()->setNoGravity(false);
    }
}

/// @addr{0x80581C90}
/// @brief Handles functionality for boosting after landing from a respawn
/// @details Implements two leniency timers such that there is an 9 frame window centered on the
/// frame the kart lands on the ground in which pressing the accelerate button will result in a 30
/// frame boost.
void KartMove::calcRespawnBoost() {
    constexpr s16 RESPAWN_BOOST_DURATION = 30;
    constexpr s16 RESPAWN_BOOST_INPUT_LENIENCY = 4;

    auto &status = KartObjectProxy::status();

    if (status.onBit(eStatus::AfterRespawn)) {
        if (status.onBit(eStatus::TouchingGround)) {
            if (m_respawnPreLandTimer > 0) {
                if (status.offBit(eStatus::BeforeRespawn, eStatus::InAction)) {
                    activateBoost(KartBoost::Type::MiniTurbo, RESPAWN_BOOST_DURATION);
                    m_respawnBoostTimer = RESPAWN_BOOST_DURATION;
                }
            } else {
                m_respawnPostLandTimer = RESPAWN_BOOST_INPUT_LENIENCY;
            }

            status.resetBit(eStatus::AfterRespawn);
            m_flags.resetBit(eFlags::Respawned);
        }

        m_respawnPreLandTimer = std::max(0, m_respawnPreLandTimer - 1);

        if (m_flags.onBit(eFlags::Respawned) && status.onBit(eStatus::AccelerateStart)) {
            m_respawnPreLandTimer = RESPAWN_BOOST_INPUT_LENIENCY;
            m_flags.resetBit(eFlags::Respawned);
        }
    } else {
        if (m_respawnPostLandTimer > 0) {
            if (status.onBit(eStatus::AccelerateStart)) {
                if (status.offBit(eStatus::BeforeRespawn, eStatus::InAction)) {
                    activateBoost(KartBoost::Type::MiniTurbo, RESPAWN_BOOST_DURATION);
                    m_respawnBoostTimer = RESPAWN_BOOST_DURATION;
                }

                m_respawnPostLandTimer = 0;
            }

            m_respawnPostLandTimer = std::max(0, m_respawnPostLandTimer - 1);
        } else {
            status.resetBit(eStatus::RespawnKillY);
        }
    }

    m_respawnBoostTimer = std::max(0, m_respawnBoostTimer - 1);
}

/// @addr{0x8057D398}
/// @brief Calculates the up vector of the kart's orientation
/// @details If the kart landed on the ground, the up vector is set to the floor collision normal
/// and the landing direction is computed by projecting the facing direction onto the plane of the
/// collision normal. Otherwise, if the kart is on the ground, checks if the kart recently left a
/// trickable surface to avoid any jittery orientation flicker. The smoothed up vector normally has
/// an interpolation rate of 0.8f, but if the kart is on a half-pipe ramp or a few other conditions
/// are met, the interpolation rate is instead derived based on how steeply the floor normal
/// projects onto the kart's forward axis, clamped to [0.3f, 0.8f]. If the kart is in a nose dive,
/// the stabilization factor is increased to help the kart re-orient itself faster. If the kart is
/// on a boost ramp, the stabilization rate is instead set to 0.4f. Finally, applies the
/// stabilization factor and updates @enum eStatus to reflect whether the new surface is trickable.
void KartMove::calcTop() {
    constexpr f32 DEFAULT_STABILIZATION_FACTOR = 0.1f;
    constexpr f32 BOOST_RAMP_STABILIZATION_FACTOR = 0.4f;
    constexpr f32 DEFAULT_SMOOTH_INTERP_RATE = 0.8f;

    f32 stabilizationFactor = DEFAULT_STABILIZATION_FACTOR;
    m_hasLandingDir = false;
    EGG::Vector3f collisionTop = state()->up();
    const auto &status = KartObjectProxy::status();

    if (status.onBit(eStatus::GroundStart) && m_nonZipperAirtime >= 3) {
        m_smoothedUp = collisionTop;
        m_up = collisionTop;
        m_landingDir = m_dir.perpInPlane(m_smoothedUp, true);
        m_dirDiff = m_landingDir.proj(m_landingDir);
        m_hasLandingDir = true;
    } else {
        if (status.onBit(eStatus::Hop) && m_hopPosY > 0.0f) {
            stabilizationFactor = m_driftingParams->stabilizationFactor;
        } else if (status.onBit(eStatus::TouchingGround)) {
            if ((m_flags.onBit(eFlags::TrickableSurface) || state()->trickableTimer() > 0) &&
                    collisionTop.dot(m_dir) > 0.0f && m_speed > 50.0f &&
                    collide()->surfaceFlags().onBit(KartCollide::eSurfaceFlags::NotTrickable)) {
                collisionTop = m_up;
            } else {
                m_up = collisionTop;
            }

            f32 smoothInterpRate = DEFAULT_SMOOTH_INTERP_RATE;

            if (status.onBit(eStatus::HalfPipeRamp) ||
                    (status.offBit(eStatus::Boost, eStatus::RampBoost, eStatus::Wheelie,
                             eStatus::OverZipper) &&
                            (status.offBit(eStatus::ZipperBoost) || m_zipperBoostTimer > 15))) {
                f32 topDotZ = DEFAULT_SMOOTH_INTERP_RATE -
                        6.0f * (EGG::Mathf::abs(collisionTop.dot(componentZAxis())));
                smoothInterpRate = std::min(DEFAULT_SMOOTH_INTERP_RATE, std::max(0.3f, topDotZ));
            }

            m_smoothedUp += (collisionTop - m_smoothedUp) * smoothInterpRate;
            m_smoothedUp.normalise();

            f32 bodyDotFront = bodyForward().dot(m_smoothedUp);

            if (bodyDotFront < -0.1f) {
                stabilizationFactor += std::min(0.2f, EGG::Mathf::abs(bodyDotFront) * 0.5f);
            }

            if (collide()->surfaceFlags().onBit(KartCollide::eSurfaceFlags::BoostRamp)) {
                stabilizationFactor = BOOST_RAMP_STABILIZATION_FACTOR;
            }
        } else {
            calcAirtimeTop();
        }
    }

    dynamics()->setStabilizationFactor(stabilizationFactor);

    m_nonZipperAirtime = status.onBit(eStatus::OverZipper) ? 0 : state()->airtime();
    m_flags.changeBit(collide()->surfaceFlags().onBit(KartCollide::eSurfaceFlags::Trickable),
            eFlags::TrickableSurface);
}

/// @addr{0x8057D888}
/// @brief Special handling of the bike's up vector after 20 frames of non-zipper airtime
void KartMove::calcAirtimeTop() {
    const auto &status = KartObjectProxy::status();

    if (status.onBit(eStatus::OverZipper) || status.offBit(eStatus::AirtimeOver20)) {
        return;
    }

    if (m_smoothedUp.y <= 0.99f) {
        m_smoothedUp += (EGG::Vector3f::ey - m_smoothedUp) * 0.03f;
        m_smoothedUp.normalise();
    } else {
        m_smoothedUp = EGG::Vector3f::ey;
    }

    if (m_up.y <= 0.99f) {
        m_up += (EGG::Vector3f::ey - m_up) * 0.03f;
        m_up.normalise();
    } else {
        m_up = EGG::Vector3f::ey;
    }
}

/// @addr{0x80587590}
/// @brief Every frame, calculates any boost resulting from a boost panel, ramp, or jump pad
void KartMove::calcSpecialFloor() {
    const auto *raceMgr = System::RaceManager::Instance();
    if (!raceMgr->isStageReached(System::RaceManager::Stage::Race)) {
        return;
    }

    if (m_padType.onBit(ePadType::BoostPanel)) {
        tryStartBoostPanel();
    }

    if (m_padType.onBit(ePadType::BoostRamp)) {
        tryStartBoostRamp();
    }

    if (m_padType.onBit(ePadType::JumpPad)) {
        tryStartJumpPad();
    }

    m_padType.makeAllZero();
}

/// @addr{0x8057A140}
/// @brief Calculates the kart's directional vectors based on its current orientation and status
/// @details Checks if player steering or drifting should influence the kart's facing direction this
/// frame. If so, chooses an anchor for the kart's facing direction (either based on the kart's
/// orientation or on the hop direction if hopping). Builds a rotation matrix using a combination of
/// drift angles and the landing angle and applies it to the anchored facing direction. Computes how
/// far the kart's current direction is from the target rotation defined by the rotation matrix and
/// accumulates this into @ref m_dirDiff after applying a rotation factor incurred by the colliding
/// KCL type. Applies this computed difference to the kart's facing direction, decaying the
/// direction difference over time. It also computes the direction that internal velocity should be
/// applied towards. Dispatches to @ref KartJump::tryStart() which may initiate a trick if
/// applicable and modify the kart's forward direction.
///
/// If the kart landed on the ground this frame, then it tracks how far the kart's current direction
/// deviates from the snapped direction on landing, so that the kart's direction eases towards the
/// landing direction rather than snapping.
void KartMove::calcDirs() {
    EGG::Vector3f forward = mainRot().rotateVector(EGG::Vector3f::ex).cross(m_smoothedUp);
    forward.normalise();
    m_flags.setBit(eFlags::LaunchBoost);
    auto &status = KartObjectProxy::status();

    if (status.offBit(eStatus::InATrick, eStatus::OverZipper) &&
            (((status.onBit(eStatus::TouchingGround) || status.offBit(eStatus::RampBoost) ||
                      !m_jump->isBoostRampEnabled()) &&
                     status.offBit(eStatus::JumpPad) && state()->airtime() <= 5) ||
                    status.onBit(eStatus::JumpPadMushroom, eStatus::NoSparkInvisibleWall))) {
        EGG::Vector3f local_94 = forward;
        if (status.onBit(eStatus::Hop)) {
            local_94 = m_hopDir;
        }

        EGG::Matrix34f mat;
        mat.setAxisRotation(DEG2RAD * (m_autoDriftAngle + m_outsideDriftAngle + m_landingAngle),
                m_smoothedUp);
        EGG::Vector3f local_b8 = mat.multVector(local_94);
        local_b8 = local_b8.perpInPlane(m_smoothedUp, true);

        EGG::Vector3f dirDiff = local_b8 - m_dir;

        if (dirDiff.squaredLength() <= std::numeric_limits<f32>::epsilon()) {
            m_dir = local_b8;
            m_dirDiff.setZero();
        } else {
            EGG::Vector3f origDirCross = m_dir.cross(local_b8);
            m_dirDiff += m_kclRotFactor * dirDiff;
            m_dir += m_dirDiff;
            m_dir.normalise();
            m_dirDiff *= 0.1f;
            EGG::Vector3f newDirCross = m_dir.cross(local_b8);

            if (origDirCross.dot(newDirCross) < 0.0f) {
                m_dir = local_b8;
                m_dirDiff.setZero();
            }
        }

        m_intVelDir = m_dir.perpInPlane(m_smoothedUp, true);
        m_flags.resetBit(eFlags::LaunchBoost);
    } else {
        m_intVelDir = m_dir;
    }

    if (status.offBit(eStatus::OverZipper)) {
        m_jump->tryStart(m_smoothedUp.cross(m_dir));
    }

    EGG::Vector3f nextDir = m_up.cross(forward);
    m_smoothedForward = nextDir.cross(m_up);
    m_smoothedForward.normalise();

    if (m_hasLandingDir) {
        f32 dot = m_dir.dot(m_landingDir);
        EGG::Vector3f cross = m_dir.cross(m_landingDir);
        f32 crossDot = cross.length();
        f32 angle = EGG::Mathf::atan2(crossDot, dot);
        angle = EGG::Mathf::abs(angle);

        f32 fVar4 = 1.0f;
        if (cross.dot(m_smoothedUp) < 0.0f) {
            fVar4 = -1.0f;
        }

        m_landingAngle += (angle * RAD2DEG) * fVar4;
    }

    if (m_landingAngle <= 0.0f) {
        if (m_landingAngle < 0.0f) {
            m_landingAngle = std::min(0.0f, m_landingAngle + 2.0f);
        }
    } else {
        m_landingAngle = std::max(0.0f, m_landingAngle - 2.0f);
    }
}

/// @addr{0x80583B88}
/// @brief Updates internal velocity direction to reflect sticky road floor normals
/// @details This function is repsonsible for adjusting the kart's driving direction when turning
/// down sticky roads so that it does not fly off in a straight direction. This function bails if we
/// are over a zipper, not on sticky road, or are moving slower than 20 units per frame. Performs
/// three collision check passes, where each iteration steps the kart's position downward and
/// narrows its "nextPos". If a collision occurs, updates @ref m_intVelDir to bend toward the curved
/// surface. Also removes moving object and moving road velocity that would fight against the kart
/// sticking to the curve. If the kart is on @enum eStatus::MovingWaterStickyRoad (Koopa Cape
/// entering the pipe) then the kart's up vector is snapped directly to the floor normal so its
/// rotation is sharply aligned with the curved path. If no collision occurs, resets the @enum
/// eStatus::StickyRoad flag.
void KartMove::calcStickyRoad() {
    constexpr f32 STICKY_RADIUS = 200.0f;
    constexpr Field::KCLTypeMask STICKY_MASK =
            KCL_TYPE_BIT(COL_TYPE_STICKY_ROAD) | KCL_TYPE_BIT(COL_TYPE_MOVING_WATER);

    auto &status = KartObjectProxy::status();

    if (status.onBit(eStatus::OverZipper)) {
        status.resetBit(eStatus::StickyRoad);
        return;
    }

    if ((status.offBit(eStatus::StickyRoad) &&
                collide()->surfaceFlags().offBit(KartCollide::eSurfaceFlags::Trickable)) ||
            EGG::Mathf::abs(m_speed) <= 20.0f) {
        return;
    }

    EGG::Vector3f pos = dynamics()->pos();
    EGG::Vector3f vel = dynamics()->movingObjVel() + m_speed * m_intVelDir;
    Field::CollisionInfo colInfo;
    colInfo.bbox.setZero();
    Field::KCLTypeMask kcl_flags = KCL_NONE;
    bool stickyRoad = false;

    for (size_t i = 0; i < 3; ++i) {
        EGG::Vector3f newPos = pos + vel;
        if (Field::CollisionDirector::Instance()->checkSphereFull(STICKY_RADIUS, newPos,
                    EGG::Vector3f::inf, STICKY_MASK, &colInfo, &kcl_flags, 0)) {
            m_intVelDir = m_intVelDir.perpInPlane(colInfo.floorNrm, true);
            dynamics()->setMovingObjVel(dynamics()->movingObjVel().rej(colInfo.floorNrm));
            dynamics()->setMovingRoadVel(dynamics()->movingRoadVel().rej(colInfo.floorNrm));

            if (status.onBit(eStatus::MovingWaterStickyRoad)) {
                m_up = colInfo.floorNrm;
                m_smoothedUp = colInfo.floorNrm;
            }

            stickyRoad = true;

            break;
        }
        vel *= 0.5f;
        pos += -STICKY_RADIUS * componentYAxis();
    }

    if (!stickyRoad) {
        status.resetBit(eStatus::StickyRoad);
    }
}

/// @addr{0x8057C3D4}
/// @brief Each frame, computes rotation and speed scalars from the colliding floor's KCL
void KartMove::calcOffroad() {
    auto &status = KartObjectProxy::status();

    if (status.onBit(eStatus::BoostOffroadInvincibility)) {
        m_kclSpeedFactor = 1.0f;
        m_kclRotFactor = param()->stats().kclRot[0];
    } else {
        bool anyWheel = status.onBit(eStatus::AnyWheelCollision);
        if (anyWheel) {
            m_kclSpeedFactor = m_kclWheelSpeedFactor;
            m_floorCollisionCount = m_floorCollisionCount != 0 ? m_floorCollisionCount : 1;
            m_kclRotFactor = m_kclWheelRotFactor / static_cast<f32>(m_floorCollisionCount);
        }

        if (status.onBit(eStatus::VehicleBodyFloorCollision)) {
            const CollisionData &colData = collisionData();
            if (anyWheel) {
                if (colData.speedFactor < m_kclWheelSpeedFactor) {
                    m_kclSpeedFactor = colData.speedFactor;
                }
                m_kclRotFactor = (m_kclWheelRotFactor + colData.rotFactor) /
                        static_cast<f32>(m_floorCollisionCount + 1);
            } else {
                m_kclSpeedFactor = colData.speedFactor;
                m_kclRotFactor = colData.rotFactor;
            }
        }
    }

    calcRisingWater();
}

/// @addr{0x8058677C}
/// @brief Computes the speed factor driving in rising water and may trigger out-of-bounds
/// @details If the kart's position falls below the rising water kill plane height, triggers an
/// out-of-bounds and will respawn the kart. Otherwise, the speed factor is calculated by linearly
/// interpolating between 1.0f and the kart's water speed stat, where the interpolation factor is
/// based off of how deep the kart is in the water.
void KartMove::calcRisingWater() {
    auto *objDir = Field::ObjectDirector::Instance();
    auto *psea = objDir->psea();
    if (!psea) {
        return;
    }

    f32 pos = wheelPos(0).y;
    u16 count = tireCount();
    for (u16 wheelIdx = 0; wheelIdx < count; ++wheelIdx) {
        f32 tmp = wheelEdgePos(wheelIdx).y;
        if (wheelIdx == 0 || tmp < pos) {
            pos = tmp;
        }
    }

    if (objDir->risingWaterKillPlaneHeight() > pos) {
        collide()->activateOob(true, nullptr, false, false);
    }

    f32 distBelow = -objDir->distAboveRisingWater(pos);
    if (distBelow > 0.0f && status().offBit(Kart::eStatus::BoostOffroadInvincibility)) {
        f32 speedScale = std::min(1.0f, distBelow / 100.0f);
        m_kclSpeedFactor = 1.0f - (1.0f - param()->stats().kclSpeed[3]) * speedScale;
    }
}

/// @addr{0x805828CC}
/// @brief Calculates standstill mini-turbo charges
/// @details If the kart has been charging the SSMT for 75 frames, sets @enum eFlags::SsmtCharged.
/// Implements a leeway timer that allows the player to resume charging a SSMT even if they
/// accidentally let go for 1 frame. Once the SSMT is charged and the player lets go of either the
/// accelerate or brake button, releases a 30 frame SSMT boost.
void KartMove::calcSsmtCharge() {
    constexpr s16 MAX_SSMT_CHARGE = 75;
    constexpr s16 SSMT_BOOST_FRAMES = 30;
    constexpr s16 LEEWAY_FRAMES = 1;
    constexpr s16 DISABLE_ACCEL_FRAMES = 20;

    calcDisableBackwardsAccel();

    auto &status = KartObjectProxy::status();

    if (status.onBit(eStatus::ChargingSSMT)) {
        if (++m_ssmtCharge > MAX_SSMT_CHARGE) {
            m_ssmtCharge = MAX_SSMT_CHARGE;
            m_flags.setBit(eFlags::SsmtCharged);
            m_ssmtLeewayTimer = 0;
        }

        return;
    }

    m_ssmtCharge = 0;

    if (m_flags.offBit(eFlags::SsmtCharged)) {
        return;
    }

    if (m_flags.onBit(eFlags::SsmtLeeway)) {
        if (--m_ssmtLeewayTimer < 0) {
            m_ssmtLeewayTimer = 0;
            m_flags.resetBit(eFlags::SsmtCharged, eFlags::SsmtLeeway);
            m_ssmtDisableAccelTimer = DISABLE_ACCEL_FRAMES;
            status.setBit(eStatus::DisableBackwardsAccel);
        } else {
            if (status.offBit(eStatus::Accelerate, eStatus::Brake)) {
                activateBoost(KartBoost::Type::MiniTurbo, SSMT_BOOST_FRAMES);
                m_ssmtLeewayTimer = 0;
                m_flags.resetBit(eFlags::SsmtCharged, eFlags::SsmtLeeway);
            }
        }
    } else {
        if (status.onBit(eStatus::Accelerate) && status.offBit(eStatus::Brake)) {
            activateBoost(KartBoost::Type::MiniTurbo, SSMT_BOOST_FRAMES);
            m_ssmtLeewayTimer = 0;
            m_flags.resetBit(eFlags::SsmtCharged, eFlags::SsmtLeeway);
        } else {
            m_ssmtLeewayTimer = LEEWAY_FRAMES;
            m_flags.setBit(eFlags::SsmtLeeway);
            status.setBit(eStatus::DisableBackwardsAccel);
            m_ssmtDisableAccelTimer = LEEWAY_FRAMES;
        }
    }
}

/// @addr{0x8057E804}
/// @brief Each frame, checks for hop or slipdrift. Computes drift direction based on player input.
/// @return Whether or not we are hopping or a slipdrift has been buffered
/// @details Checks which direction the player is inputting for the drift and sets that direction to
/// @ref m_hopStickX.
bool KartMove::calcPreDrift() {
    auto &status = KartObjectProxy::status();

    if (status.offBit(eStatus::TouchingGround, eStatus::Hop, eStatus::DriftManual)) {
        if (status.onBit(eStatus::StickLeft, eStatus::StickRight)) {
            if (status.offBit(eStatus::DriftInput)) {
                status.resetBit(eStatus::SlipdriftBuffered);
            } else if (status.offBit(eStatus::SlipdriftBuffered)) {
                if (m_hopStickX == 0) {
                    if (status.onBit(eStatus::StickRight)) {
                        m_hopStickX = -1;
                    } else if (status.onBit(eStatus::StickLeft)) {
                        m_hopStickX = 1;
                    }
                    status.setBit(eStatus::SlipdriftBuffered);
                    onPreDrift();
                }
            }
        }
    }

    if (status.onBit(eStatus::Hop)) {
        if (m_hopStickX == 0) {
            if (status.onBit(eStatus::StickRight)) {
                m_hopStickX = -1;
            } else if (status.onBit(eStatus::StickLeft)) {
                m_hopStickX = 1;
            }
        }
        if (m_hopFrame < 3) {
            ++m_hopFrame;
        }
    } else if (status.onBit(eStatus::SlipdriftBuffered)) {
        m_hopFrame = 0;
    }

    return status.onBit(eStatus::Hop, eStatus::SlipdriftBuffered);
}

/// @addr{0x8057E348}
/// @brief Clears the current drift state and resets related variables
void KartMove::clearDrift() {
    m_flags.resetBit(eFlags::DriftReset);
    m_outsideDriftAngle = 0.0f;
    m_hopStickX = 0;
    m_hopFrame = 0;
    m_driftState = DriftState::NotDrifting;
    m_smtCharge = 0;
    m_mtCharge = 0;
    m_outsideDriftBonus = 0.0f;
    status().resetBit(eStatus::Hop, eStatus::SlipdriftBuffered, eStatus::DriftManual,
            eStatus::DriftAuto);
    m_autoDriftAngle = 0.0f;
    m_hopStickX = 0;
    m_autoDriftStartFrameCounter = 0;
}

/// @addr{0x8057E0DC}
/// @brief Each frame, handles automatic transmission drifting
/// @details Automatic drifts are activated if a steep enough X stick input is held for 15 frames.
/// If the player's X stick falls below this steep threshold, then the 15 frame timer resets,
/// delaying the next possible drift start. When a drift activates and the kart is touching the
/// ground, the hop direction is set to @ref m_hopStickX and the rotation due to drifting is applied
/// to @ref m_autoDriftAngle. Finally, applies the resulting rotation to the kart's physics.
void KartMove::calcAutoDrift() {
    constexpr s16 AUTO_DRIFT_START_DELAY = 12;

    auto &status = KartObjectProxy::status();

    if (status.offBit(eStatus::AutoDrift)) {
        return;
    }

    if (canStartDrift() &&
            status.offBit(eStatus::OverZipper, eStatus::RejectRoadTrigger, eStatus::Wheelie) &&
            EGG::Mathf::abs(state()->stickX()) > 0.85f) {
        m_autoDriftStartFrameCounter =
                std::min<s16>(AUTO_DRIFT_START_DELAY, m_autoDriftStartFrameCounter + 1);
    } else {
        m_autoDriftStartFrameCounter = 0;
    }

    if (m_autoDriftStartFrameCounter >= AUTO_DRIFT_START_DELAY) {
        status.setBit(eStatus::DriftAuto);

        if (status.onBit(eStatus::TouchingGround)) {
            if (state()->stickX() < 0.0f) {
                m_hopStickX = 1;
                m_autoDriftAngle -= 30.0f * param()->stats().driftAutomaticTightness;

            } else {
                m_hopStickX = -1;
                m_autoDriftAngle += 30.0f * param()->stats().driftAutomaticTightness;
            }
        }

        f32 halfTarget = 0.5f * param()->stats().driftOutsideTargetAngle;
        m_autoDriftAngle = std::min(halfTarget, std::max(-halfTarget, m_autoDriftAngle));
    } else {
        status.resetBit(eStatus::DriftAuto);
        m_hopStickX = 0;

        if (m_autoDriftAngle > 0.0f) {
            m_autoDriftAngle =
                    std::max(0.0f, m_autoDriftAngle - param()->stats().driftOutsideDecrement);
        } else {
            m_autoDriftAngle =
                    std::min(0.0f, m_autoDriftAngle + param()->stats().driftOutsideDecrement);
        }
    }

    EGG::Quatf angleAxis;
    angleAxis.setAxisRotation(-m_autoDriftAngle * DEG2RAD, m_up);
    physics()->composeExtraRot(angleAxis);
}

/// @addr{0x8057DC44}
/// @brief Each frame, handles manual transmission hopping, drifting, and mini-turbos
/// @details Computes the drift rotation angle for outward drifting vehicles. Checks if the kart is
/// starting a drift hop and and cancels wheelies if so. If the kart is not drifting but is also
/// touching the ground, then resets all manual drift-related settings. If the kart is drifting,
/// checks various conditions to see if the drift should end. This occurs either when the the player
/// stops accelerating or drifting, the kart enters an action, reject road applies a rejection on
/// the kart, a wall collision occurs, or the kart falls below the required speed threshold for
/// drifting. If a mini-turbo was charged by the time the drift ends, it will be activated.
void KartMove::calcManualDrift() {
    bool isHopping = calcPreDrift();
    auto &status = KartObjectProxy::status();

    if (status.offBit(eStatus::OverZipper)) {
        const EGG::Vector3f rotZ = mainRot().rotateVector(EGG::Vector3f::ez);

        if (status.offBit(eStatus::TouchingGround) &&
                vehicleType() != KartParam::Stats::DriftType::Inside_Drift_Bike &&
                status.offBit(eStatus::JumpPadMushroom) &&
                status.onBit(eStatus::DriftManual, eStatus::SlipdriftBuffered) &&
                m_flags.onBit(eFlags::LaunchBoost)) {
            const EGG::Vector3f up = mainRot().rotateVector(EGG::Vector3f::ey);
            EGG::Vector3f driftRej = m_outsideDriftLastDir.rej(up);

            if (driftRej.normalise() != 0.0f) {
                f32 rejCrossDirMag = driftRej.cross(rotZ).length();
                f32 angle = EGG::Mathf::atan2(rejCrossDirMag, driftRej.dot(rotZ));
                f32 sign = 1.0f;
                if ((rotZ.z * (rotZ.x - driftRej.x)) - (rotZ.x * (rotZ.z - driftRej.z)) > 0.0f) {
                    sign = -1.0f;
                }

                m_outsideDriftAngle += angle * RAD2DEG * sign;
            }
        }

        m_outsideDriftLastDir = rotZ;
    }

    if (((status.offBit(eStatus::Hop) || m_hopFrame < 3) &&
                status.offBit(eStatus::SlipdriftBuffered)) ||
            (status.onBit(eStatus::InAction) || status.offBit(eStatus::TouchingGround))) {
        if (canHop()) {
            startHop();
            isHopping = true;
        }
    } else {
        startManualDrift();
        isHopping = false;
    }

    m_flags.resetBit(eFlags::DriftReset);

    if (status.offBit(eStatus::DriftManual)) {
        if (!isHopping && status.onBit(eStatus::TouchingGround)) {
            resetDriftManual();

            if (action()->flags().offBit(KartAction::eFlags::Rotating) || m_speed <= 20.0f) {
                f32 driftAngleDecr = param()->stats().driftOutsideDecrement;
                if (m_outsideDriftAngle > 0.0f) {
                    m_outsideDriftAngle = std::max(0.0f, m_outsideDriftAngle - driftAngleDecr);
                } else if (m_outsideDriftAngle < 0.0f) {
                    m_outsideDriftAngle = std::min(0.0f, m_outsideDriftAngle + driftAngleDecr);
                }
            }
        }
    } else {
        // This is a different comparison than @ref KartMove::canStartDrift().
        bool canStartDrift = m_speed > MINIMUM_DRIFT_THRESOLD * m_baseSpeed;

        if (status.offBit(eStatus::OverZipper) &&
                (status.offAnyBit(eStatus::DriftInput, eStatus::Accelerate) ||
                        status.onBit(eStatus::InAction, eStatus::RejectRoadTrigger,
                                eStatus::Wall3Collision, eStatus::WallCollision) ||
                        !canStartDrift)) {
            if (canStartDrift) {
                releaseMt();
            }

            resetDriftManual();
            m_flags.setBit(eFlags::DriftReset);
        } else {
            calcDrift();
        }
    }
}

/// @addr{0x8057E3F4}
/// @brief Called when the player lands from a drift hop or a buffered slipdrift
/// @details For outward drifting vehicles, computes the drift angle based on the drift direction
/// (and the hop direction, if the drift started via a hop). Clears @enum eStatus::Hop and @enum
/// eStatus::SlipdriftBuffered. If the drift input is not active or the hop stick X is zero, the
/// function returns early without starting a manual drift. Otherwise, sets @enum
/// eStatus::DriftManual and changes the drift state to @enum DriftState::ChargingMt.
void KartMove::startManualDrift() {
    constexpr f32 OUTSIDE_DRIFT_BONUS = 0.5f;

    const auto &stats = param()->stats();
    auto &status = KartObjectProxy::status();

    if (stats.driftType != KartParam::Stats::DriftType::Inside_Drift_Bike) {
        f32 driftAngle = 0.0f;

        if (status.onBit(eStatus::Hop)) {
            const EGG::Vector3f rotZ = mainRot().rotateVector(EGG::Vector3f::ez);
            EGG::Vector3f rotRej = rotZ.rej(m_hopUp);

            if (rotRej.normalise() != 0.0f) {
                const EGG::Vector3f hopCrossRot = m_hopDir.cross(rotRej);
                driftAngle =
                        EGG::Mathf::atan2(hopCrossRot.length(), m_hopDir.dot(rotRej)) * RAD2DEG;
            }
        }

        m_outsideDriftAngle += driftAngle * static_cast<f32>(-m_hopStickX);
        m_outsideDriftAngle = std::max(-60.0f, std::min(60.0f, m_outsideDriftAngle));
    }

    status.resetBit(eStatus::Hop, eStatus::SlipdriftBuffered);

    if (status.offBit(eStatus::DriftInput)) {
        return;
    }

    if (getAppliedHopStickX() == 0) {
        return;
    }

    status.setBit(eStatus::DriftManual).resetBit(eStatus::Hop);
    m_driftState = DriftState::ChargingMt;
    m_outsideDriftBonus = OUTSIDE_DRIFT_BONUS * (m_speedRatioCapped * stats.driftManualTightness);
}

/// @addr{0x80582F9C}
/// @brief Stops charging a mini-turbo, and applies boost if charged
/// @details If a MT was not charged or the player is braking, resets the drift state to @enum
/// DriftState::NotDrifting and bails out. If a super mini-turbo was charged, extends the mini-turbo
/// duration by 3x.
void KartMove::releaseMt() {
    constexpr f32 SMT_LENGTH_FACTOR = 3.0f;

    auto &status = KartObjectProxy::status();

    if (m_driftState < DriftState::ChargedMt || status.onBit(eStatus::Brake)) {
        m_driftState = DriftState::NotDrifting;
        return;
    }

    u16 mtLength = param()->stats().miniTurboDuration;

    if (m_driftState == DriftState::ChargedSmt) {
        mtLength *= SMT_LENGTH_FACTOR;
    }

    if (status.offBit(eStatus::BeforeRespawn, eStatus::InAction)) {
        activateBoost(KartBoost::Type::MiniTurbo, mtLength);
    }

    m_driftState = DriftState::NotDrifting;
}

/// @addr{0x8057EAB8}
/// @brief Every frame, handles mini-turbo charging and outside drifting bike rotation
/// @details If the kart has more than 5 frames of airtime, skips updating the drift angle and MT
/// charging.
void KartMove::calcDrift() {
    if (state()->airtime() > 5) {
        return;
    }

    if (vehicleType() != KartParam::Stats::DriftType::Inside_Drift_Bike) {
        if (m_hopStickX == -1) {
            f32 angle = m_outsideDriftAngle;
            f32 targetAngle = param()->stats().driftOutsideTargetAngle;
            if (angle > targetAngle) {
                m_outsideDriftAngle = std::max(m_outsideDriftAngle - 2.0f, targetAngle);
            } else if (angle < targetAngle) {
                m_outsideDriftAngle += 150.0f * param()->stats().driftManualTightness;
                m_outsideDriftAngle = std::min(m_outsideDriftAngle, targetAngle);
            }
        } else if (m_hopStickX == 1) {
            f32 angle = m_outsideDriftAngle;
            f32 targetAngle = -param()->stats().driftOutsideTargetAngle;
            if (targetAngle > angle) {
                m_outsideDriftAngle = std::min(m_outsideDriftAngle + 2.0f, targetAngle);
            } else if (targetAngle < angle) {
                m_outsideDriftAngle -= 150.0f * param()->stats().driftManualTightness;
                m_outsideDriftAngle = std::max(m_outsideDriftAngle, targetAngle);
            }
        }
    }

    calcMtCharge();
}

/// @addr{0x8057C69C}
/// @brief Every frame, calculates kart rotation based on player input
/// @details The turn magnitude is pulled from the kart's stats depending on whether or not the kart
/// is drifting and which transmission is being used. If using an outward drifting vehicle, an
/// additional rotation bonus is decayed and then applied to the turn magnitude.
///
/// If charging a stand-still mini-turbo, the turn magnitude is set to 4% of the raw turn value.
/// Otherwise, the turn magnitude is scaled based on whether the kart is hopping or driving slowly.
/// The turn magnitude is negated if the vehicle is braking or driving backwards. It's doubled if
/// the kart is not drifting during a landing zipper boost. The turn magnitude increases if we are
/// in an automatic transmission drift and the stick is held at a large enough value. If the kart is
/// in a ramp boost, the turn magnitude is reset to zero. If the kart is midair from something other
/// than a ramp boost and a mushroom bounce pad, the turn magnitude will decrease after 30 frames of
/// airtime, such that it interpolates from its initial value (up to 30 frames of airtime) to zero
/// (after 70 frames of airtime). Finally, applies this turn magnitude to the kart's rotation.
void KartMove::calcRotation() {
    constexpr u32 AIRTIME_DECAY_START = 30;
    constexpr u32 AIRTIME_DECAY_END = 70;
    constexpr f32 AIRTIME_DECAY_STEP = 1.0f / (AIRTIME_DECAY_END - AIRTIME_DECAY_START);

    f32 turn;
    auto &status = KartObjectProxy::status();
    bool drifting = state()->isDrifting() && status.offBit(eStatus::JumpPadMushroom);
    bool autoDrift = status.onBit(eStatus::AutoDrift);
    const auto &stats = param()->stats();

    if (drifting) {
        turn = autoDrift ? stats.driftAutomaticTightness : stats.driftManualTightness;
    } else {
        turn = autoDrift ? stats.handlingAutomaticTightness : stats.handlingManualTightness;
    }

    if (drifting && stats.driftType != KartParam::Stats::DriftType::Inside_Drift_Bike) {
        m_outsideDriftBonus *= 0.99f;
        turn += m_outsideDriftBonus;
    }

    bool forwards = status.offBit(eStatus::Brake) || m_speed > 0.0f;
    turn *= m_realTurn;
    if (status.onBit(eStatus::ChargingSSMT)) {
        turn = m_realTurn * 0.04f;
    } else {
        if (status.onBit(eStatus::Hop) && m_hopPosY > 0.0f) {
            turn *= 1.4f;
        }

        if (!drifting) {
            bool noTurn = false;
            if (status.offBit(eStatus::WallCollision, eStatus::Wall3Collision) &&
                    EGG::Mathf::abs(m_speed) < 1.0f) {
                if (!(status.onBit(eStatus::Hop) && m_hopPosY > 0.0f)) {
                    turn = 0.0f;
                    noTurn = true;
                }
            }
            if (forwards && !noTurn) {
                if (m_speed >= 20.0f) {
                    turn *= 0.5f;
                    if (m_speed < 70.0f) {
                        turn += (1.0f - (m_speed - 20.0f) / 50.0f) * turn;
                    }
                } else {
                    turn = (turn * 0.4f) + (m_speed / 20.0f) * (turn * 0.6f);
                }
            }
        }

        if (!forwards) {
            turn = -turn;
        }

        if (status.onBit(eStatus::ZipperBoost) && status.offBit(eStatus::DriftManual)) {
            turn *= 2.0f;
        }

        f32 stickX = EGG::Mathf::abs(state()->stickX());
        if (autoDrift && stickX > 0.3f) {
            f32 stickScalar = (stickX - 0.3f) / 0.7f;
            stickX = drifting ? 0.2f : 0.5f;
            turn += stickScalar * (turn * stickX * m_speedRatioCapped);
        }
    }

    if (status.offBit(eStatus::InAction, eStatus::ZipperTrick)) {
        if (status.offBit(eStatus::TouchingGround)) {
            if (status.onBit(eStatus::RampBoost) && m_jump->isBoostRampEnabled()) {
                turn = 0.0f;
            } else if (status.offBit(eStatus::JumpPadMushroom)) {
                u32 airtime = state()->airtime();
                if (airtime >= AIRTIME_DECAY_END) {
                    turn = 0.0f;
                } else if (airtime >= AIRTIME_DECAY_START) {
                    turn = std::max(0.0f,
                            turn * (1.0f - (airtime - AIRTIME_DECAY_START) * AIRTIME_DECAY_STEP));
                }
            }
        }

        const EGG::Vector3f forward = mainRot().rotateVector(EGG::Vector3f::ez);
        f32 angle = EGG::Mathf::atan2(forward.cross(m_dir).length(), forward.dot(m_dir));
        angle = EGG::Mathf::abs(angle) * RAD2DEG;

        if (angle > 60.0f) {
            turn *= std::max(0.0f, 1.0f - (angle - 60.0f) / 40.0f);
        }
    }

    calcVehicleRotation(turn);
}

/// @addr{0x8057AB68}
/// @brief Every frame, computes speed based on acceleration and any active boosts
/// @details Applies the forward component of external velocity to the kart's speed. If the kart's
/// speed is below -20.0f, it increases by 0.5f every frame. If the kart is in moving water that
/// decays speed, the kart's speed is scaled by @ref KartPullPath::m_roadSpeedDecay. If the kart is
/// in an @enum Action, then defers the rest of the vehicle's speed calculation to @ref
/// KartAction::calcVehicleSpeed(). If in a ramp boost with fewer than 4 frames of airtime, sets
/// acceleration to 7. Otherwise, computes @ref m_speedDragMultiplier depending on whether the kart
/// is on a jump pad without accelerating, over a zipper, or has more than 5 frames of airtime.
/// Applies this multiplier to the kart's speed. Otherwise, if the kart is on the ground,
/// acceleration is set based on a couple different flags.
///
/// If the kart is in a boost, acceleration is set to the boost acceleration, unless colliding with
/// moving water, in which case it defers to @ref calcVehicleAcceleration. If the kart is in a ramp
/// boost or jump pad, acceleration is set to 7, unless colliding with moving water, in which case
/// it defers to @ref calcVehicleAcceleration as well. Otherwise, if the player is pressing the
/// accel button, then acceleration is based on @ref calcVehicleAcceleration, unless on a zipper, in
/// which case it is set to 5. If the player is not accelerating, then acceleration is computed
/// based on whether the kart is braking or reversing. Additionally, the kart's speed is scaled down
/// based on how much the kart is turning while not in a boost or drift.
void KartMove::calcVehicleSpeed() {
    const auto *raceMgr = System::RaceManager::Instance();
    auto &status = KartObjectProxy::status();

    if (raceMgr->isStageReached(System::RaceManager::Stage::Race)) {
        f32 forwardExtVel = dynamics()->headingExtVel();
        if (status.onBit(eStatus::InAction) ||
                ((status.onBit(eStatus::WallCollisionStart) || state()->wallBonkTimer() == 0 ||
                         EGG::Mathf::abs(forwardExtVel) >= 3.0f) &&
                        status.offBit(eStatus::DriftManual))) {
            m_speed += forwardExtVel;
        }
    }

    if (m_speed < -20.0f) {
        m_speed += 0.5f;
    }

    bool water = false;

    if (status.onBit(eStatus::MovingWaterDecaySpeed) && status.offBit(eStatus::MushroomBoost) &&
            EGG::Mathf::abs(m_speed) > 5.0f) {
        water = true;
        m_speed *= collide()->pullPath().roadSpeedDecay();
    }

    m_acceleration = 0.0f;
    m_speedDragMultiplier = 1.0f;

    if (status.onBit(eStatus::InAction)) {
        action()->calcVehicleSpeed();
        return;
    }

    if ((status.onAllBit(eStatus::SoftWallPush, eStatus::TouchingGround) &&
                status.offBit(eStatus::AnyWheelCollision)) ||
            status.offBit(eStatus::TouchingGround) ||
            status.onBit(eStatus::DisableAcceleration, eStatus::ChargingSSMT)) {
        if (status.onBit(eStatus::RampBoost) && state()->airtime() < 4) {
            m_acceleration = 7.0f;
        } else {
            if (status.onBit(eStatus::JumpPad) && status.offBit(eStatus::Accelerate)) {
                m_speedDragMultiplier = 0.99f;
            } else {
                if (status.onBit(eStatus::OverZipper)) {
                    m_speedDragMultiplier = 0.999f;
                } else {
                    if (state()->airtime() > 5) {
                        m_speedDragMultiplier = 0.999f;
                    }
                }
            }
            m_speed *= m_speedDragMultiplier;
        }
    } else if (status.offBit(eStatus::Boost)) {
        if (status.offBit(eStatus::JumpPad, eStatus::RampBoost)) {
            if (status.onBit(eStatus::Accelerate)) {
                m_acceleration =
                        status.onBit(eStatus::HalfPipeRamp) ? 5.0f : calcVehicleAcceleration();
            } else {
                if (status.offBit(eStatus::Brake) ||
                        status.onBit(eStatus::DisableBackwardsAccel, eStatus::SoftWallPush)) {
                    m_speed *= m_speed > 0.0f ? 0.98f : 0.95f;
                } else if (m_drivingDirection == DrivingDirection::Braking) {
                    m_acceleration = -1.5f;
                } else if (m_drivingDirection == DrivingDirection::WaitingForBackwards) {
                    if (++m_backwardsAllowCounter > 15) {
                        m_drivingDirection = DrivingDirection::Backwards;
                    }
                } else if (m_drivingDirection == DrivingDirection::Backwards) {
                    m_acceleration = -2.0f;
                }
            }

            if (status.offBit(eStatus::Boost, eStatus::DriftManual, eStatus::AutoDrift)) {
                const auto &stats = param()->stats();

                f32 x = 1.0f - EGG::Mathf::abs(m_weightedTurn) * m_speedRatioCapped;
                m_speed *= stats.turningSpeed + (1.0f - stats.turningSpeed) * x;
            }
        } else {
            m_acceleration = water ? calcVehicleAcceleration() : 7.0f;
        }
    } else {
        m_acceleration = water ? calcVehicleAcceleration() : m_boost.acceleration();
    }
}

/// @addr{0x8057B868}
/// @brief Every frame, computes acceleration based off the character/vehicle stats
/// @returns The calculated acceleration value based on the current speed ratio and vehicle stats
/// @details This function leverages an array of speed thresholds and associated acceleration values
/// for each threshold. The array varies depending on whether the vehicle is drifting or not. If the
/// vehicle is traveling backwards, the kart moves with a constant acceleration of 1. Performs a
/// lookup in the array of speed thresholds to find which acceleration should correspond with the
/// current speed ratio.
f32 KartMove::calcVehicleAcceleration() const {
    f32 ratio = m_speed / m_softSpeedLimit;
    if (ratio < 0.0f) {
        return 1.0f;
    }

    std::span<const f32> as;
    std::span<const f32> ts;
    if (state()->isDrifting()) {
        as = param()->stats().accelerationDriftA;
        ts = param()->stats().accelerationDriftT;
    } else {
        as = param()->stats().accelerationStandardA;
        ts = param()->stats().accelerationStandardT;
    }

    size_t i = 0;
    f32 acceleration = 0.0f;
    f32 t_curr = 0.0f;
    for (; i < ts.size(); ++i) {
        if (ratio < ts[i]) {
            acceleration = as[i] + ((as[i + 1] - as[i]) / (ts[i] - t_curr)) * (ratio - t_curr);
            break;
        }

        t_curr = ts[i];
    }

    return i < ts.size() ? acceleration : as.back();
}

/// @addr{0x8057B9BC}
/// @brief Every frame, calculates the kart's speed and applies it to the internal velocity
/// @details If the kart is in a burnout, its speed is set to 0. Otherwise, acceleration is applied
/// to the kart's speed. If the acceleration was negative, it's first clamped to prevent the speed
/// from going below -20. If the kart fell out-of-bounds, its speed decays by 5% each frame. If the
/// kart is charging a stand-still mini-turbo, its speed decayse by 20% to quickly bring the kart to
/// a stop. Otherwise, if the kart is braking, the kart's speed is clamped to 0 to induce a delay
/// before reversing.
///
/// Computes a speed limit based on the current status of the kart. It's initialized to the kart's
/// base speed or the jump pad's max speed if there is an active jump pad. Next, if the jump pad has
/// a fixed speed, then the speed limit is unaffected; otherwise, it is scaled down if the kart is
/// shocked or crushed, scaled up if the bike is currently wheeling, and is scaled down based on the
/// speed factor of the colliding floor KCL.
///
/// Constructs a "scale multiplier" that is either initialized to 1 or is set to the shock speed
/// multiplier if the kart has a ramp boost or is on a zipper. The "boost speed limit" is computed
/// by applying the scale multiplier to any active boost speed limit and the colliding floor's KCL
/// speed factor. If the kart is not on a jump pad and this boost speed limit is greater than the
/// previous computed speed limit, then the speed limit is set to this boost speed limit. If the
/// kart is in a ramp boost, the speed limit is clamped to 100.0f. Finally, the speed limit is
/// further scaled based off the return of @ef calcWallCollisionSpeedFactor.
///
/// Next, this function computes the "soft" speed limit. If it's less than the speed limit computed
/// above or a wall collision occured, it's set to the speed limit; otherwise, it decays by 3 units
/// per frame. Next, the soft speed limit is clamped to the "hard" speed limit. Finally, the kart's
/// speed is clamped to the soft speed limit. If the kart is in a jump pad and the kart's speed is
/// lower than @ref m_jumpPadMinSpeed, it is set to @ref m_jumpPadMinSpeed. Next, it dispatches to
/// @ref calcWallCollisionStart() to handle new wall collisions.
///
/// Computes the kart's speed ratio and its capped speed ratio (capped at 1.0f). Applies a rotation
/// scalar to the kart's internal velocity direction, which varies based on whether the kart is in a
/// ramp boost or midair. Also calls @ref calcDeceleration() to apply gravity-induced rolling on
/// slopes. Based off the kart's internal velocity direction and speed, applies the resulting
/// velocity to the kart's internal velocity. The velocity added is clamped to terminal velocity,
/// which is normally 90 unless on a zipper (65). Lastly, updates the kart's @ref m_drivingDirection
/// based on the current speed.
void KartMove::calcSpeed() {
    constexpr f32 ROTATION_SCALAR_NORMAL = 0.5f;
    constexpr f32 ROTATION_SCALAR_MIDAIR = 0.2f;
    constexpr f32 ROTATION_SCALAR_BOOST_RAMP = 4.0f;
    constexpr f32 OOB_SLOWDOWN_RATE = 0.95f;
    constexpr f32 TERMINAL_VELOCITY = 90.0f;
    constexpr f32 MAX_REVERSE_SPEED = -20.0f;
    constexpr f32 CRUSH_SLOWDOWN_RATE = 0.7f;

    m_lastSpeed = m_speed;
    const auto &status = KartObjectProxy::status();

    if (status.offBit(eStatus::InAction)) {
        dynamics()->setKillExtVelY(status.onBit(eStatus::RespawnKillY));
    }

    if (status.onBit(eStatus::Burnout)) {
        m_speed = 0.0f;
    } else {
        if (m_acceleration < 0.0f) {
            if (m_speed < MAX_REVERSE_SPEED) {
                m_acceleration = 0.0f;
            } else {
                if (m_speed + m_acceleration <= MAX_REVERSE_SPEED) {
                    m_acceleration = MAX_REVERSE_SPEED - m_speed;
                }
            }
        }

        m_speed += m_acceleration;
    }

    if (status.onBit(eStatus::BeforeRespawn)) {
        m_speed *= OOB_SLOWDOWN_RATE;
    } else {
        if (status.onBit(eStatus::ChargingSSMT)) {
            m_speed *= 0.8f;
        } else {
            if (m_drivingDirection == DrivingDirection::Braking && m_speed < 0.0f) {
                m_speed = 0.0f;
                m_drivingDirection = DrivingDirection::WaitingForBackwards;
                m_backwardsAllowCounter = 0;
            }
        }
    }

    f32 speedLimit = status.onBit(eStatus::JumpPad) ? m_jumpPadMaxSpeed : m_baseSpeed;
    const f32 boostMultiplier = m_boost.multiplier();
    const f32 boostSpdLimit = m_boost.speedLimit();
    m_jumpPadBoostMultiplier = boostMultiplier;

    f32 scaleMultiplier = m_shockSpeedMultiplier;
    if (status.onBit(eStatus::Crushed)) {
        scaleMultiplier *= CRUSH_SLOWDOWN_RATE;
    }

    f32 wheelieBonus = boostMultiplier + getWheelieSoftSpeedLimitBonus();
    speedLimit *= status.onBit(eStatus::JumpPadFixedSpeed) ?
            1.0f :
            scaleMultiplier * (wheelieBonus * m_kclSpeedFactor);

    bool ignoreScale = status.onBit(eStatus::RampBoost, eStatus::ZipperInvisibleWall,
            eStatus::OverZipper, eStatus::HalfPipeRamp);
    scaleMultiplier = ignoreScale ? 1.0f : scaleMultiplier;
    f32 boostSpeedLimit = scaleMultiplier * (boostSpdLimit * m_kclSpeedFactor);

    if (status.offBit(eStatus::JumpPad) && boostSpeedLimit > 0.0f && boostSpeedLimit > speedLimit) {
        speedLimit = boostSpeedLimit;
    }

    m_jumpPadSoftSpeedLimit = boostSpdLimit * m_kclSpeedFactor;

    if (status.onBit(eStatus::RampBoost)) {
        speedLimit = std::max(speedLimit, 100.0f);
    }

    m_lastDir = (m_speed > 0.0f) ? 1.0f * m_dir : -1.0f * m_dir;

    f32 wallColSeverity = 1.0f;
    speedLimit *= calcWallCollisionSpeedFactor(wallColSeverity);

    if (m_softSpeedLimit <= speedLimit) {
        m_softSpeedLimit = speedLimit;
    } else if (status.offBit(eStatus::WallCollision, eStatus::Wall3Collision)) {
        m_softSpeedLimit = std::max(m_softSpeedLimit - 3.0f, speedLimit);
    } else {
        m_softSpeedLimit = speedLimit;
    }

    m_softSpeedLimit = std::min(m_hardSpeedLimit, m_softSpeedLimit);
    m_speed = std::clamp(m_speed, -m_softSpeedLimit, m_softSpeedLimit);

    if (status.onBit(eStatus::JumpPad)) {
        m_speed = std::max(m_speed, m_jumpPadMinSpeed);
    }

    calcWallCollisionStart(wallColSeverity);

    m_speedRatio = EGG::Mathf::abs(m_speed / m_baseSpeed);
    m_speedRatioCapped = std::min(1.0f, m_speedRatio);

    EGG::Vector3f crossVec = m_smoothedUp.cross(m_dir);
    if (m_speed < 0.0f) {
        crossVec = -crossVec;
    }

    f32 rotationScalar = ROTATION_SCALAR_NORMAL;
    if (collide()->surfaceFlags().onBit(KartCollide::eSurfaceFlags::BoostRamp)) {
        rotationScalar = ROTATION_SCALAR_BOOST_RAMP;
    } else if (status.offBit(eStatus::TouchingGround)) {
        rotationScalar = ROTATION_SCALAR_MIDAIR;
    }

    EGG::Matrix34f local_90;
    local_90.setAxisRotation(DEG2RAD * rotationScalar, crossVec);
    m_intVelDir = local_90.multVector33(m_intVelDir);

    const auto *raceMgr = System::RaceManager::Instance();
    if (status.offBit(eStatus::InAction, eStatus::DisableBackwardsAccel, eStatus::Accelerate) &&
            status.onBit(eStatus::TouchingGround) &&
            raceMgr->isStageReached(System::RaceManager::Stage::Race)) {
        calcDeceleration();
    }

    EGG::Vector3f vel = m_speed * m_intVelDir;

    f32 maxSpeedY = status.onBit(eStatus::OverZipper) ? KartHalfPipe::TerminalVelocity() :
                                                        TERMINAL_VELOCITY;
    vel.y = std::min(vel.y, maxSpeedY);

    dynamics()->setIntVel(intVel() + vel);

    if (status.onBit(eStatus::TouchingGround) &&
            status.offBit(eStatus::DriftManual, eStatus::Hop)) {
        if (status.onBit(eStatus::Brake)) {
            if (m_drivingDirection == DrivingDirection::Forwards) {
                m_drivingDirection =
                        m_speed > 5.0f ? DrivingDirection::Braking : DrivingDirection::Backwards;
            }
        } else {
            if (m_speed >= 0.0f) {
                m_drivingDirection = DrivingDirection::Forwards;
            }
        }
    } else {
        m_drivingDirection = DrivingDirection::Forwards;
    }
}

/// @addr{0x8057B108}
/// @brief Every frame, computes a speed scalar if we are colliding with a wall
/// @param walColSeverity Out param, represents how head-on the wall hit is [0.0f, 1.0f]
/// @return The speed factor to apply due to the wall collision [0.0f, 1.0f]
/// @details Bails out early if we are not colliding with a wall. Dispatches to @ref
/// onWallCollision() so that wheelies are cancelled on wall collisions. If the kart is over a
/// zipper, bails out early. Computes the dot product between the kart's last direction and the wall
/// normal to determine how head-on the collision is. If the dot product is non-negative, then the
/// kart isn't driving into the wall, so returns 1.0f. Otherwise, the speed factory is computed
/// based on the dot product multiplied by a scalar that depends on the type of the wall the kart
/// collided with.
f32 KartMove::calcWallCollisionSpeedFactor(f32 &walColSeverity) {
    constexpr f32 WALL_COLLISION_SPEED_FACTOR = 0.4f;
    constexpr f32 WALL3_COLLISION_SPEED_FACTOR = 0.7f;

    auto &status = KartObjectProxy::status();

    if (status.offBit(eStatus::WallCollision, eStatus::Wall3Collision)) {
        return 1.0f;
    }

    onWallCollision();

    if (status.onBit(eStatus::ZipperInvisibleWall, eStatus::OverZipper)) {
        return 1.0f;
    }

    EGG::Vector3f wallNrm = collisionData().wallNrm;
    if (wallNrm.y > 0.0f) {
        wallNrm.y = 0.0f;
        wallNrm.normalise();
    }

    f32 dot = m_lastDir.dot(wallNrm);

    if (dot < 0.0f) {
        walColSeverity = std::max(0.0f, dot + 1.0f);
        f32 wallSpeedFactor = status.onBit(eStatus::WallCollision) ? WALL_COLLISION_SPEED_FACTOR :
                                                                     WALL3_COLLISION_SPEED_FACTOR;
        return std::min(1.0f, walColSeverity * wallSpeedFactor);
    }

    return 1.0f;
}

/// @addr{0x8057B2A0}
/// @brief If we started to collide with a wall this frame, applies a rotation and force on the kart
/// @param wallColSeverity How head-on the wall collision hit was [0.0f, 1.0f]
/// @details Bails out early if a wall collision has not started on this frame. Clears @ref
/// m_outsideDriftAngle. If the kart is not in an action, it resets the forward and landing
/// directions to match the forward direction of the kart's body. If the kart is over a zipper or
/// the wall collision severity is very high (>= 0.9f), bails out.
///
/// Computes the speed difference between this frame and the last frame. If the speed difference is
/// > 30.0f, a wall bounce occurs. First @enum eFlags::WallBounce is set. Computes the vertical
/// center point of the vehicle to avoid excessive torque from wall collisions that occur far
/// above/below the vehicle's center. Scales the wall normal based off of the speed difference
/// (clamped to a max of 60.0f) and splits it into a projection and rejection relative to the
/// vehicle's internal velocity direction. Scales those components so that the projection is reduced
/// by 70% and the rejection is reduced by 10%. If the kart is in a boost, the projection and
/// rejection are cleared to the zero vector. If the kart is facing away from the wall, the
/// projection is zeroed. Then, the rejection is reduced a further 10%. Lastly, sums the projection
/// and rejection and applies the wrench to the kart, scaled by @ref
/// KartParam::Stats::bumpDeviationLevel.
void KartMove::calcWallCollisionStart(f32 wallColSeverity) {
    m_flags.resetBit(eFlags::WallBounce);

    const auto &status = KartObjectProxy::status();

    if (status.offBit(eStatus::WallCollisionStart)) {
        return;
    }

    m_outsideDriftAngle = 0.0f;
    if (status.offBit(eStatus::InAction)) {
        m_dir = bodyForward();
        m_intVelDir = m_dir;
        m_landingDir = m_dir;
        m_smoothedForward = m_dir;
    }

    if (status.offBit(eStatus::ZipperInvisibleWall, eStatus::OverZipper) &&
            wallColSeverity < 0.9f) {
        f32 speedDiff = m_lastSpeed - m_speed;
        const CollisionData &colData = collisionData();

        if (speedDiff > 30.0f) {
            m_flags.setBit(eFlags::WallBounce);
            EGG::Vector3f newPos = colData.relPos + pos();
            f32 dot = -bodyUp().dot(colData.relPos) * 0.5f;
            EGG::Vector3f scaledUp = dot * bodyUp();
            newPos -= scaledUp;

            speedDiff = std::min(60.0f, speedDiff);
            EGG::Vector3f scaledWallNrm = speedDiff * colData.wallNrm;

            auto [proj, rej] = scaledWallNrm.projAndRej(m_intVelDir);
            proj *= 0.3f;
            rej *= 0.9f;

            if (status.onBit(eStatus::Boost)) {
                proj = EGG::Vector3f::zero;
                rej = EGG::Vector3f::zero;
            }

            if (bodyForward().dot(colData.wallNrm) > 0.0f) {
                proj = EGG::Vector3f::zero;
            }
            rej *= 0.9f;

            EGG::Vector3f projRejSum = proj + rej;
            f32 bumpDeviation = 0.0f;
            if (m_flags.offBit(eFlags::DriftReset) && status.onBit(eStatus::TouchingGround)) {
                bumpDeviation = param()->stats().bumpDeviationLevel;
            }

            dynamics()->applyWrenchScaled(newPos, projRejSum, bumpDeviation);
        } else if (wallKclType() == COL_TYPE_SPECIAL_WALL && wallKclVariant() == 2) {
            dynamics()->addForce(colData.wallNrm * 15.0f);
            collide()->applyWeakFloorMomentScalar();
        }

        if (wallKclType() == COL_TYPE_SPECIAL_WALL && wallKclVariant() == 0) {
            dynamics()->addForce(colData.wallNrm * 15.0f);
            collide()->applyWeakFloorMomentScalar();
        }
    }
}

/// @addr{0x8057D1D4}
/// @brief Computes @ref m_standStillBoostRot based on countdown start boost charge, stand-still MT
/// (SSMT) charge, or speed difference from the last frame depending on certain conditions
/// @details If the kart is touching the ground during the race countdown, @ref m_standStillBoostRot
/// is computed based on the start boost charge. If the kart is touching the ground after the race
/// has started and is charging a stand-still MT (SSMT), @ref m_standStillBoostRot is computed based
/// on the SSMT charge.
///
/// If the kart is touching the ground, not charging a SSMT, and does not have a ramp boost or
/// active jump pad, @ref m_standStillBoostRot is computed based on the kart's speed difference from
/// the previous frame. The rotation is further scaled down depending on whether the kart is in a
/// Mushroom boost or a wheelie.
///
/// Finally, @ref m_standStillBoostRot is set based on whether or not the kart is bouncing off of a
/// wall and whether or not it is a bike.
void KartMove::calcStandstillBoostRot() {
    constexpr s16 MAX_SSMT_CHARGE = 75;

    f32 next = 0.0f;
    f32 scalar = 1.0f;

    auto &status = KartObjectProxy::status();

    if (status.onBit(eStatus::TouchingGround)) {
        if (System::RaceManager::Instance()->stage() == System::RaceManager::Stage::Countdown) {
            next = 0.015f * -state()->startBoostCharge();
        } else if (status.offBit(eStatus::ChargingSSMT)) {
            if (status.offBit(eStatus::JumpPad, eStatus::RampBoost,
                        eStatus::SoftWallUnlockRotation)) {
                f32 speedDiff = m_lastSpeed - m_speed;
                scalar = std::min(3.0f, std::max(speedDiff, -3.0f));

                if (status.onBit(eStatus::MushroomBoost)) {
                    next = (scalar * 0.15f) * 0.25f;
                    if (status.onBit(eStatus::Wheelie)) {
                        next *= 0.5f;
                    }
                } else {
                    next = (scalar * 0.15f) * 0.08f;
                }
                scalar = m_driftingParams->boostRotFactor;
            }
        } else {
            next = 0.015f * (-static_cast<f32>(m_ssmtCharge) / static_cast<f32>(MAX_SSMT_CHARGE));
        }
    }

    if (m_flags.onBit(eFlags::WallBounce)) {
        m_standStillBoostRot = isBike() ? next * 3.0f : next * 10.0f;
    } else {
        m_standStillBoostRot += scalar * (next * m_invScale - m_standStillBoostRot);
    }
}

/// @addr{0x805869DC}
/// @brief Responds to player input to handle up/down kart tilt mid-air
/// @details Every frame, the diving rotation decayse by 4%. If the kart is on the ground, in a
/// cannon, in an action, or over a zipper, diving inputs are ignored. If the kart is in a side
/// trick, upwards Y stick inputs are more sensitive towards the max dive magnitude. The Y stick
/// magnitude is then scaled down based on how much airtime has occurred. On the first frame of
/// airtime, it is scaled to 2% and steps up 2% every frame until 50 frames of airtime. If the
/// magnitude of the Y stick input is close to 0, the diving rotation decays further. Finally, the
/// modified Y stick input is applied to the diving rotation, with a maximum increase of 0.005f per
/// frame. Lastly, the diving rotation is added to the X component of @ref KartDynamics::m_angVel2.
///
/// If the kart has more than 50 frames of airtime, gravity is altered based on whether the kart is
/// in a nosedive or a taildive, such that the kart falls faster in a nosedive and slower in a
/// taildive.
void KartMove::calcDive() {
    constexpr f32 DIVE_LIMIT = 0.8f;

    m_divingRot *= 0.96f;

    const auto &status = KartObjectProxy::status();

    if (status.onBit(eStatus::TouchingGround, eStatus::CannonStart, eStatus::InCannon,
                eStatus::InAction, eStatus::OverZipper)) {
        return;
    }

    f32 stickY = state()->stickY();

    if (status.onBit(eStatus::InATrick) && m_jump->type() == TrickType::BikeSideStuntTrick) {
        stickY = std::min(1.0f, stickY + 0.4f);
    }

    u32 airtime = state()->airtime();

    if (airtime > 50) {
        if (EGG::Mathf::abs(stickY) < 0.1f) {
            m_divingRot += 0.05f * (-0.025f - m_divingRot);
        }
    } else {
        stickY *= (airtime / 50.0f);
    }

    m_divingRot = std::max(-DIVE_LIMIT, std::min(DIVE_LIMIT, m_divingRot + stickY * 0.005f));

    EGG::Vector3f nextAngVel2 = angVel2();
    nextAngVel2.x += m_divingRot;
    dynamics()->setAngVel2(nextAngVel2);

    if (state()->airtime() < 50) {
        return;
    }

    EGG::Vector3f topRotated = mainRot().rotateVector(EGG::Vector3f::ey);
    EGG::Vector3f forwardRotated = mainRot().rotateVector(EGG::Vector3f::ez);
    f32 upDotTop = m_up.dot(topRotated);
    EGG::Vector3f upCrossTop = m_up.cross(topRotated);
    f32 crossNorm = upCrossTop.length();
    f32 angle = EGG::Mathf::abs(EGG::Mathf::atan2(crossNorm, upDotTop));

    f32 fVar1 = angle * RAD2DEG - 20.0f;
    if (fVar1 <= 0.0f) {
        return;
    }

    f32 mult = std::min(1.0f, fVar1 / 20.0f);
    if (forwardRotated.y > 0.0f) {
        dynamics()->setGravity((1.0f - 0.2f * mult) * dynamics()->gravity());
    } else {
        dynamics()->setGravity((0.2f * mult + 1.0f) * dynamics()->gravity());
    }
}

/// @addr{0x80583F2C}
/// @brief Computes a collision check for the kart anchored to the underside of the kart's position
/// @param radius The radius of the collision sphere
/// @param scale The scale factor for the kart's position
/// @param pos The position of the kart's origin
/// @param upLocal Out parameter, the local up vector of the kart
/// @param prevPos The previous position of the kart
/// @param colInfo Pointer to store collision information
/// @param maskOut Pointer to store the KCL type mask of the collision
/// @param flags The KCL type mask flags to use for the collision check
/// @return True if a collision occurred, false otherwise
bool KartMove::calcCollisions(f32 radius, f32 scale, EGG::Vector3f &pos, EGG::Vector3f &upLocal,
        const EGG::Vector3f &prevPos, Field::CollisionInfo *colInfo, Field::KCLTypeMask *maskOut,
        Field::KCLTypeMask flags) const {
    upLocal = mainRot().rotateVector(EGG::Vector3f::ey);
    pos = dynamics()->pos() + (-scale * m_scale.y) * upLocal;

    auto *colDir = Field::CollisionDirector::Instance();
    return colDir->checkSphereFullPush(radius, pos, prevPos, flags, colInfo, maskOut, 0);
}

/// @addr{0x80586DB4}
/// @brief Called when an object collision applies a force on the kart
/// @param force The magnitude of the force applied
/// @param hitDir The direction of the force applied
/// @param stop True if the kart's speed should be set to zero, false otherwise
/// @details Also sets a 5 frame bump cooldown which prevents additional forces being applied
/// sporadically.
void KartMove::applyForce(f32 force, const EGG::Vector3f &hitDir, bool stop) {
    constexpr s16 BUMP_COOLDOWN = 5;

    if (m_bumpTimer >= 1) {
        return;
    }

    dynamics()->addForce(force * hitDir.perpInPlane(m_up, true));
    collide()->applyWeakFloorMomentScalar();

    m_bumpTimer = BUMP_COOLDOWN;

    if (stop) {
        m_speed = 0.0f;
    }
}

/// @addr{0x8057CF0C}
/// @brief Calculates the rotation of the kart based on the current turn input and kart state
/// @param turn The yaw/turn magnitude
/// @details If the kart is in an action, no wheels are touching the floor, and the kart is not
/// hopping, then decays @ref KartDynamics::m_angVel0 by 2%. Otherwise, checks if the kart is
/// skidding and calculates the appropriate tile magnitude if so. With the computed tilt magnitude,
/// multiplies it by @ref m_weightedTurn, @ref KartParam::Stats::tilt, and @ref m_invScale to
/// determine the final lean applied to the kart's rotation. Dispatches to @ref
/// calcStandstillBoostRot() and applies the resulting @ref m_standStillBoostRot and computed lean
/// to the kart's @ref KartDynamics::m_angVel0. Also applies the turn parameter to @ref
/// KartDynamics::m_angVel2. Lastly, calls @ref calcDive() to adjust the kart's dive rotation based
/// on the current state.
void KartMove::calcVehicleRotation(f32 turn) {
    f32 tiltMagnitude = 0.0f;
    auto &status = KartObjectProxy::status();

    if (status.offBit(eStatus::InAction, eStatus::SoftWallUnlockRotation) &&
            status.onBit(eStatus::AnyWheelCollision)) {
        EGG::Vector3f front = componentZAxis();
        front = front.perpInPlane(m_up, true);
        EGG::Vector3f frontSpeed = velocity().rej(front).perpInPlane(m_up, false);

        if (frontSpeed.squaredLength() > std::numeric_limits<f32>::epsilon()) {
            tiltMagnitude = frontSpeed.length();

            if (front.z * frontSpeed.x - front.x * frontSpeed.z > 0.0f) {
                tiltMagnitude = -tiltMagnitude;
            }

            tiltMagnitude = std::clamp(tiltMagnitude, -1.0f, 1.0f);
        }
    } else if (status.offBit(eStatus::Hop) || m_hopPosY <= 0.0f) {
        EGG::Vector3f angVel0 = dynamics()->angVel0();
        angVel0.z *= 0.98f;
        dynamics()->setAngVel0(angVel0);
    }

    f32 lean =
            m_invScale * (tiltMagnitude * param()->stats().tilt * EGG::Mathf::abs(m_weightedTurn));

    calcStandstillBoostRot();

    EGG::Vector3f angVel0 = dynamics()->angVel0();
    angVel0.x += m_standStillBoostRot;
    angVel0.z += lean;
    dynamics()->setAngVel0(angVel0);

    EGG::Vector3f nextAngVel2 = angVel2();
    nextAngVel2.y += turn;
    dynamics()->setAngVel2(nextAngVel2);

    calcDive();
}

/// @addr{0x8057EE50}
/// @brief Every frame during a drift, calculates MT/SMT charge based on player input
/// @details If the kart has already charged a super mini-turbo, bails out early. Every frame, the
/// MT charge increases by 2. Additionally, if the X stick input is large enough, the MT charge
/// increases an additional 3 units. If the MT charge exceeds 270, it is clamped at 270 and the
/// drift state advances to @enum DriftState::ChargingSmt to signal that a regular MT boost can be
/// released and that the kart is now charging a super mini-turbo. After this point, the same
/// incrementing logic applies but for super mini-turbos with a charge threshold of 300. Once this
/// threshold is hit, the drift state advances to @enum DriftState::ChargedSmt to signal that a
/// super mini-turbo boost can be released.
void KartMove::calcMtCharge() {
    // TODO: Some of these are shared between the base and derived class implementations.
    constexpr u16 MAX_MT_CHARGE = 270;
    constexpr u16 MAX_SMT_CHARGE = 300;
    constexpr u16 BASE_MT_CHARGE = 2;
    constexpr u16 BASE_SMT_CHARGE = 2;
    constexpr f32 BONUS_CHARGE_STICK_THRESHOLD = 0.4f;
    constexpr u16 EXTRA_MT_CHARGE = 3;

    if (m_driftState == DriftState::ChargedSmt) {
        return;
    }

    f32 stickX = state()->stickX();

    if (m_driftState == DriftState::ChargingMt) {
        m_mtCharge += BASE_MT_CHARGE;

        if (-BONUS_CHARGE_STICK_THRESHOLD <= stickX) {
            if (BONUS_CHARGE_STICK_THRESHOLD < stickX && m_hopStickX == -1) {
                m_mtCharge += EXTRA_MT_CHARGE;
            }
        } else if (m_hopStickX != -1) {
            m_mtCharge += EXTRA_MT_CHARGE;
        }

        if (m_mtCharge > MAX_MT_CHARGE) {
            m_mtCharge = MAX_MT_CHARGE;
            m_driftState = DriftState::ChargingSmt;
        }
    }

    if (m_driftState != DriftState::ChargingSmt) {
        return;
    }

    m_smtCharge += BASE_SMT_CHARGE;

    if (-BONUS_CHARGE_STICK_THRESHOLD <= stickX) {
        if (BONUS_CHARGE_STICK_THRESHOLD < stickX && m_hopStickX == -1) {
            m_smtCharge += EXTRA_MT_CHARGE;
        }
    } else if (m_hopStickX != -1) {
        m_smtCharge += EXTRA_MT_CHARGE;
    }

    if (m_smtCharge > MAX_SMT_CHARGE) {
        m_smtCharge = MAX_SMT_CHARGE;
        m_driftState = DriftState::ChargedSmt;
    }
}

/// @addr{0x8057DA5C}
/// @brief Called when beginning a manual drift hop
/// @details Sets the @enum eStatus::Hop flag. Cancels wheelies (if the vehicle is a bike).
/// Calculates hop direction vectors based on the kart's current orientation. Initializes manual
/// drift-related members. Applies initial hop velocity to @ref KartDynamics::m_extVel and clears
/// the Y component of @ref KartDynamics::m_totalForce.
void KartMove::startHop() {
    status().setBit(eStatus::Hop).resetBit(eStatus::DriftManual);
    onPreDrift();

    m_hopUp = mainRot().rotateVector(EGG::Vector3f::ey);
    m_hopDir = mainRot().rotateVector(EGG::Vector3f::ez);
    m_driftState = DriftState::NotDrifting;
    m_smtCharge = 0;
    m_mtCharge = 0;
    m_hopStickX = 0;
    m_hopFrame = 0;
    m_hopPosY = 0.0f;
    m_hopGravity = dynamics()->gravity();
    m_hopVelY = m_driftingParams->hopVelY;
    m_outsideDriftBonus = 0.0f;

    EGG::Vector3f nextExtVel = extVel();
    nextExtVel.y = 0.0f + m_hopVelY;
    dynamics()->setExtVel(nextExtVel);

    EGG::Vector3f totalForce = dynamics()->totalForce();
    totalForce.y = 0.0f;
    dynamics()->setTotalForce(totalForce);
}

/// @addr{0x8057FD18}
/// @brief Applies calculations to start interacting with KCL #COL_TYPE_JUMP_PAD
/// @details If the kart is in a respawn, action, or zipper, bails out early. Sets the @enum
/// eStatus::JumpPad flag. Fetches the @ref JumpPadProperties corresponding to the current jump pad
/// variant. If the variant is a Mushroom Gorge ramp (3) or a bouncy mushroom (4), then it overrides
/// the previously fetched @ref JumpPadProperties so that the kart has more speed and also sets the
/// @enum eStatus::JumpPadFixedSpeed flag.
///
/// If the variant is a bouncy mushroom (4), also sets the @enum eStatus::JumpPadMushroomTrigger,
/// @enum eStatus::JumpPadMushroomVelYInc, and @enum eStatus::JumpPadMushroom flags. Otherwise, the
/// jump pad applies vertical external velocity to the vehicle defined by ref
/// JumpPadProperties::velY and clears the vertical component of @ref KartDynamics::m_totalForce. If
/// the jump pad variant is not a Mushroom Gorge ramp (3) or a bouncy mushroom (4), then the kart's
/// speed is scaled based on whether the kart's forward direction was angled upward or downward.
/// Additionally, the kart's facing direction is snapped to lie within the XZ plane and the kart's
/// internal velocity direction snaps to match this same facing direction. Also sets @enum
/// eStatus::JumpPadDisableYsusForce so that the kart does not bounce when it lands.
///
/// Regardless of the jump pad variant, the min and max speed are cached from the @ref
/// JumpPadProperties and the speed is clamped to the min jump pad speed.
/// @bug The ramps on Mushroom Gorge have variant 3, which sets the @enum eStatus::JumpPadFixedSpeed
/// flag. This flag is only cleared in @ref tryEndJumpPad() if @enum eStatus::JumpPadMushroomTrigger
/// is ALSO set. This means that if you hit a variant 3 ramp without also hitting a mushroom before
/// landing back on the ground, @enum eStatus::JumpPadFixedSpeed will not be cleared. This results
/// in what is known as the "Off-Road Glitch", where the player can now drive through offroad
/// without any drop in speed. Subsequently bouncing off a mushroom (variant 4) results in the flag
/// being correctly cleared.
void KartMove::tryStartJumpPad() {
    static constexpr std::array<JumpPadProperties, 8> JUMP_PAD_PROPERTIES = {{
            {50.0f, 50.0f, 35.0f},
            {50.0f, 50.0f, 47.0f},
            {59.0f, 59.0f, 30.0f},
            {73.0f, 73.0f, 45.0f},
            {73.0f, 73.0f, 53.0f},
            {56.0f, 56.0f, 50.0f},
            {55.0f, 55.0f, 35.0f},
            {56.0f, 56.0f, 50.0f},
    }};

    auto &status = KartObjectProxy::status();

    if (status.onBit(eStatus::BeforeRespawn, eStatus::InAction, eStatus::HalfPipeRamp)) {
        return;
    }

    status.setBit(eStatus::JumpPad);
    s32 jumpPadVariant = state()->jumpPadVariant();
    m_jumpPadProperties = &JUMP_PAD_PROPERTIES[jumpPadVariant];

    if (jumpPadVariant == 3 || jumpPadVariant == 4) {
        if (m_jumpPadBoostMultiplier > 1.3f || m_jumpPadSoftSpeedLimit > 110.0f) {
            // Set speed to 100 if the player has boost from a boost panel or mushroom(item) before
            // hitting the jump pad
            static constexpr std::array<JumpPadProperties, 2> JUMP_PAD_PROPERTIES_SHROOM_BOOST = {{
                    {100.0f, 100.0f, 70.0f},
                    {100.0f, 100.0f, 65.0f},
            }};
            m_jumpPadProperties = &JUMP_PAD_PROPERTIES_SHROOM_BOOST[jumpPadVariant != 3];
        }

        status.setBit(eStatus::JumpPadFixedSpeed);
    }

    if (jumpPadVariant == 4) {
        status.setBit(eStatus::JumpPadMushroomTrigger, eStatus::JumpPadMushroomVelYInc,
                eStatus::JumpPadMushroom);
    } else {
        EGG::Vector3f nextExtVel = extVel();
        EGG::Vector3f totalForce = dynamics()->totalForce();

        nextExtVel.y = m_jumpPadProperties->velY;
        totalForce.y = 0.0f;

        dynamics()->setExtVel(nextExtVel);
        dynamics()->setTotalForce(totalForce);

        if (jumpPadVariant != 3) {
            EGG::Vector3f dir = m_dir;
            dir.y = 0.0f;
            dir.normalise();
            m_speed *= m_dir.dot(dir);
            m_dir = dir;
            m_intVelDir = dir;
            status.setBit(eStatus::JumpPadDisableYsusForce);
        }
    }

    m_jumpPadMinSpeed = m_jumpPadProperties->minSpeed;
    m_jumpPadMaxSpeed = m_jumpPadProperties->maxSpeed;
    m_speed = std::max(m_speed, m_jumpPadMinSpeed);
}

/// @addr{0x80582530}
/// @brief Checks if the jump pad effect should end for the kart
/// @details If the kart lands on the ground after bouncing off a mushroom, clears @enum
/// eStatus::JumpPadMushroomTrigger, @enum eStatus::JumpPadFixedSpeed, and @enum
/// eStatus::JumpPadMushroomVelYInc. If the kart bounced off a mushroom and @enum
/// eStatus::JumpPadMushroomVelYInc is set, applies 20 units up upward external velocity to the
/// kart. It will keep adding 20 units every frame until the external velocity reaches the Y
/// velocity defined by the jump pad properties, at which point it clears @enum
/// eStatus::JumpPadMushroomVelYInc. Finally, if the kart landed on the ground, the jump pad state
/// is cancelled.
/// @bug The ramps on Mushroom Gorge have variant 3, which sets the @enum eStatus::JumpPadFixedSpeed
/// flag in @ref tryStartJumpPad(). This flag is only cleared in @ref this function if @enum
/// eStatus::JumpPadMushroomTrigger is ALSO set. This means that if you hit a variant 3 ramp without
/// also hitting a mushroom before landing back on the ground, @enum eStatus::JumpPadFixedSpeed will
/// not be cleared. This results in what is known as the "Off-Road Glitch", where the player can now
/// drive through offroad without any drop in speed. Subsequently bouncing off a mushroom (variant
/// 4) results in the flag being correctly cleared.
void KartMove::tryEndJumpPad() {
    auto &status = KartObjectProxy::status();
    if (status.onBit(eStatus::JumpPadMushroomTrigger)) {
        if (status.onBit(eStatus::GroundStart)) {
            status.resetBit(eStatus::JumpPadMushroomTrigger, eStatus::JumpPadFixedSpeed,
                    eStatus::JumpPadMushroomVelYInc);
        }

        if (status.onBit(eStatus::JumpPadMushroomVelYInc)) {
            EGG::Vector3f newExtVel = extVel();
            newExtVel.y += 20.0f;
            if (m_jumpPadProperties->velY < newExtVel.y) {
                newExtVel.y = m_jumpPadProperties->velY;
                status.resetBit(eStatus::JumpPadMushroomVelYInc);
            }
            dynamics()->setExtVel(newExtVel);
        }
    }

    if (status.onBit(eStatus::GroundStart) && status.offBit(eStatus::JumpPadMushroomTrigger)) {
        cancelJumpPad();
    }
}

/// @addr{0x8057F3D8}
/// @brief Activates the mushroom boost for the kart
/// @details The boost lasts for 90 frames and grants offroad invincibility during this time.
void KartMove::activateMushroom() {
    constexpr s16 MUSHROOM_DURATION = 90;

    auto &status = KartObjectProxy::status();

    if (status.onBit(eStatus::BeforeRespawn, eStatus::InAction)) {
        return;
    }

    activateBoost(KartBoost::Type::MushroomAndBoostPanel, MUSHROOM_DURATION);

    m_mushroomBoostTimer = MUSHROOM_DURATION;
    status.setBit(eStatus::MushroomBoost);
    setOffroadInvincibility(MUSHROOM_DURATION);
}

/// @addr{0x8057F96C}
/// @brief Activates the zipper boost for the kart
/// @details If the player tricked on the zipper, the duration is 100 frames, otherwise it's 50
/// frames. During this time, the kart also has offroad invinsibility.
void KartMove::activateZipperBoost() {
    constexpr s16 BASE_DURATION = 50;
    constexpr s16 TRICK_DURATION = 100;

    auto &status = KartObjectProxy::status();

    if (status.onBit(eStatus::BeforeRespawn, eStatus::InAction)) {
        return;
    }

    s16 boostDuration = status.onBit(eStatus::ZipperTrick) ? TRICK_DURATION : BASE_DURATION;
    activateBoost(KartBoost::Type::TrickAndZipper, boostDuration);

    setOffroadInvincibility(boostDuration);
    m_zipperBoostTimer = 0;
    m_zipperBoostDuration = boostDuration;
    status.setBit(eStatus::ZipperBoost);
}

/// @addr{0x80582E34}
/// @brief Calculates the duration of the active zipper boost
/// @details This function does nothing if @enum eStatus::ZipperBoost is not set. Sets
/// @eStatus::Accelerate to indicate the kart is accelerating. Increments the @ref
/// m_zipperBoostTimer to track the duration of the boost and clears @enum eStatus::ZipperBoost once
/// it exceeds the boost duration. For the first 9 frames of the boost, this function clears the Y
/// component of @ref KartDynamics::m_angVel0.
void KartMove::calcZipperBoost() {
    auto &status = KartObjectProxy::status();

    if (status.offBit(eStatus::ZipperBoost)) {
        return;
    }

    status.setBit(eStatus::Accelerate);

    if (status.offBit(eStatus::OverZipper) && ++m_zipperBoostTimer >= m_zipperBoostDuration) {
        m_zipperBoostTimer = 0;
        status.resetBit(eStatus::ZipperBoost);
    }

    if (m_zipperBoostTimer < 10) {
        EGG::Vector3f angVel = dynamics()->angVel0();
        angVel.y = 0.0f;
        dynamics()->setAngVel0(angVel);
    }
}

/// @addr{0x8058160C}
/// @brief Updates the kart's scale and the kart's hitbox scale
/// @details Also computes the inverse of the Z-component of the scale to avoid frequent division
/// elsewhere.
void KartMove::calcScale() {
    m_kartScale->calc();

    const EGG::Vector3f sizeScale = m_kartScale->shrinkScale();
    setScale(m_kartScale->pressScale() * sizeScale);
    m_totalScale = m_shockSpeedMultiplier;
    m_hitboxScale = std::max(sizeScale.z, m_totalScale);

    if (sizeScale.z != 1.0f) {
        setInertiaScale(m_scale);
    }

    m_invScale = m_scale.z > 1.0f ? 1.0f / m_scale.z : 1.0f;
}

/// @addr{0x80580778}
/// @brief Applies the shrink effect to the kart for a specified duration.
/// @param timer The duration for which the shrink effect should be applied
/// @details Also clears the kart's item inventory.
void KartMove::applyShrink(u16 timer) {
    auto &status = state()->status();

    if (status.onBit(eStatus::InRespawn, eStatus::AfterRespawn, eStatus::CannonStart)) {
        return;
    }

    action()->start(Action::SpinShrink);
    Item::ItemDirector::Instance()->kartItem(0).clear();
    status.setBit(eStatus::Shocked);

    if (timer > m_shockTimer) {
        m_shockTimer = timer;
        m_kartScale->startShrink(false);
    }
}

/// @addr{0x8058498C}
/// @brief Called when the kart enters a cannon
/// @details Resets most of the kart's movement state, including drifts and boosts. Computes the
/// direction and length the kart will travel in the cannon, and changes the kart's forward
/// direction to the cannon travel direction.
void KartMove::enterCannon() {
    reset(true, true);
    physics()->clearDecayingRot();
    m_boost.resetActive();

    auto &status = KartObjectProxy::status();

    status.resetBit(eStatus::Boost);

    cancelJumpPad();
    clearRampBoost();
    clearZipperBoost();
    clearSsmt();
    clearOffroadInvincibility();

    dynamics()->reset();

    clearDrift();

    status.resetBit(eStatus::Hop, eStatus::CannonStart)
            .setBit(eStatus::InCannon, eStatus::SkipWheelCalc);

    const auto [cannonPos, cannonDir] = getCannonPosRot();
    m_cannonEntryPos = pos();
    m_cannonDir = cannonPos - pos();
    m_cannonLength = m_cannonDir.normalise();
    m_cannonDir.normalise();
    m_dir = m_cannonDir;
    m_intVelDir = m_cannonDir;
    m_cannonOrthog = EGG::Vector3f::ey.perpInPlane(m_cannonDir, true);
    m_cannonProgress.setZero();
}

/// @addr{0x80584D58}
/// @brief Called when the kart is traveling through a cannon
/// @details Fetches the position and direction towards the cannon exit. Computes the remaining
/// distance vector from where the kart currently is in the cannon. Builds an orthonormal basis from
/// the remaining distance vector and the up vector, then extracts the upward component to make the
/// kart pitch up or down during the cannon. If the kart is within 30 units of the cannon exit or
/// the kart has overshot the exit, the cannon is exited via @ref exitCannon(). Otherwise, the kart
/// continues to travel through the cannon.
///
/// Sets the kart's forward direction based on the cannonorthonormal basis. If the remaining
/// distance to the cannon exit is within the deceleration threshold, interpolates the kart's speed
/// from @ref CannonParameter::speed to @ref CannonParameter::endSpeed based on how far the kart is
/// along the deceleration zone. Accumulates the distance traveled to @ref m_cannonProgress. If @ref
/// CannonParameter::height is non-zero, computes an additional half-sine arc as the kart travels
/// along the cannon path. Updates the kart's position to reflect the current speed in the cannon.
/// Also locks the kart's facing direction to the cannon's direction. Smooths the kart's orientation
/// toward the cannon path's tangent direction. Finally, zeroes the kart's external velocity.
void KartMove::calcCannon() {
    auto [cannonPos, cannonDir] = getCannonPosRot();
    EGG::Vector3f forwardXZ = cannonPos - m_cannonEntryPos - m_cannonProgress;
    EGG::Vector3f forward = forwardXZ;
    f32 forwardLength = forward.normalise();
    forwardXZ.y = 0;
    forwardXZ.normalise();
    EGG::Vector3f local94 = m_cannonDir;
    local94.y = 0;
    local94.normalise();
    m_speedRatioCapped = 1.0f;
    m_speedRatio = 1.5f;
    EGG::Matrix34f cannonOrientation;
    cannonOrientation.makeOrthonormalBasis(forward, EGG::Vector3f::ey);
    EGG::Vector3f up = cannonOrientation.multVector33(EGG::Vector3f::ey);
    m_smoothedUp = up;
    m_up = up;

    if (forwardLength < 30.0f || local94.dot(forwardXZ) <= 0.0f) {
        exitCannon();
        return;
    }

    m_smoothedForward = cannonOrientation.multVector33(EGG::Vector3f::ez);
    m_speed = m_baseSpeed;
    const auto *cannonPoint =
            System::CourseMap::Instance()->getCannonPoint(state()->cannonPointId());
    size_t cannonParameterIdx = std::max<s16>(0, cannonPoint->parameterIdx());
    ASSERT(cannonParameterIdx < CANNON_PARAMETERS.size());
    const auto &cannonParams = CANNON_PARAMETERS[cannonParameterIdx];
    f32 newSpeed = cannonParams.speed;
    if (forwardLength < cannonParams.decelThreshold) {
        f32 factor = std::max(0.0f, forwardLength / cannonParams.decelThreshold);

        newSpeed = cannonParams.endSpeed;
        if (newSpeed <= 0.0f) {
            newSpeed = m_baseSpeed;
        }

        newSpeed += factor * (cannonParams.speed - newSpeed);
        if (cannonParams.endSpeed > 0.0f) {
            m_speed = std::min(newSpeed, m_speed);
        }
    }

    m_cannonProgress += m_cannonDir * newSpeed;

    EGG::Vector3f newPos = EGG::Vector3f::zero;
    if (cannonParams.height > 0.0f) {
        f32 fVar9 =
                EGG::Mathf::SinFIdx((1.0f - (forwardLength / m_cannonLength)) * 180.0f * DEG2FIDX);
        newPos = fVar9 * cannonParams.height * m_cannonOrthog;
    }

    dynamics()->setPos(m_cannonEntryPos + m_cannonProgress + newPos);
    m_dir = m_cannonDir;
    m_intVelDir = m_cannonDir;

    calcRotCannon(forward);

    dynamics()->setExtVel(EGG::Vector3f::zero);
}

/// @addr{0x805855BC}
/// @brief Calculates the rotation of the kart while in the cannon based on the forward direction
/// @param forward The target forward direction vector of the kart while in the cannon
void KartMove::calcRotCannon(const EGG::Vector3f &forward) {
    EGG::Vector3f targetDir = forward;
    targetDir.normalise();
    EGG::Vector3f currForward = bodyForward();
    EGG::Vector3f smoothForward = currForward + ((targetDir - currForward) * 0.3f);
    currForward.normalise();
    smoothForward.normalise();
    EGG::Quatf local80;
    local80.makeVectorRotation(currForward, smoothForward);
    local80 *= fullRot();
    local80.normalise();
    EGG::Quatf localB8;
    localB8.makeVectorRotation(local80.rotateVector(EGG::Vector3f::ey), smoothedUp());
    EGG::Quatf newRot = local80.slerpTo(localB8.multSwap(local80), 0.3f);
    dynamics()->setFullRot(newRot);
    dynamics()->setMainRot(newRot);
}

/// @addr{0x805852C8}
/// @brief Called when the kart begins dropping from the cannon
/// @details Transitions the kart from @enum eStatus::InCannon to @enum eStatus::AfterCannon, and
/// updates the vehicle's internval velocity to reflect the kart's non-cannon speed.
void KartMove::exitCannon() {
    auto &status = KartObjectProxy::status();

    if (status.offBit(eStatus::InCannon)) {
        return;
    }

    status.resetBit(eStatus::InCannon, eStatus::SkipWheelCalc).setBit(eStatus::AfterCannon);
    dynamics()->setIntVel(m_cannonDir * m_speed);
}

/// @addr{0x80587B30}
/// @brief Constructor
KartMoveBike::KartMoveBike() : m_leanRot(0.0f) {}

/// @addr{0x80589704}
/// @brief Default destructor
KartMoveBike::~KartMoveBike() = default;

/// @addr{0x80588350}
/// @brief Sets the wheelie bit flag and some wheelie-related variables
/// @details In the base game this is a virtual function, but since no derived class overrides
/// it here, we can divirtualize for Kinoko.
void KartMoveBike::startWheelie() {
    constexpr f32 MAX_WHEELIE_ROTATION = 0.07f;
    constexpr u16 WHEELIE_COOLDOWN = 20;

    status().setBit(eStatus::Wheelie);
    m_wheelieFrames = 0;
    m_maxWheelieRot = MAX_WHEELIE_ROTATION;
    m_wheelieCooldown = WHEELIE_COOLDOWN;
    m_wheelieRotDec = 0.0f;
    m_autoHardStickXFrames = 0;
}

/// @addr{0x80587D68}
/// @copybrief KartMove::calcVehicleRotation()
/// @param turn The yaw/turn magnitude
/// @details Computes the bike's lean rotation incrementor and cap based on whether the kart is
/// charging a stand-still MT, the countdown has ended, and whether the kart's speed is below 5. The
/// incrementor and cap is interpolated towards their target values to smoothen the bike's leaning
/// behavior.
///
/// If the kart is in a state such that it cannot lean, the lean rotation decays by @ref
/// TurningParameters::leanRotDecayFactor. Otherwise, if the kart is not drifting, the lean rotation
/// is adjusted based on the stick input. Otherwise, if the kart is drifting, the lean rotation is
/// clamped to @ref TurningParameters::leanRotMinDrift and @ref TurningParameters::leanRotMaxDrift.
/// If the kart is hopping to the left, the min and max are swapped to reflect the hop direction. If
/// the kart is hopping to the left or right but the player is not touching the X stick input, the
/// lean rotation decays towards 0.5f. Otherwise, the lean rotation increments based on the
/// magnitude of the stick input scaled by @ref TurningParameters::driftStickXFactor. The lean
/// rotation is then clamped to the min and max values. If the lean rotation was not capped, then
/// applies @ref TurningParameters::leanRotExtVelFactor to the kart's external velocity.
///
/// Dispatches to @ref calcStandstillBoostRot() and applies the resulting @ref m_standStillBoostRot,
/// turn parameter, and @ref m_leanRot to the @ref KartDynamics::m_angVel2. Calls @ref calcDive() to
/// apply mid-air dive rotation. Lastly, sets @ref KartDynamics::m_stabilizeUp by interpolating @ref
/// m_up towards the world up vector based on the kart's speed ratio.
void KartMoveBike::calcVehicleRotation(f32 turn) {
    f32 leanRotInc = m_turningParams->leanRotIncRace;
    f32 leanRotCap = m_turningParams->leanRotCapRace;
    const auto *raceManager = System::RaceManager::Instance();

    auto &status = KartObjectProxy::status();

    if (status.offBit(eStatus::ChargingSSMT)) {
        if (!raceManager->isStageReached(System::RaceManager::Stage::Race) ||
                EGG::Mathf::abs(m_speed) < 5.0f) {
            leanRotInc = m_turningParams->leanRotIncCountdown;
            leanRotCap = m_turningParams->leanRotCapCountdown;
        }
    } else {
        leanRotInc = m_turningParams->leanRotIncSSMT;
        leanRotCap = m_turningParams->leanRotCapSSMT;
    }

    m_leanRotCap += 0.3f * (leanRotCap - m_leanRotCap);
    m_leanRotInc += 0.3f * (leanRotInc - m_leanRotInc);

    f32 stickX = state()->stickX();
    f32 extVelXFactor = 0.0f;
    f32 leanRotMin = -m_leanRotCap;
    f32 leanRotMax = m_leanRotCap;

    if (status.onBit(eStatus::BeforeRespawn, eStatus::InAction, eStatus::Wheelie,
                eStatus::OverZipper, eStatus::RejectRoadTrigger, eStatus::AirtimeOver20,
                eStatus::SoftWallUnlockRotation, eStatus::SoftWallPush, eStatus::HWG,
                eStatus::CannonStart, eStatus::InCannon)) {
        m_leanRot *= m_turningParams->leanRotDecayFactor;
    } else if (!state()->isDrifting()) {
        if (stickX <= 0.2f) {
            if (stickX >= -0.2f) {
                m_leanRot *= m_turningParams->leanRotDecayFactor;
            } else {
                m_leanRot -= m_leanRotInc;
                extVelXFactor = m_turningParams->leanRotExtVelFactor;
            }
        } else {
            m_leanRot += m_leanRotInc;
            extVelXFactor = -m_turningParams->leanRotExtVelFactor;
        }
    } else {
        leanRotMax = m_turningParams->leanRotMaxDrift;
        leanRotMin = m_turningParams->leanRotMinDrift;

        if (m_hopStickX == 1) {
            leanRotMin = -leanRotMax;
            leanRotMax = -m_turningParams->leanRotMinDrift;
        }
        if (m_hopStickX == -1) {
            if (stickX == 0.0f) {
                m_leanRot += (0.5f - m_leanRot) * 0.05f;
            } else {
                m_leanRot += m_turningParams->driftStickXFactor * stickX;
                extVelXFactor = -m_turningParams->leanRotExtVelFactor * stickX;
            }
        } else if (stickX == 0.0f) {
            m_leanRot += (-0.5f - m_leanRot) * 0.05f;
        } else {
            m_leanRot += m_turningParams->driftStickXFactor * stickX;
            extVelXFactor = -m_turningParams->leanRotExtVelFactor * stickX;
        }
    }

    bool capped = false;
    if (leanRotMin <= m_leanRot) {
        if (leanRotMax < m_leanRot) {
            m_leanRot = leanRotMax;
            capped = true;
        }
    } else {
        m_leanRot = leanRotMin;
        capped = true;
    }

    if (!capped) {
        dynamics()->setExtVel(extVel() + componentXAxis() * extVelXFactor);
    }

    f32 leanRotScalar = state()->isDrifting() ? 0.065f : 0.05f;

    calcStandstillBoostRot();

    dynamics()->setAngVel2(angVel2() +
            EGG::Vector3f(m_standStillBoostRot, turn * wheelieRotFactor(),
                    m_leanRot * leanRotScalar));

    calcDive();

    EGG::Vector3f top = m_up;

    if (status.offBit(eStatus::RejectRoad, eStatus::HalfPipeRamp, eStatus::OverZipper)) {
        f32 scalar = (m_speed >= 0.0f) ? m_speedRatioCapped * 2.0f : 0.0f;
        scalar = std::min(1.0f, scalar);
        top = scalar * m_up + (1.0f - scalar) * EGG::Vector3f::ey;

        if (std::numeric_limits<f32>::epsilon() < top.squaredLength()) {
            top.normalise();
        }
    }

    dynamics()->setStabilizeUp(top);
}

/// @addr{0x80587C54}
/// @copybrief KartMove::setTurnParams()
/// @details On init, sets the bike's lean rotation cap and increment.
/// In addition to setting the lean rotation cap and increment on init,
/// this function also gets called when falling out-of-bounds.
void KartMoveBike::setTurnParams() {
    static constexpr std::array<TurningParameters, 2> TURNING_PARAMS_ARRAY = {{
            {0.8f, 0.08f, 1.0f, 0.1f, 1.2f, 0.8f, 0.08f, 0.6f, 0.15f, 1.6f, 0.9f, 180},
            {1.0f, 0.1f, 1.0f, 0.05f, 1.5f, 0.7f, 0.08f, 0.6f, 0.15f, 1.3f, 0.9f, 180},
    }};

    KartMove::setTurnParams();

    if (vehicleType() == KartParam::Stats::DriftType::Outside_Drift_Bike) {
        m_turningParams = &TURNING_PARAMS_ARRAY[0];
    } else if (vehicleType() == KartParam::Stats::DriftType::Inside_Drift_Bike) {
        m_turningParams = &TURNING_PARAMS_ARRAY[1];
    }

    if (System::RaceManager::Instance()->isStageReached(System::RaceManager::Stage::Race)) {
        m_leanRotInc = m_turningParams->leanRotIncRace;
        m_leanRotCap = m_turningParams->leanRotCapRace;
    } else {
        m_leanRotInc = m_turningParams->leanRotIncCountdown;
        m_leanRotCap = m_turningParams->leanRotCapCountdown;
    }
}

/// @addr{0x80587D00}
/// @copybrief KartMove::reset()
/// @param preserveScale Whether to preserve the kart's current scale
/// @param preserveFloorCount Whether to preserve the number of colliding floors
/// @details Additionally resets the bike-specific wheelie and lean rotation parameters
void KartMoveBike::reset(bool preserveScale, bool preserveFloorCount) {
    KartMove::reset(preserveScale, preserveFloorCount);

    m_leanRot = 0.0f;
    m_leanRotCap = 0.0f;
    m_leanRotInc = 0.0f;
    m_wheelieRot = 0.0f;
    m_maxWheelieRot = 0.0f;
    m_wheelieFrames = 0;
    m_wheelieCooldown = 0;
    m_autoHardStickXFrames = 0;
}

/// @addr{0x805883F4}
/// @copybrief KartMove::calcWheelie()
/// @details Checks player inputs to see if a wheelie should start or end. Decrements the @ref
/// m_wheelieCooldown timer. If the kart is not in a wheelie, then decreases the pitch rotation
/// since the kart is lowering from a wheelie. Otherwise, either increments the @ref
/// m_autoHardStickXFrames timer or resets it depending on whether the player is holding a steep
/// enough X stick input. If the wheelie duration has exceeded the limit or the player started an
/// automatic drift due to holding the X stick input long enough, cancels the wheelie. Otherwise if
/// the wheelie is continuing, increments the wheelie rotation and decays @ref
/// KartDynamics::m_angVel0. Lastly, if the kart is driving on a slope of 45 degrees or greater, the
/// wheelie will be cancelled after 15 frames. While in the wheelie, the @ref m_wheelieRot is
/// applied to @ref KartDynamics::m_angVel2.
void KartMoveBike::calcWheelie() {
    constexpr u32 FAILED_WHEELIE_FRAMES = 15;
    constexpr s16 AUTO_WHEELIE_CANCEL_FRAMES = 15;
    constexpr f32 AUTO_WHEELIE_CANCEL_STICK_THRESHOLD = 0.85f;

    calcWheelieInput();
    m_wheelieCooldown = std::max(0, m_wheelieCooldown - 1);

    auto &status = KartObjectProxy::status();

    if (status.onBit(eStatus::Wheelie)) {
        bool cancelAutoWheelie = false;

        if (status.offBit(eStatus::AutoDrift) ||
                EGG::Mathf::abs(state()->stickX()) <= AUTO_WHEELIE_CANCEL_STICK_THRESHOLD) {
            m_autoHardStickXFrames = 0;
        } else {
            if (++m_autoHardStickXFrames > AUTO_WHEELIE_CANCEL_FRAMES) {
                cancelAutoWheelie = true;
            }
        }

        ++m_wheelieFrames;
        if (m_turningParams->maxWheelieFrames < m_wheelieFrames || cancelAutoWheelie ||
                (!canWheelie() && FAILED_WHEELIE_FRAMES <= m_wheelieFrames)) {
            cancelWheelie();
        } else {
            m_wheelieRot += 0.01f;
            EGG::Vector3f angVel0 = dynamics()->angVel0();
            angVel0.x *= 0.9f;
            dynamics()->setAngVel0(angVel0);
        }
    } else if (0.0f < m_wheelieRot) {
        m_wheelieRotDec -= 0.001f;
        m_wheelieRotDec = std::max(-0.03f, m_wheelieRotDec);
        m_wheelieRot += m_wheelieRotDec;
    }

    m_wheelieRot = std::clamp(m_wheelieRot, 0.0f, m_maxWheelieRot);

    f32 vel1DirUp = m_intVelDir.dot(EGG::Vector3f::ey);

    if (m_wheelieRot > 0.0f) {
        if (vel1DirUp <= 0.5f || m_wheelieFrames < FAILED_WHEELIE_FRAMES) {
            EGG::Vector3f nextAngVel2 = angVel2();
            nextAngVel2.x -= m_wheelieRot * (1.0f - EGG::Mathf::abs(vel1DirUp));
            dynamics()->setAngVel2(nextAngVel2);
        } else {
            cancelWheelie();
        }

        status.setBit(eStatus::WheelieRot);
    } else {
        status.resetBit(eStatus::WheelieRot);
    }
}

/// @addr{0x80588888}
/// @copybrief KartMove::calcMtCharge()
/// @details If the kart is not actually in the process of charging a mini-turbo, bails out early.
/// Every frame, the MT charge increases by 2. Additionally, if the X stick input is large enough,
/// the MT charge increases an additional 3 units. If the MT charge exceeds 270, it is clamped at
/// 270 and the drift state advances to @enum DriftState::ChargedMt to signal that a boost can be
/// released.
void KartMoveBike::calcMtCharge() {
    constexpr u16 MAX_MT_CHARGE = 270;
    constexpr u16 BASE_MT_CHARGE = 2;
    constexpr f32 BONUS_CHARGE_STICK_THRESHOLD = 0.4f;
    constexpr u16 EXTRA_MT_CHARGE = 3;

    if (m_driftState != DriftState::ChargingMt) {
        return;
    }

    m_mtCharge += BASE_MT_CHARGE;

    f32 stickX = state()->stickX();
    if (-BONUS_CHARGE_STICK_THRESHOLD <= stickX) {
        if (BONUS_CHARGE_STICK_THRESHOLD < stickX && m_hopStickX == -1) {
            m_mtCharge += EXTRA_MT_CHARGE;
        }
    } else if (m_hopStickX != -1) {
        m_mtCharge += EXTRA_MT_CHARGE;
    }

    if (m_mtCharge > MAX_MT_CHARGE) {
        m_mtCharge = MAX_MT_CHARGE;
        m_driftState = DriftState::ChargedMt;
    }
}

/// @addr{0x80588798}
/// @brief Every frame, checks player input to see if we should start or stop a wheelie
/// @details If the kart is currently wheeling, checks to see if the player's controller state is
/// pressing "trick down" to try to cancel the wheelie. It will successfully cancel the wheelie if
/// the cooldown timer is 0, at which point the timer will be set to 20 frames. Otherwise, if the
/// kart is not yet in a wheelie, checks to see if the player's controller state is pressing "trick
/// up", the kart is not drifting, in an action, colliding with a wall, or hopping. If all those
/// conditions are met, and the wheelie cooldown timer is 0, starts a wheelie.
void KartMoveBike::calcWheelieInput() {
    constexpr s16 COOLDOWN_FRAMES = 20;

    bool dpadUp = inputs()->currentState().trickUp();
    auto &status = KartObjectProxy::status();

    if (status.offBit(eStatus::Wheelie)) {
        if (dpadUp && status.onBit(eStatus::TouchingGround)) {
            if (status.onBit(eStatus::DriftManual, eStatus::WallCollision, eStatus::Wall3Collision,
                        eStatus::Hop, eStatus::DriftAuto, eStatus::InAction)) {
                return;
            }

            if (m_wheelieCooldown > 0) {
                return;
            }

            startWheelie();
        }
    } else if (inputs()->currentState().trickDown() && m_wheelieCooldown <= 0) {
        cancelWheelie();
        m_wheelieCooldown = COOLDOWN_FRAMES;
    }
}

} // namespace Kinoko::Kart
