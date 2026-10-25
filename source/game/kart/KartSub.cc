#include "KartSub.hh"

#include "game/kart/CollisionGroup.hh"
#include "game/kart/KartAction.hh"
#include "game/kart/KartBody.hh"
#include "game/kart/KartCollide.hh"
#include "game/kart/KartDynamics.hh"
#include "game/kart/KartMove.hh"
#include "game/kart/KartPhysics.hh"
#include "game/kart/KartState.hh"
#include "game/kart/KartSuspensionPhysics.hh"

#include "game/field/BoxColManager.hh"
#include "game/field/CollisionDirector.hh"

#include "game/system/RaceConfig.hh"
#include "game/system/RaceManager.hh"

namespace Kinoko::Kart {

/// @brief Default constructor
KartSub::KartSub() = default;

/// @addr{0x80598AC8}
/// @brief Virtual destructor that destroys all underyling subsystems
KartSub::~KartSub() {
    EGG::egg_delete(m_collide);
    EGG::egg_delete(m_state);
    EGG::egg_delete(m_move);
    EGG::egg_delete(m_action);
}

/// @addr{0x80595D48}
/// @brief Creates the kart's subsystems with the provided stats
/// @param isBike Indicates if the vehicle is a bike
/// @param stats The stats to initialize the subsystems with
/// @details Creates @ref m_move based on whether the vehicle is a bike (@ref KartMoveBike) or a
/// kart (@ref KartMove). Creates the @ref KartAction, @ref KartState, and @ref KartCollide
/// subsystems. Also calls @ref KartMove::createSubsystems() with the provided `stats`.
void KartSub::createSubsystems(bool isBike, const KartParam::Stats &stats) {
    m_move = isBike ? static_cast<KartMove *>(EGG::egg_new<KartMoveBike>()) :
                      EGG::egg_new<KartMove>();
    m_action = EGG::egg_new<KartAction>();
    m_move->createSubsystems(stats);
    m_state = EGG::egg_new<KartState>();
    m_collide = EGG::egg_new<KartCollide>();
}

/// @addr{0x80596454}
/// @brief Called during static construction of KartObject to synchronize the pointers
/// @param pointers The KartAccessor to synchronize the kart's subsystem pointers with
/// @details Copies the internal subsystem pointers from this @ref KartSub instance to the
/// provided @ref KartAccessor, ensuring that the accessor has up-to-date references to the
/// kart's subsystems.
void KartSub::copyPointers(KartAccessor &pointers) {
    pointers.collide = m_collide;
    pointers.state = m_state;
    pointers.move = m_move;
    pointers.action = m_action;
}

/// @addr{0x80595F78}
/// @brief Initializes the Kart subsystems and resets their physics states
/// @details Calls @ref resetPhysics() to initialize the kart's physics to their default state. Also
/// calls @ref KartBody::reset(), @ref KartState::init(), @ref KartMove::setTurnParams(), @ref
/// KartAction::init(), and @ref KartCollide::init().
void KartSub::init() {
    resetPhysics();
    body()->reset();
    m_state->init();
    m_move->setTurnParams();
    m_action->init();
    m_collide->init();
}

/// @addr{0x8059828C}
/// @brief Creates the kart's @ref Field::BoxColUnit so it can participate in collision detection
/// @param accessor The KartAccessor containing the boxColUnit to initialize
/// @param object The KartObject associated with this KartSub
/// @details Calls to @ref Field::BoxColManager::insertDriver() with a radius of `25.0f` times @ref
/// KartCollide:m_boundingRadius and a max speed of @ref KartCollide::m_hardSpeedLimit.
void KartSub::initAABB(KartAccessor &accessor, KartObject *object) {
    f32 radius = 25.0f + m_collide->boundingRadius();
    f32 hardSpeedLimit = m_move->hardSpeedLimit();

    accessor.boxColUnit = Field::BoxColManager::Instance()->insertDriver(radius, hardSpeedLimit,
            &pos(), true, object);
}

/// @addr{0x80597934}
/// @brief Initializes the kart's pose and hitbox positions
/// @details Dispatches to @ref KartPhysics::updatePose() and @ref KartCollide::setHitboxLastPos().
void KartSub::initPhysicsValues() {
    physics()->updatePose();
    m_collide->setHitboxLastPos();
}

/// @addr{0x8059617C}
/// @brief Resets the kart's physics state and suspension/tire physics to their default values
/// @details This is called during race initialization and in the black screen during respawns.
/// Calls @ref KartPhysics::reset() and @ref initPhysicsValues(). Calls @ref
/// KartSuspensionPhysics::reset() and @ref KartTirePhysics::reset() for all suspensions and tires.
/// Resets the kart's speed limit to `120.0f` via @ref KartMove::setKartSpeedLimit(). Resets the
/// object's @ref Field::BoxColUnit via @ref resizeAABB(). Finally, resets @ref m_suspScale to
/// `1.0f` and @ref m_sideCollisionTimer, @ref m_maxSuspOvertravel, and @ref m_minSuspOvertravel to
/// zero.
void KartSub::resetPhysics() {
    physics()->reset();
    initPhysicsValues();

    for (u16 wheelIdx = 0; wheelIdx < suspCount(); ++wheelIdx) {
        suspensionPhysics(wheelIdx)->reset();
    }
    for (u16 tireIdx = 0; tireIdx < tireCount(); ++tireIdx) {
        tirePhysics(tireIdx)->reset();
    }
    m_move->setKartSpeedLimit();

    resizeAABB(1.0f);

    m_sideCollisionTimer = 0;
    m_suspScale = 1.0f;
    m_maxSuspOvertravel.setZero();
    m_minSuspOvertravel.setZero();
}

/// @addr{0x80596480}
/// @brief The first phase of physics computations on each frame.
/// @details If the kart has entered a cannon this frame (indicated via the @ref
/// eStatus::CannonStart flag in the kart's @ref Status), resets the kart and tires' @ref
/// CollisionGroup and calls @ref KartMove::enterCannon(). Calls @ref KartState::calc() to reset the
/// kart's state for the new frame and process collision-related flags from the previous frame. If
/// the kart's position needs to be updated due to a respawn (indicated via @ref
/// eStatus::TriggerRespawn), resets the kart's inertia scale via @ref setInertiaScale() and calls
/// @ref resetPhysics(), @ref KartState::reset(), @ref KartMove::setTurnParams(), and @ref
/// KartMove::calcRespawnTrigger().
///
/// Updates @ref KartPhysics::m_pos and @ref KartPhysics::m_velocity to match @ref
/// KartDynamics::m_pos and @ref KartDynamics::m_velocity. Resets @ref KartDynamics::m_gravity to
/// @ref GRAVITY and @ref KartDynamics::m_angVel0YFactor to `0.9f`.
///
/// Processes inputs via @ref KartState::calcInput(). Updates the kart's movement physics via @ref
/// KartMove::calc(). Processes any active actions via @ref KartAction::calc() and any active pull
/// paths via @ref KartPullPath::calc().
///
/// If the @ref eStatus::SkipWheelCalc flag is set in the kart's @ref Status, then updates each
/// tire's @ref WheelPhysics::m_lastPos and early returns.
///
/// Evaluates soft wall and the "Horizontal Wall Glitch" timer via @ref calcSoftWall(). Sets @ref
/// KartDynamics::m_up to @ref KartMove::m_up.
///
/// If the countdown has not ended yet, enforces that @ref KartDynamics::m_intVel is zero. If the
/// vehicle is a kart, then the X and Z components of @ref KartDynamics::m_extVel are reset to zero.
/// If the vehicle is a bike, then @ref KartDynamics::m_extVel is instead set to the rejection of
/// the external velocity along @ref KartMove::m_smoothedUp.
///
/// Calls @ref KartPhysics::calc() to apply trick and correction rotation. Calls @ref
/// KartMove::calcRejectRoad() to handle road rejection collisions. Finally, if the kart is not in a
/// cannon (indicated via the @ref eStatus::InCannon flag), calls @ref KartCollide::calcHitboxes()
/// to update the hitbox positions for this frame and resets the @ref CollisionGroup::m_hitboxScale
/// to @ref KartMove::m_totalScale.
void KartSub::calcPass0() {
    auto &status = KartObjectProxy::status();

    if (status.onBit(eStatus::CannonStart)) {
        physics()->hitboxGroup()->reset();
        for (size_t i = 0; i < tireCount(); ++i) {
            tirePhysics(i)->hitboxGroup()->reset();
        }
        m_move->enterCannon();
    }

    m_state->calc();

    if (status.onBit(eStatus::TriggerRespawn)) {
        setInertiaScale(EGG::Vector3f::unit);
        resetPhysics();
        m_state->reset();
        m_move->setTurnParams();
        m_move->calcRespawnTrigger();
    }

    physics()->setPos(dynamics()->pos());
    physics()->setVelocity(dynamics()->velocity());
    dynamics()->setGravity(GRAVITY);
    dynamics()->setAngVel0YFactor(0.9f);

    m_state->calcInput();
    m_move->calc();
    m_action->calc();
    collide()->pullPath().calc();

    if (status.onBit(eStatus::SkipWheelCalc)) {
        for (size_t tireIdx = 0; tireIdx < tireCount(); ++tireIdx) {
            tirePhysics(tireIdx)->setLastPos(pos());
        }
        return;
    }

    calcSoftWall();

    dynamics()->setUp(m_move->up());

    // Pertains to startslides / leaning in stage 0 and 1
    const auto *raceManager = System::RaceManager::Instance();
    if (!raceManager->isStageReached(System::RaceManager::Stage::Race)) {
        dynamics()->setIntVel(EGG::Vector3f::zero);

        EGG::Vector3f killExtVel = extVel();
        if (isBike()) {
            killExtVel = killExtVel.rej(m_move->smoothedUp());
        } else {
            killExtVel.x = 0.0f;
            killExtVel.z = 0.0f;
        }

        dynamics()->setExtVel(killExtVel);
    }

    f32 maxSpeed = m_move->hardSpeedLimit();
    physics()->calc(maxSpeed, scale(), status.offBit(eStatus::TouchingGround));

    m_move->calcRejectRoad();

    if (status.offBit(eStatus::InCannon)) {
        collide()->calcHitboxes();
        collisionGroup()->setHitboxScale(m_move->totalScale());
    }
}

/// @addr{0x80596CFC}
/// @brief The second phase of physics computations on each frame.
/// @details Resets rejection-related @ref Status flags via @ref KartState::resetEjection(). Resets
/// @ref m_movingWaterCollisionCount, @ref m_movingObjCollisionCount, and @ref
/// m_floorCollisionCount to zero. Also sets @ref m_objVel, @ref m_maxSuspOvertravel, and @ref
/// m_minSuspOvertravel to zero.
///
/// Performs a GJK collision check via @ref BoxColUnit::search(). Processes detected collisions via
/// @ref KartCollide::calcObjectCollision(). Updates @ref KartDynamics::m_pos to reflect the
/// accumulated @ref KartCollide::m_tangentOff from collisions.
///
/// If the kart is being pushed out from a soft wall (indicated via @ref eStatus::SoftWallPush),
/// then offsets the kart's position with a speed that varies depending on whether the @ref
/// eStatus::HWG flag is set. Adds this speed to @ref KartCollide::m_movement as well.
///
/// If a side collision is detected (indicated via @ref m_sideCollisionTimer, @ref
/// CollisionData::bWallAtLeftCloser, or @ref CollisionData::bWallAtRightCloser), then applies an
/// XZ-plane velocity away from the wall, scaled based on @ref m_colPerpendicularity. This velocity
/// is applied to the kart's position and added to @ref KartCollide::m_movement. Finally, decrements
/// @ref m_sideCollisionTimer towards zero.
///
/// Calls @ref KartBody::calcSinkDepth() to reflect collisions with KCL having "wheel depth".
/// Narrows the spatial cache to KCL having #KCL_TYPE_VEHICLE_INTERACTABLE within `250f` units
/// around the kart's position via @ref Field::CollisionDirector::checkCourseColNarrScLocal().
///
/// Performs additional calls if the kart is not in a cannon (indicated via @ref eStatus::InCannon).
/// If the kart is not sticking to a zipper (@ref eStatus::ZipperStick), then performs a kart body
/// collision check via @ref KartCollide::findCollision(), calls @ref
/// KartBody::calcTargetSinkDepth() to update the kart's sink depth from any resulting body
/// collisions, and applies movement resulting from wall collisions via @ref
/// KartCollide::setMovement(). Otherwise, if the kart is sticking to a zipper, then resets the
/// kart's @ref CollisionData. Regardless, calls @ref KartCollide::calcFloorEffect() and @ref
/// KartCollide::calcFloorMomentScalar() to handle floor collisions. If a floor collision occurred,
/// calls @ref addFloor() to update @ref m_floorCollisionCount and conditionally set various flags
/// and moving road velocity.
///
/// Enforces @ref m_suspScale to be the maximum between the kart's veritcal @ref KartMove::m_scale
/// and the scale when shocked (@ref KartParam::Stats::shrinkScale). For each suspension, updates
/// the suspension's position and performs collision checks via @ref
/// KartSuspensionPhysics::calcCollision(), accumulating the lowest KCL speed and rotation factors
/// encountered.
///
/// Performs additional logic if @ref eStatus::SkipWheelCalc is not set in the kart's @ref Status.
/// Updates @ref KartDynamics::m_pos to handle suspension overtravel. If the kart's body did not
/// collide with the floor but any of the tires did, then saves the average of each tire's collision
/// relative position, velocity, and floor normal to the kart body's @ref CollisionData and calls
/// @ref KartCollide::calcRebound() to apply additional velocity and rotation to the kart.
/// Regardless of whether the kart's body collided with the floor, updates each suspension via @ref
/// KartSuspensionPhysics::calcSuspension(). Finally, calls @ref KartMove::calcHop() to alter
/// velocity and position resulting from a manual drift hop.
///
/// Sets the KCL wheel speed and rotation factors in @ref KartMove based on any tire collisions that
/// occurred. The accumulated factors are overridden if the kart is colliding with offroad KCL
/// (identified via @ref eStatus::CollidingOffroad).
///
/// Finally, Copies @ref m_m_floorCollisionCount to @ref KartMove::m_floorCollisionCount, processes
/// moving objects and moving road velocity and rotation via @ref calcMovingObj() and @ref
/// calcMovingRoad(), updates the kart's pose via @ref KartPhysics::updatePose(), and updates the
/// kart hitboxes' last frame position via @ref KartCollide::setHitboxLastPos().
void KartSub::calcPass1() {
    constexpr s16 SIDE_COLLISION_TIME = 5;

    m_state->resetEjection();

    m_movingWaterCollisionCount = 0;
    m_movingObjCollisionCount = 0;
    m_floorCollisionCount = 0;
    m_objVel.setZero();
    m_maxSuspOvertravel.setZero();
    m_minSuspOvertravel.setZero();

    // The flag is really 0x1f, but we only care about objects.
    Field::BoxColFlag flags;
    flags.setBit(Field::eBoxColFlag::Drivable, Field::eBoxColFlag::Object);
    boxColUnit()->search(flags);

    collide()->calcObjectCollision();
    dynamics()->setPos(pos() + collide()->tangentOff());

    auto &status = KartObjectProxy::status();

    if (status.onBit(eStatus::SoftWallPush)) {
        const EGG::Vector3f &softWallNrm = m_state->softWallNrm();
        f32 speedFactor = 5.0f;
        EGG::Vector3f effectiveSpeed;

        if (status.onBit(eStatus::HWG)) {
            speedFactor = 10.0f;
            effectiveSpeed = softWallNrm;
        } else {
            effectiveSpeed = softWallNrm.perpInPlane(m_move->smoothedUp(), true);
            f32 speedDotUp = softWallNrm.dot(m_move->smoothedUp());
            if (speedDotUp < 0.0f) {
                speedFactor += -speedDotUp * 10.0f;
            }
        }

        effectiveSpeed *= speedFactor * scale().y;
        setPos(pos() + effectiveSpeed);
        collide()->setMovement(collide()->movement() + effectiveSpeed);
    }

    auto &colData = collisionData();
    if (colData.bWallAtLeftCloser || colData.bWallAtRightCloser || m_sideCollisionTimer > 0) {
        EGG::Vector3f right = mainRot().rotateVector(EGG::Vector3f::ex);

        if (colData.bWallAtLeftCloser || colData.bWallAtRightCloser) {
            f32 sign = colData.bWallAtRightCloser ? 1.0f : -1.0f;
            m_colPerpendicularity = sign * colData.colPerpendicularity;
            m_sideCollisionTimer = SIDE_COLLISION_TIME;
        }

        EGG::Vector3f vel = 2.0f * m_colPerpendicularity * scale().x * right;
        vel.y = 0.0f;
        setPos(pos() + vel);
        collide()->setMovement(collide()->movement() + vel);
    }

    m_sideCollisionTimer = std::max(m_sideCollisionTimer - 1, 0);

    body()->calcSinkDepth();

    Field::CollisionDirector::Instance()->checkCourseColNarrScLocal(250.0f, pos(),
            KCL_TYPE_VEHICLE_INTERACTABLE, 0);

    if (status.offBit(eStatus::InCannon)) {
        if (status.offBit(eStatus::ZipperStick)) {
            collide()->findCollision();
            body()->calcTargetSinkDepth();

            if (colData.bWall || colData.bWall3) {
                collide()->setMovement(collide()->movement() + colData.movement);
            }
        } else {
            colData.reset();
        }

        collide()->calcFloorEffect();
        collide()->calcFloorMomentScalar();

        if (colData.bFloor) {
            // Update floor count
            addFloor(colData, false);
        }
    }

    EGG::Vector3f forward = fullRot().rotateVector(EGG::Vector3f::ez);
    const auto &stats = param()->stats();
    m_suspScale = std::max(scale().y, stats.shrinkScale);

    const EGG::Vector3f gravity(0.0f, GRAVITY, 0.0f);
    f32 speedFactor = 1.0f;
    f32 handlingFactor = 0.0f;
    for (u16 i = 0; i < suspCount(); ++i) {
        const EGG::Matrix34f wheelMatrix = body()->wheelMatrix(i);
        suspensionPhysics(i)->calcCollision(gravity, wheelMatrix);

        const CollisionData &colData = tirePhysics(i)->hitboxGroup()->collisionData();

        speedFactor = std::min(speedFactor, colData.speedFactor);

        if (colData.bFloor) {
            handlingFactor += colData.rotFactor;
            addFloor(colData, false);
        }
    }

    if (status.offBit(eStatus::SkipWheelCalc)) {
        EGG::Vector3f vehicleCompensation = m_maxSuspOvertravel + m_minSuspOvertravel;
        dynamics()->setPos(dynamics()->pos() + vehicleCompensation);

        if (!collisionData().bFloor) {
            EGG::Vector3f relPos = EGG::Vector3f::zero;
            EGG::Vector3f vel = EGG::Vector3f::zero;
            EGG::Vector3f floorNrm = EGG::Vector3f::zero;
            u32 count = 0;

            for (u16 wheelIdx = 0; wheelIdx < tireCount(); ++wheelIdx) {
                const WheelPhysics *wheelPhysics = tirePhysics(wheelIdx);
                if (wheelPhysics->hasSuspTravel() == 0.0f) {
                    continue;
                }

                const CollisionData &colData = wheelPhysics->hitboxGroup()->collisionData();
                relPos += colData.relPos;
                vel += colData.vel;
                floorNrm += colData.floorNrm;
                ++count;
            }

            if (count > 0) {
                f32 scalar = (1.0f / static_cast<f32>(count));
                floorNrm.normalise();

                KartCollide::SetFloorColInfo(collisionData(), relPos * scalar, vel * scalar,
                        floorNrm);

                collide()->calcRebound();
            }
        }

        // Optimization for Kinoko - KartCollide::applySomeFloorMoment() will re-compute the world
        // inertia tensor for the kart on each iteration of the suspension loop. Since this value
        // doesn't change within this loop, we compute it once and cache it.
        collide()->setWorldInertiaTensor(dynamics()->calcWorldInertiaTensor());

        for (u16 wheelIdx = 0; wheelIdx < suspCount(); ++wheelIdx) {
            suspensionPhysics(wheelIdx)->calcSuspension(forward, vehicleCompensation);
        }

        m_move->calcHop();
    }

    if (status.onBit(eStatus::CollidingOffroad)) {
        speedFactor = stats.kclSpeed[3];
        handlingFactor = stats.kclRot[3];
    }

    m_move->setKCLWheelSpeedFactor(speedFactor);
    m_move->setKCLWheelRotFactor(handlingFactor);

    m_move->setFloorCollisionCount(m_floorCollisionCount);

    calcMovingObj();
    calcMovingWater();

    physics()->updatePose();

    collide()->setHitboxLastPos();

    // calcRotation() is only ever used for gfx rendering, so skip
}

/// @addr{0x80598338}
/// @brief Resizes the kart's @ref Field::BoxColUnit to reflect its scaled radius and movement speed
/// @param radiusScale The scale factor to apply to the kart's collision radius
/// @details Calls @ref BoxColUnit::resize() with a max speed of @ref KartMove::m_hardSpeedLimit and
/// a radius of `250.0f` times the provided `radiusScale` times @ref
/// CollisionGroup::m_boundingRadius.
void KartSub::resizeAABB(f32 radiusScale) {
    f32 radius = radiusScale * collisionGroup()->boundingRadius();
    boxColUnit()->resize(radius + 25.0f, m_move->hardSpeedLimit());
}

/// @addr{0x805980D8}
/// @brief Called when a floor collision occurs
/// @param colData The collision data for the floor collision
/// @details Updates the @ref m_floorCollisionCount, @ref m_movingObjCollisionCount, and @ref
/// m_movingWaterCollisionCount based on the collision data. Accumulates road velocity from moving
/// objects. Updates the kart's @ref Status flags to reflect moving water collisions.
void KartSub::addFloor(const CollisionData &colData, bool) {
    ++m_floorCollisionCount;

    if (colData.bHasRoadVel) {
        ++m_movingObjCollisionCount;
        m_objVel += colData.roadVelocity;
    }

    auto &status = KartObjectProxy::status();
    if (colData.bMovingWaterMomentum || colData.bMovingWaterDecaySpeed) {
        ++m_movingWaterCollisionCount;

        status.changeBit(colData.bMovingWaterDecaySpeed, eStatus::MovingWaterDecaySpeed);
        status.changeBit(colData.bMovingWaterDisableAccel, eStatus::DisableAcceleration);
        status.changeBit(colData.bMovingWaterVertical, eStatus::MovingWaterVertical);
    } else {
        status.resetBit(eStatus::MovingWaterDecaySpeed, eStatus::DisableAcceleration,
                eStatus::MovingWaterVertical);
    }

    status.changeBit(colData.bMovingWaterStickyRoad, eStatus::MovingWaterStickyRoad);
}

/// @addr{0x805979EC}
/// @brief Updates the maximum and minimum suspension overtravel values based on the provided
/// suepension overtravel
/// @param suspOvertravel The suspension overtravel vector to update the max and min values with
void KartSub::updateSuspOvertravel(const EGG::Vector3f &suspOvertravel) {
    m_maxSuspOvertravel = m_maxSuspOvertravel.minimize(suspOvertravel);
    m_minSuspOvertravel = m_minSuspOvertravel.maximize(suspOvertravel);
}

/// @addr{0x80598744}
/// @brief Manages the soft wall and Horizontal Wall Glitch (HWG) status bits
/// @details This function checks the current status bits related to soft wall and HWG conditions
/// and updates them accordingly. It's also what's responsible for forcing the kart upright for the
/// vertical waterfall at the end of Koopa Cape.
void KartSub::calcSoftWall() {
    auto &status = KartObjectProxy::status();

    if (status.onBit(eStatus::SoftWallUnlockRotation)) {
        if (EGG::Mathf::abs(speed()) > 15.0f ||
                status.onBit(eStatus::AirtimeOver20, eStatus::AllWheelsCollision)) {
            status.resetBit(eStatus::SoftWallUnlockRotation);
        } else if (status.onBit(eStatus::TouchingGround)) {
            if (EGG::Mathf::abs(componentXAxis().dot(EGG::Vector3f::ey)) > 0.8f) {
                status.resetBit(eStatus::SoftWallUnlockRotation);
            }
        }
    }

    if (status.onBit(eStatus::HWG) && status.offBit(eStatus::SoftWallPush)) {
        if (status.offBit(eStatus::WallCollision, eStatus::Wall3Collision) ||
                status.onBit(eStatus::AllWheelsCollision)) {
            status.resetBit(eStatus::HWG);
        }
    }

    if (status.offBit(eStatus::InAction)) {
        dynamics()->setForceUpright(status.offBit(eStatus::SoftWallUnlockRotation));
    }
}

/// @addr{0x80597A88}
/// @brief Processes the moving object velocity accumulated this frame (or decays it if no
/// moving object collisions occurred)
/// @details If the kart is not colliding with any moving objects, then decays moving object
/// velocity based on whether the kart has any airtime (decays 30% if colliding with the floor,
/// decays 10% if more than `20` frames of airtime, or doesn't decay if fewer than `20` frames of
/// airtime). Otherwise, if no moving object collision has occurred, scales down @ref m_objVel based
/// on the number of tires colliding with the floor and interpolates @ref
/// KartPhysics::m_movingObjVel towards this value with a rate of `0.2f`.
/// @note This function is the reason why it is faster to hop at the end of a TF conveyer belt.
/// Rather than take the `else` branch, moving object velocity can be preserved for 20 frames (at
/// the cost of wheelie bonus speed).
void KartSub::calcMovingObj() {
    constexpr f32 FLOOR_DECAY_FACTOR = 0.7f;
    constexpr f32 SHORT_AIR_DECAY_FACTOR = 1.0f;
    constexpr f32 LONG_AIR_DECAY_FACTOR = 0.9f;
    constexpr f32 NO_MOVING_OBJ_INTERP_RATE = 0.2f;

    if (m_movingObjCollisionCount == 0) {
        f32 scalar = m_state->airtime() < 20 ? SHORT_AIR_DECAY_FACTOR : LONG_AIR_DECAY_FACTOR;
        physics()->composeDecayingMovingObjVel(FLOOR_DECAY_FACTOR, scalar,
                m_floorCollisionCount != 0);
    } else {
        m_objVel *= 1.0f / static_cast<f32>(m_floorCollisionCount);
        physics()->composeMovingObjVel(m_objVel, NO_MOVING_OBJ_INTERP_RATE);
    }
}

/// @addr{0x80597D4C}
/// @brief Processes road velocity pertaining to the moving water on Koopa Cape
/// @details If the kart is colliding with moving water (indicated via @ref
/// m_movingWaterCollisionCount), then composes the pull path's speed an direction via either @ref
/// KartPhysics::composeMovingRoadVel() or @ref KartPhysics::shiftDecayMovingRoadVel (depending on
/// whether the @ref eStatus::MovingWaterDecaySpeed flag is set in the kart's @ref Status).
/// Otherwise, if the kart is not colliding with moving water this frame, decays the water's
/// velocity via @ref KartPhysics::decayMovingRoadVel(). Finally, if the @ref
/// eStatus::MovingWaterStickyRoad flag is set (used when entering the pipe in Koopa Cape), applies
/// a `50.0f` scalar to the downward velocity of the pull path.
void KartSub::calcMovingWater() {
    constexpr f32 DECAY_SCALAR = 0.2f;
    constexpr f32 DECAY_FLOOR_SCALAR = 0.7f;
    constexpr f32 DECAY_AIR_SCALAR = 0.5f;
    constexpr f32 DECAY_KC_AIR_SCALAR = 0.3f;
    constexpr f32 STICKY_ROAD_SCALAR = 50.0f;

    auto &status = KartObjectProxy::status();
    const auto &pullPath = collide()->pullPath();

    if (m_movingWaterCollisionCount > 0) {
        f32 ratio = static_cast<f32>(m_movingWaterCollisionCount) /
                static_cast<f32>(m_floorCollisionCount);
        EGG::Vector3f dir = pullPath.pullDirection().perpInPlane(m_move->smoothedUp(), true);

        if (status.offBit(eStatus::MovingWaterDecaySpeed)) {
            EGG::Vector3f vel = pullPath.pullSpeed() * dir * ratio;
            physics()->composeMovingRoadVel(vel, DECAY_SCALAR);
        } else {
            EGG::Vector3f vel = pullPath.pullSpeed() * dir;
            physics()->shiftDecayMovingRoadVel(vel, pullPath.maxPullSpeed());
        }
    } else {
        f32 airScalar = status.onBit(eStatus::MovingWaterStickyRoad) ? DECAY_AIR_SCALAR : 1.0f;
        if (System::RaceConfig::Instance()->raceScenario().course == Course::Koopa_Cape &&
                status.onBit(eStatus::MovingWaterDecaySpeed)) {
            airScalar = DECAY_KC_AIR_SCALAR;
        }

        physics()->decayMovingRoadVel(DECAY_FLOOR_SCALAR, airScalar, m_floorCollisionCount > 0);
    }

    if (status.onBit(eStatus::MovingWaterStickyRoad)) {
        EGG::Vector3f dir = pullPath.pullDirection().perpInPlane(m_move->smoothedUp(), true);
        dir.y *= STICKY_ROAD_SCALAR;
        const EGG::Vector3f &vel = dynamics()->movingRoadVel();
        dynamics()->setMovingRoadVel(EGG::Vector3f(vel.x, dir.y, vel.z));
    }
}

/// @brief Gets the suspension scale for the kart
/// @return The current suspension scale value, @ref m_suspScale
f32 KartSub::suspScale() {
    return m_suspScale;
}

} // namespace Kinoko::Kart
