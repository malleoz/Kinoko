#include "KartSub.hh"

#include "game/kart/KartCollide.hh"
#include "game/kart/KartSuspensionPhysics.hh"

#include "game/field/CollisionDirector.hh"

#include "game/system/RaceConfig.hh"
#include "game/system/RaceManager.hh"

namespace Kinoko::Kart {

/// @brief Default constructor
KartSub::KartSub() = default;

/// @addr{0x80598AC8}
/// @brief Destructor that destroys all owning subsystems
KartSub::~KartSub() {
    EGG::egg_delete(m_collide);
    EGG::egg_delete(m_state);
    EGG::egg_delete(m_move);
    EGG::egg_delete(m_action);
}

/// @addr{0x80595D48}
/// @brief Creates the Kart subsystems based on if the vehicle is a bike and the provided stats
/// @param isBike Indicates if the vehicle is a bike
/// @param stats The stats to initialize the subsystems with
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
void KartSub::copyPointers(KartAccessor &pointers) {
    pointers.collide = m_collide;
    pointers.state = m_state;
    pointers.move = m_move;
    pointers.action = m_action;
}

/// @addr{0x80595F78}
/// @brief Initializes the Kart subsystems and resets their physics states
void KartSub::init() {
    resetPhysics();
    body()->reset();
    m_state->init();
    move()->setTurnParams();
    action()->init();
    m_collide->init();
}

/// @addr{0x8059828C}
/// @brief Creates the kart's @ref Field::BoxColUnit so it can participate in collision detection
/// @param accessor The KartAccessor containing the boxColUnit to initialize
/// @param object The KartObject associated with this KartSub
void KartSub::initAABB(KartAccessor &accessor, KartObject *object) {
    f32 radius = 25.0f + collide()->boundingRadius();
    f32 hardSpeedLimit = move()->hardSpeedLimit();

    accessor.boxColUnit = Field::BoxColManager::Instance()->insertDriver(radius, hardSpeedLimit,
            &pos(), true, object);
}

/// @addr{0x80597934}
/// @brief Initializes the kart's pose and hitbox positions
void KartSub::initPhysicsValues() {
    physics()->updatePose();
    collide()->setHitboxLastPos();
}

/// @addr{0x8059617C}
/// @brief Resets the kart's physics state and suspension/tire physics to their default values
/// @details This is called during race initialization and in the balck screen during respawns.
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
/// @details Handles the first-half of physics calculations. This includes input processing,
/// subsequent position/speed updates, as well as responding to last frame's collisions.
void KartSub::calcPass0() {
    auto &status = KartObjectProxy::status();

    if (status.onBit(eStatus::CannonStart)) {
        physics()->hitboxGroup()->reset();
        for (size_t i = 0; i < tireCount(); ++i) {
            tirePhysics(i)->hitboxGroup()->reset();
        }
        move()->enterCannon();
    }

    state()->calc();

    if (status.onBit(eStatus::TriggerRespawn)) {
        setInertiaScale(EGG::Vector3f(1.0f, 1.0f, 1.0f));
        resetPhysics();
        state()->reset();
        move()->setTurnParams();
        move()->calcRespawnTrigger();
    }

    physics()->setPos(dynamics()->pos());
    physics()->setVelocity(dynamics()->velocity());
    dynamics()->setGravity(-1.3f);
    dynamics()->setAngVel0YFactor(0.9f);

    state()->calcInput();
    move()->calc();
    action()->calc();
    collide()->pullPath().calc();

    if (status.onBit(eStatus::SkipWheelCalc)) {
        for (size_t tireIdx = 0; tireIdx < tireCount(); ++tireIdx) {
            tirePhysics(tireIdx)->setLastPos(pos());
        }
        return;
    }

    calcSoftWall();

    dynamics()->setUp(move()->up());

    // Pertains to startslides / leaning in stage 0 and 1
    const auto *raceManager = System::RaceManager::Instance();
    if (!raceManager->isStageReached(System::RaceManager::Stage::Race)) {
        dynamics()->setIntVel(EGG::Vector3f::zero);

        EGG::Vector3f killExtVel = extVel();
        if (isBike()) {
            killExtVel = killExtVel.rej(move()->smoothedUp());
        } else {
            killExtVel.x = 0.0f;
            killExtVel.z = 0.0f;
        }

        dynamics()->setExtVel(killExtVel);
    }

    f32 maxSpeed = move()->hardSpeedLimit();
    physics()->calc(DT, maxSpeed, scale(), status.offBit(eStatus::TouchingGround));

    move()->calcRejectRoad();

    if (status.offBit(eStatus::InCannon)) {
        collide()->calcHitboxes();
        collisionGroup()->setHitboxScale(move()->totalScale());
    }
}

/// @addr{0x80596CFC}
/// @brief The second phase of physics computations on each frame.
/// @details Handles the second-half of physics calculations. This mainly includes
/// collision detection, as well as suspension physics.
void KartSub::calcPass1() {
    constexpr s16 SIDE_COLLISION_TIME = 5;

    state()->resetEjection();

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
        const EGG::Vector3f &softWallNrm = state()->softWallNrm();
        f32 speedFactor = 5.0f;
        EGG::Vector3f effectiveSpeed;

        if (status.onBit(eStatus::HWG)) {
            speedFactor = 10.0f;
            effectiveSpeed = softWallNrm;
        } else {
            effectiveSpeed = softWallNrm.perpInPlane(move()->smoothedUp(), true);
            f32 speedDotUp = softWallNrm.dot(move()->smoothedUp());
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

        EGG::Vector3f colPerpBounceDir = 2.0f * m_colPerpendicularity * scale().x * right;
        colPerpBounceDir.y = 0.0f;
        setPos(pos() + colPerpBounceDir);
        collide()->setMovement(collide()->movement() + colPerpBounceDir);
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
    m_suspScale = std::max(scale().y, param()->stats().shrinkScale);

    const EGG::Vector3f gravity(0.0f, -1.3f, 0.0f);
    f32 speedFactor = 1.0f;
    f32 handlingFactor = 0.0f;
    for (u16 i = 0; i < suspCount(); ++i) {
        const EGG::Matrix34f wheelMatrix = body()->wheelMatrix(i);
        suspensionPhysics(i)->calcCollision(DT, gravity, wheelMatrix);

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

                collide()->setFloorColInfo(collisionData(), relPos * scalar, vel * scalar,
                        floorNrm);

                collide()->calcRebound();
            }
        }

        for (u16 wheelIdx = 0; wheelIdx < suspCount(); ++wheelIdx) {
            suspensionPhysics(wheelIdx)->calcSuspension(forward, vehicleCompensation);
        }

        move()->calcHop();
    }

    if (status.onBit(eStatus::CollidingOffroad)) {
        const auto &stats = param()->stats();
        speedFactor = stats.kclSpeed[3];
        handlingFactor = stats.kclRot[3];
    }

    move()->setKCLWheelSpeedFactor(speedFactor);
    move()->setKCLWheelRotFactor(handlingFactor);

    move()->setFloorCollisionCount(m_floorCollisionCount);

    calcMovingObj();
    calcMovingWater();

    physics()->updatePose();

    collide()->setHitboxLastPos();

    // calcRotation() is only ever used for gfx rendering, so skip
}

/// @addr{0x80598338}
/// @brief Resizes the kart's @ref Field::BoxColUnit to reflect its scaled radius and movement speed
/// @param radiusScale The scale factor to apply to the kart's collision radius
void KartSub::resizeAABB(f32 radiusScale) {
    f32 radius = radiusScale * collisionGroup()->boundingRadius();
    boxColUnit()->resize(radius + 25.0f, move()->hardSpeedLimit());
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
/// collisions occurred)
void KartSub::calcMovingObj() {
    if (m_movingObjCollisionCount == 0) {
        f32 scalar = state()->airtime() < 20 ? 1.0f : 0.9f;
        physics()->composeDecayingMovingObjVel(0.7f, scalar, m_floorCollisionCount != 0);
    } else {
        m_objVel *= 1.0f / static_cast<f32>(m_floorCollisionCount);
        physics()->composeMovingObjVel(m_objVel, 0.2f);
    }
}

/// @addr{0x80597D4C}
/// @brief Processes road velocity pertaining to the moving water on Koopa Cape
void KartSub::calcMovingWater() {
    constexpr f32 DECAY_FLOOR_SCALAR = 0.7f;
    constexpr f32 DECAY_AIR_SCALAR = 0.5f;
    constexpr f32 DECAY_KC_AIR_SCALAR = 0.3f;

    auto &status = KartObjectProxy::status();
    const auto &pullPath = collide()->pullPath();

    if (m_movingWaterCollisionCount > 0) {
        f32 ratio = static_cast<f32>(m_movingWaterCollisionCount) /
                static_cast<f32>(m_floorCollisionCount);
        EGG::Vector3f dir = pullPath.pullDirection().perpInPlane(move()->smoothedUp(), true);

        if (status.offBit(eStatus::MovingWaterDecaySpeed)) {
            EGG::Vector3f vel = pullPath.pullSpeed() * dir * ratio;
            physics()->composeMovingRoadVel(vel, 0.2f);
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
        EGG::Vector3f dir = pullPath.pullDirection().perpInPlane(move()->smoothedUp(), true);
        dir.y *= 50.0f;
        const EGG::Vector3f &vel = dynamics()->movingRoadVel();
        dynamics()->setMovingRoadVel(EGG::Vector3f(vel.x, dir.y, vel.z));
    }
}

} // namespace Kinoko::Kart
