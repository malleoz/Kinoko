#include "KartCollide.hh"

#include "game/kart/KartBody.hh"

#include "game/field/CollisionDirector.hh"
#include "game/field/ObjectDirector.hh"

namespace Kinoko::Kart {

/// @addr{0x8056E56C}
KartCollide::KartCollide() {
    m_boundingRadius = 100.0f;
    m_surfaceFlags.makeAllZero();
}

/// @addr{0x80573FF0}
KartCollide::~KartCollide() = default;

/// @addr{0x8056E624}
/// @brief Initializes the collision state for the kart
void KartCollide::init() {
    m_pullPath.init();
    calcBoundingRadius();
    m_floorMomentScalar = 0.8f;
    m_surfaceFlags.makeAllZero();
    m_respawnTimer = 0;
    m_solidOobTimer = 0;
    m_shrinkTimer = 0;
    m_smoothedBack = 0.0f;
    m_sumFloorBottomHeight = 0.0f;
    m_sumSoftwallBottomHeight = 0.0f;
    m_numFloorOnlyCollisions = 0;
    m_numSoftWallCollisions = 0;
    m_poleAngVelTimer = 0;
    m_poleYaw = 0.0f;
    m_colPerpendicularity = 0.0f;
}

/// @addr{0x805730D4}
/// @brief Updates the last position for each hitbox in the kart's collision group
void KartCollide::setHitboxLastPos() {
    CollisionGroup *hitboxGroup = physics()->hitboxGroup();
    for (u16 idx = 0; idx < hitboxGroup->hitboxCount(); ++idx) {
        hitboxGroup->hitbox(idx).setLastPos(scale(), pose());
    }
}

/// @addr{0x8056EE24}
/// @brief On each frame, calculates the positions for each hitbox
void KartCollide::calcHitboxes() {
    CollisionGroup *hitboxGroup = physics()->hitboxGroup();
    for (u16 idx = 0; idx < hitboxGroup->hitboxCount(); ++idx) {
        hitboxGroup->hitbox(idx).calc(move()->totalScale(), body()->sinkDepth(), scale(), fullRot(),
                pos());
    }
}

/// @addr{0x80572C20}
/// @brief Handles collisions for the kart's body and responds accordingly
/// @details Evaluates kart body collisions and updates the kart's @ref CollisionData state
/// accordingly. Applies the necessary external velocity and rotation due to floor and wall
/// collisions.
void KartCollide::findCollision() {
    bool wasHalfPipe = status().onBit(eStatus::EndHalfPipe, eStatus::ActionMidZipper);
    const EGG::Quatf &rot = wasHalfPipe ? mainRot() : fullRot();
    calcBodyCollision(move()->totalScale(), body()->sinkDepth(), rot, scale());

    auto &colData = collisionData();
    bool existingWallCollision = colData.bWall || colData.bWall3;
    bool newWallCollision =
            m_surfaceFlags.onBit(eSurfaceFlags::ObjectWall, eSurfaceFlags::ObjectWall3);
    if (existingWallCollision || newWallCollision) {
        if (!existingWallCollision) {
            colData.wallNrm = m_totalReactionWallNrm;
            if (m_surfaceFlags.onBit(eSurfaceFlags::ObjectWall)) {
                colData.bWall = true;
            } else if (m_surfaceFlags.onBit(eSurfaceFlags::ObjectWall3)) {
                colData.bWall3 = true;
            }
        } else if (newWallCollision) {
            colData.wallNrm += m_totalReactionWallNrm;
            if (m_surfaceFlags.onBit(eSurfaceFlags::ObjectWall)) {
                colData.bWall = true;
            } else if (m_surfaceFlags.onBit(eSurfaceFlags::ObjectWall3)) {
                colData.bWall3 = true;
            }
        }

        colData.wallNrm.normalise();
    }

    calcRebound();
}

/// @addr{0x80572F4C}
/// @brief Calculates the rebound effect for collisions with floors and walls
/// @details While on a zipper or half-pipe or boosting, the kart will not rebound. Otherwise,
/// applies an additional rebound effect when colliding with walls. If the kart has been in the air
/// for more than 20 frames and is falling fast, then the relative X and Z position of the collision
/// will be reset to 0.
void KartCollide::calcRebound() {
    f32 fVar1;

    auto &status = KartObjectProxy::status();

    if (isInRespawn() ||
            status.onBit(eStatus::Boost, eStatus::OverZipper, eStatus::ZipperInvisibleWall,
                    eStatus::NoSparkInvisibleWall, eStatus::HalfPipeRamp)) {
        fVar1 = 0.0f;
    } else {
        fVar1 = 0.05f;
    }

    bool resetXZ = fVar1 > 0.0f && status.onBit(eStatus::AirtimeOver20) &&
            dynamics()->velocity().y < -50.0f;

    applyRebound(status.onBit(eStatus::InAction) ? 0.3f : 0.01f, fVar1, resetXZ,
            status.offBit(eStatus::JumpPadDisableYsusForce));
}

/// @addr{0x805B72B8}
/// @brief Affects velocity and rotation when colliding with floors and walls
/// @details Every frame, this function checks the player's position relative to the floor and wall
/// collision and applies external velocity and angular velocity.
/// @param reboundScalar The scalar to apply to the rebound effect
/// @param reboundVelAmt The velocity to add to the velocity from the kart's @ref CollisionData
/// @param lockXZ If true, the relative X and Z position of the collision will be set to 0
/// @param addExtVelY Gates whether the vertical component of rebound actually gets applied to the
/// kart's external velocity
void KartCollide::applyRebound(f32 reboundScalar, f32 reboundVelAmt, bool lockXZ, bool addExtVelY) {
    const auto &colData = collisionData();

    if (!colData.bFloor && !colData.bWall && !colData.bWall3) {
        return;
    }

    EGG::Vector3f colDir = colData.floorNrm + colData.wallNrm;
    colDir.normalise();

    f32 normalVel = colData.vel.dot(colDir);
    if (normalVel >= 0.0f) {
        return;
    }

    EGG::Matrix34f rtMat;
    rtMat.makeQ(dynamics()->mainRot());
    rtMat = rtMat.multiplyTo(dynamics()->invInertiaTensor()).multiplyTo(rtMat.transpose());

    EGG::Vector3f relPos = colData.relPos;
    if (lockXZ) {
        relPos.x = 0.0f;
        relPos.z = 0.0f;
    }

    EGG::Vector3f torque = rtMat.multVector33(relPos.cross(colDir));
    f32 normalImpulse =
            (-normalVel * (1.0f + reboundVelAmt)) / (1.0f + colDir.dot(torque.cross(relPos)));

    EGG::Vector3f tanDir = colDir.cross(-colData.vel).cross(colDir);
    tanDir.normalise();

    f32 maxTanImpulse = reboundScalar * EGG::Mathf::abs(normalImpulse);
    f32 tanImpulse = (normalImpulse * colData.vel.dot(tanDir)) / normalVel;

    f32 clampedTanImpulse = tanImpulse;
    if (maxTanImpulse < EGG::Mathf::abs(tanImpulse)) {
        clampedTanImpulse = maxTanImpulse;
        if (tanImpulse < 0.0f) {
            clampedTanImpulse = -reboundScalar * EGG::Mathf::abs(normalImpulse);
        }
    }

    EGG::Vector3f impulse = normalImpulse * colDir + clampedTanImpulse * tanDir;

    f32 impulseY = impulse.y;
    if (!addExtVelY) {
        impulseY = 0.0f;
    } else if (colData.bFloor) {
        f32 velY = intVel().y;
        if (velY > 0.0f) {
            velY += extVel().y;
            if (velY < 0.0f) {
                EGG::Vector3f newExtVel = extVel();
                newExtVel.y = velY;
                dynamics()->setExtVel(newExtVel);
            }
        }
    }

    f32 prevExtVelY = extVel().y;
    EGG::Vector3f extVelAdd = impulse;
    extVelAdd.y = impulseY;
    dynamics()->setExtVel(extVel() + extVelAdd);

    if (prevExtVelY < 0.0f && extVel().y > 0.0f && extVel().y < 10.0f) {
        EGG::Vector3f extVelNoY = extVel();
        extVelNoY.y = 0.0f;
        dynamics()->setExtVel(extVelNoY);
    }

    EGG::Vector3f worldAngVelDelta = rtMat.multVector33(relPos.cross(impulse));
    EGG::Vector3f localAngVelDelta = mainRot().rotateVectorInv(worldAngVelDelta);
    localAngVelDelta.y = 0.0f;
    dynamics()->setAngVel0(dynamics()->angVel0() + localAngVelDelta);
}

/// @addr{0x805B6724}
/// @brief Checks and acts on collision for each kart hitbox
/// @param totalScale Overall scale of the vehicle (only affected by shrinking)
/// @param sinkDepth The depth of the kart's body into the ground
/// @param rot The rotation of the kart's body
/// @param scale Per-axis scale of the kart's body
void KartCollide::calcBodyCollision(f32 totalScale, f32 sinkDepth, const EGG::Quatf &rot,
        const EGG::Vector3f &scale) {
    CollisionGroup *hitboxGroup = physics()->hitboxGroup();
    CollisionData &collisionData = hitboxGroup->collisionData();
    collisionData.reset();

    EGG::Vector3f posRel = EGG::Vector3f::zero;
    s32 count = 0;
    Field::CollisionInfo colInfo;
    colInfo.bbox.setDirect(EGG::Vector3f::zero, EGG::Vector3f::zero);
    Field::KCLTypeMask maskOut;
    Field::CourseColMgr::NoBounceWallColInfo noBounceWallInfo;
    EGG::BoundBox3f minMax;
    minMax.setZero();
    bool bVar1 = false;

    for (u16 hitboxIdx = 0; hitboxIdx < hitboxGroup->hitboxCount(); ++hitboxIdx) {
        Field::KCLTypeMask flags = KCL_TYPE_DRIVER_SOLID_SURFACE;
        Hitbox &hitbox = hitboxGroup->hitbox(hitboxIdx);

        if (hitbox.bspHitbox()->wallsOnly != 0) {
            flags = 0x4A109000;
            Field::CourseColMgr::Instance()->setNoBounceWallInfo(&noBounceWallInfo);
        }

        hitbox.calc(totalScale, sinkDepth, scale, rot, pos());

        if (Field::CollisionDirector::Instance()->checkSphereCachedFullPush(hitbox.radius(),
                    hitbox.worldPos(), hitbox.lastPos(), flags, &colInfo, &maskOut, 0)) {
            if (!!(maskOut & KCL_TYPE_VEHICLE_COLLIDEABLE)) {
                Field::CollisionDirector::Instance()->findClosestCollisionEntry(&maskOut,
                        KCL_TYPE_VEHICLE_COLLIDEABLE);
            }

            if (!accumulateBodyCollision(collisionData, hitbox, minMax, posRel, count, maskOut,
                        colInfo)) {
                bVar1 = true;

                if (colInfo.movingFloorDist > -std::numeric_limits<f32>::min()) {
                    collisionData.bHasRoadVel = true;
                    collisionData.roadVelocity = colInfo.roadVelocity;
                }

                processBody(collisionData, hitbox, &colInfo, &maskOut);
            }
        }
    }

    if (bVar1) {
        EGG::Vector3f movement = minMax.min + minMax.max;
        applyBodyCollision(collisionData, movement, posRel, count);
    } else {
        collisionData.speedFactor = 1.0f;
        collisionData.rotFactor = 1.0f;
    }
}

/// @addr{0x80571634}
/// @brief Calculates the effect the colliding floor has on the kart
void KartCollide::calcFloorEffect() {
    m_sumFloorBottomHeight = 0.0f;
    m_surfaceFlags.resetBit(eSurfaceFlags::Wall, eSurfaceFlags::SolidOOB, eSurfaceFlags::BoostRamp,
            eSurfaceFlags::Offroad, eSurfaceFlags::Trickable, eSurfaceFlags::NotTrickable,
            eSurfaceFlags::EndHalfPipe);
    m_sumSoftwallBottomHeight = 0.0f;
    m_numFloorOnlyCollisions = 0;
    m_numSoftWallCollisions = 0;

    Field::KCLTypeMask mask = KCL_NONE;
    calcLeanCollision(&mask, pos(), false);

    auto *colDir = Field::CollisionDirector::Instance();

    if (m_solidOobTimer >= 3 && m_surfaceFlags.onBit(eSurfaceFlags::SolidOOB) &&
            m_surfaceFlags.offBit(eSurfaceFlags::Wall)) {
        if (mask & KCL_TYPE_BIT(COL_TYPE_SOLID_OOB)) {
            colDir->findClosestCollisionEntry(&mask, KCL_TYPE_BIT(COL_TYPE_SOLID_OOB));
        }

        activateOob(true, &mask, false, false);
    }

    mask = KCL_NONE;
    calcLeanCollision(&mask, pos(), true);

    m_solidOobTimer =
            m_surfaceFlags.onBit(eSurfaceFlags::SolidOOB) ? std::min(3, m_solidOobTimer + 1) : 0;

    if (status().onBit(eStatus::Wall3Collision, eStatus::WallCollision)) {
        Field::KCLTypeMask maskOut = KCL_NONE;

        if (colDir->checkSphereCachedPartialPush(m_boundingRadius, pos(), EGG::Vector3f::inf,
                    KCL_TYPE_BIT(COL_TYPE_FALL_BOUNDARY), nullptr, &maskOut, 0)) {
            calcFallBoundary(&maskOut, true);
        }
    }
}

/// @addr{0x805718D4}
/// @brief Checks for collisions using a position behind and below the kart.
/// @param mask The collision type mask to store the result of the collision check
/// @param pos The position of the kart to check for collisions
/// @param checkDirection If true, the function will check for directional collisions including
/// triggers (such as fall plane OOB and cannon entries)
void KartCollide::calcLeanCollision(Field::KCLTypeMask *mask, const EGG::Vector3f &pos,
        bool checkDirection) {
    EGG::Vector3f v1 = checkDirection ? physics()->pos() : EGG::Vector3f::inf;
    Field::KCLTypeMask typeMask = checkDirection ? KCL_TYPE_DIRECTIONAL : KCL_TYPE_NON_DIRECTIONAL;
    f32 radius = checkDirection ? 80.0f : 100.0f * move()->totalScale();
    f32 scalar = -bsp().initialYPos * move()->totalScale() * 0.3f;
    EGG::Vector3f scaledPos = pos + scalar * componentYAxis();
    EGG::Vector3f back = dynamics()->mainRot().rotateVector(EGG::Vector3f::ez);

    m_smoothedBack += (back.dot(move()->smoothedUp()) - m_smoothedBack) * 0.3f;

    scalar = m_smoothedBack * -physics()->fc() * 1.8f * move()->totalScale();
    scaledPos += scalar * back;

    bool collide = Field::CollisionDirector::Instance()->checkSphereCachedPartialPush(radius,
            scaledPos, v1, typeMask, nullptr, mask, 0);

    if (!collide) {
        return;
    }

    if (checkDirection) {
        calcTriggers(mask);
    } else {
        if (*mask & KCL_TYPE_FLOOR) {
            Field::CollisionDirector::Instance()->findClosestCollisionEntry(mask, KCL_TYPE_FLOOR);
        }

        if (*mask & KCL_TYPE_WALL) {
            m_surfaceFlags.setBit(eSurfaceFlags::Wall);
        }

        if (*mask & KCL_TYPE_BIT(COL_TYPE_SOLID_OOB)) {
            m_surfaceFlags.setBit(eSurfaceFlags::SolidOOB);
        }
    }
}

/// @addr{0x8056F510}
/// @brief Checks for collisions with triggers and processes them accordingly
/// @param mask The collision type mask to store the result of the collision check
/// @details Checks for both fall boundary OOBs and cannon entries. If the kart is colliding with an
/// effect trigger, ends the half-pipe state.
void KartCollide::calcTriggers(Field::KCLTypeMask *mask) {
    calcFallBoundary(mask, false);
    processCannon(mask);

    if (*mask & KCL_TYPE_BIT(COL_TYPE_EFFECT_TRIGGER)) {
        auto *colDir = Field::CollisionDirector::Instance();
        if (colDir->findClosestCollisionEntry(mask, KCL_TYPE_BIT(COL_TYPE_EFFECT_TRIGGER))) {
            if (colDir->closestCollisionEntry()->variant() == 4) {
                halfPipe()->end(true);
                status().setBit(eStatus::EndHalfPipe);
                m_surfaceFlags.setBit(eSurfaceFlags::EndHalfPipe);
            }
        }
    }
}

/// @addr{0x80571D98}
/// @brief Checks for collisions with fall boundaries and activates OOB if necessary
/// @param mask The collision type mask to store the result of the collision check
/// @param shortBoundary If true, the function will only check for short fall boundaries
void KartCollide::calcFallBoundary(Field::KCLTypeMask *mask, bool shortBoundary) {
    if (!(*mask & KCL_TYPE_BIT(COL_TYPE_FALL_BOUNDARY))) {
        return;
    }

    auto *colDir = Field::CollisionDirector::Instance();
    if (!colDir->findClosestCollisionEntry(mask, KCL_TYPE_BIT(COL_TYPE_FALL_BOUNDARY))) {
        return;
    }

    if (!shortBoundary || colDir->closestCollisionEntry()->variant() == 7) {
        activateOob(false, mask, false, false);
    }
}

/// @addr{0x80573ED4}
/// @brief Checks if the kart is out of bounds and handles respawn and shrinking
/// @details If the kart's Y-position is negative, unconditionally triggers a respawn. If the kart
/// is in the "BeforeRespawn" state, decrements the respawn timer and respawns the player when it
/// reaches 0. Also decrements the shrink timer if it is greater than 0.
void KartCollide::calcBeforeRespawnAndShrink() {
    if (pos().y < 0.0f) {
        activateOob(true, nullptr, false, false);
    }

    auto &status = KartObjectProxy::status();

    if (status.onBit(eStatus::BeforeRespawn)) {
        if (--m_respawnTimer > 0) {
            return;
        }

        status.resetBit(eStatus::BeforeRespawn);
        m_respawnTimer = 0;
        move()->triggerRespawn();
    }

    m_shrinkTimer = std::max(0, m_shrinkTimer - 1);
}

/// @addr{0x80573B00}
/// @brief Activates the out-of-bounds state for the kart and sets the respawn timer
void KartCollide::activateOob(bool /*detachCamera*/, Field::KCLTypeMask * /*mask*/,
        bool /*somethingCPU*/, bool /*somethingBullet*/) {
    constexpr s16 RESPAWN_TIME = 130;

    auto &status = KartObjectProxy::status();

    if (status.onBit(eStatus::BeforeRespawn)) {
        return;
    }

    move()->initOob();

    m_respawnTimer = RESPAWN_TIME;
    status.setBit(eStatus::BeforeRespawn);
}

/// @addr{0x805B6F4C}
/// @brief Checks wheel hitbox collision and stores position/velocity info.
/// @param hitboxGroup The wheel's collision information
/// @param colVel The wheel's velocity. In the base game, it is always \f$\vec{v} = \begin{bmatrix}
/// 0 \\ -13 \\ 0 \end{bmatrix}\f$
/// @param center The wheel's position
/// @param radius The wheel's size
void KartCollide::calcWheelCollision(u16 /*wheelIdx*/, CollisionGroup *hitboxGroup,
        const EGG::Vector3f &colVel, const EGG::Vector3f &center, f32 radius) {
    Hitbox &firstHitbox = hitboxGroup->hitbox(0);
    BSP::Hitbox *bspHitbox = const_cast<BSP::Hitbox *>(firstHitbox.bspHitbox());
    bspHitbox->radius = radius;
    hitboxGroup->resetCollision();
    firstHitbox.setWorldPos(center);

    Field::CollisionInfo colInfo;
    colInfo.bbox.setZero();
    Field::KCLTypeMask kclOut;
    Field::CourseColMgr::NoBounceWallColInfo noBounceWallInfo;
    Field::CourseColMgr::Instance()->setNoBounceWallInfo(&noBounceWallInfo);

    bool collided = Field::CollisionDirector::Instance()->checkSphereCachedFullPush(
            firstHitbox.radius(), firstHitbox.worldPos(), firstHitbox.lastPos(),
            KCL_TYPE_VEHICLE_COLLIDEABLE, &colInfo, &kclOut, 0);

    CollisionData &collisionData = hitboxGroup->collisionData();

    if (!collided) {
        collisionData.speedFactor = 1.0f;
        collisionData.rotFactor = 1.0f;
        return;
    }

    collisionData.tangentOff = colInfo.tangentOff;

    if (noBounceWallInfo.dist > std::numeric_limits<f32>::min()) {
        collisionData.tangentOff += noBounceWallInfo.tangentOff;
        collisionData.noBounceWallNrm = noBounceWallInfo.fnrm;
        collisionData.bSoftWall = true;
    }

    if (kclOut & KCL_TYPE_FLOOR) {
        collisionData.bFloor = true;
        collisionData.floorNrm = colInfo.floorNrm;
    }

    collisionData.relPos = firstHitbox.worldPos() - pos();
    collisionData.vel = colVel;

    if (colInfo.movingFloorDist > -std::numeric_limits<f32>::min()) {
        collisionData.bHasRoadVel = true;
        collisionData.roadVelocity = colInfo.roadVelocity;
    }

    processWheel(collisionData, firstHitbox, &colInfo, &kclOut);

    if (!(kclOut & KCL_TYPE_VEHICLE_COLLIDEABLE)) {
        return;
    }

    Field::CollisionDirector::Instance()->findClosestCollisionEntry(&kclOut,
            KCL_TYPE_VEHICLE_COLLIDEABLE);
}

/// @addr{0x8056F26C}
/// @brief Checks for collisions on the left and right sides of the kart
void KartCollide::calcSideCollision(CollisionData &collisionData, Hitbox &hitbox,
        Field::CollisionInfo *colInfo) {
    if (colInfo->perpendicularity <= 0.0f) {
        return;
    }

    m_colPerpendicularity = std::max(m_colPerpendicularity, colInfo->perpendicularity);

    if (collisionData.bWallAtLeftCloser || collisionData.bWallAtRightCloser) {
        return;
    }

    f32 bspPosX = hitbox.bspHitbox()->position.x;
    if (EGG::Mathf::abs(bspPosX) > 10.0f) {
        if (bspPosX > 0.0f) {
            collisionData.bWallAtLeftCloser = true;
        } else {
            collisionData.bWallAtRightCloser = true;
        }

        collisionData.colPerpendicularity = colInfo->perpendicularity;

        return;
    }

    EGG::Vector3f right = dynamics()->mainRot().rotateVector(EGG::Vector3f::ex);
    std::array<f32, 2> tangents = {0.0f, 0.0f};

    // The loop is just to do left/right wall
    for (size_t i = 0; i < tangents.size(); ++i) {
        f32 sign = i == 1 ? -1.0f : 1.0f;
        f32 effectiveRadius = sign * hitbox.radius();
        EGG::Vector3f effectivePos = hitbox.worldPos() + effectiveRadius * right;
        Field::CollisionInfoPartial tempColInfo;

        if (Field::CollisionDirector::Instance()->checkSphereCachedPartial(hitbox.radius(),
                    effectivePos, hitbox.lastPos(), KCL_TYPE_DRIVER_WALL, &tempColInfo, nullptr,
                    0)) {
            tangents[i] = colInfo->tangentOff.squaredLength();
        }
    }

    if (tangents[0] > tangents[1]) {
        collisionData.bWallAtLeftCloser = true;
        collisionData.colPerpendicularity = colInfo->perpendicularity;
    } else if (tangents[1] > tangents[0]) {
        collisionData.bWallAtRightCloser = true;
        collisionData.colPerpendicularity = colInfo->perpendicularity;
    }
}

/// @addr{0x8056E70C}
/// @brief Calculates the bounding radius for the kart based off the collision group's bounding
/// radius and the kart's current hitbox scale.
void KartCollide::calcBoundingRadius() {
    m_boundingRadius = collisionGroup()->boundingRadius() * move()->hitboxScale();
}

/// @addr{0x80571F10}
/// @brief Checks for collisions with objects and applies the appropriate response
/// @details For each object collision, runs the appropriate collision handler function. Based off
/// the returned @ref Action type, either applies shrinking or induces movement away from the
/// object.
void KartCollide::calcObjectCollision() {
    constexpr s32 DUMMY_POLE_ANG_VEL_FRAMES = 3;
    constexpr f32 DUMMY_POLE_ANG_VEL = 0.005f;
    constexpr s32 SHRINK_DURATION = 60;

    m_totalReactionWallNrm = EGG::Vector3f::zero;
    m_surfaceFlags.resetBit(eSurfaceFlags::ObjectWall, eSurfaceFlags::ObjectWall3);

    auto *objColKart = objectCollisionKart();
    size_t collisionCount = objColKart->checkCollision(pose(), velocity());

    const auto *objectDirector = Field::ObjectDirector::Instance();

    for (size_t i = 0; i < collisionCount; ++i) {
        Reaction reaction = objectDirector->reaction(i);
        if (reaction != Reaction::None && reaction != Reaction::UNK_7) {
            size_t handlerIdx = static_cast<std::underlying_type_t<Reaction>>(reaction);
            Action newAction = (this->*s_objectCollisionHandlers[handlerIdx])(i);

            if (reaction == Reaction::SpinShrink && (m_shrinkTimer == 0)) {
                m_shrinkTimer = SHRINK_DURATION;
                move()->activateShrink();
                move()->applyForce(30.0f, objColKart->GetHitDirection(i), false);
            } else if (reaction != Reaction::SmallBump && reaction != Reaction::BigBump) {
                const EGG::Vector3f &hitDepth = objectDirector->hitDepth(i);
                m_tangentOff += hitDepth;
                m_movement += hitDepth;

                if (newAction != Action::None) {
                    action()->setHitDepth(objectDirector->hitDepth(i));
                    action()->start(newAction);
                }
            }
        }

        if (objectDirector->collidingObject(i)->id() == Field::ObjectId::DummyPole) {
            EGG::Vector3f hitDirection = objectCollisionKart()->GetHitDirection(i);
            const EGG::Vector3f &lastDir = move()->lastDir();

            if (lastDir.dot(hitDirection) < -COS_PI_OVER_4) {
                EGG::Vector3f angVel = hitDirection.cross(lastDir);
                f32 sign = angVel.y > 0.0f ? -1.0f : 1.0f;

                m_poleAngVelTimer = DUMMY_POLE_ANG_VEL_FRAMES;
                m_poleYaw = DUMMY_POLE_ANG_VEL * sign;
            }
        }
    }

    calcPoleTimer();
}

/// @addr{Inlined in 0x80571F10}
/// @brief Applies a small angular velocity to the kart when colliding with a dummy pole
void KartCollide::calcPoleTimer() {
    if (m_poleAngVelTimer > 0 && status().onBit(eStatus::Accelerate, eStatus::Brake)) {
        EGG::Vector3f angVel2 = dynamics()->angVel2();
        angVel2.y += m_poleYaw;
        dynamics()->setAngVel2(angVel2);
    }

    m_poleAngVelTimer = std::max(0, m_poleAngVelTimer - 1);
}

/// @addr{0x8056E764}
/// @brief Processes the collision for a single hitbox and updates the kart's collision data
void KartCollide::processBody(CollisionData &collisionData, Hitbox &hitbox,
        Field::CollisionInfo *colInfo, Field::KCLTypeMask *maskOut) {
    processMovingWater(collisionData, maskOut);

    bool hasWallCollision = processWall(collisionData, maskOut);

    processFloor(collisionData, hitbox, colInfo, maskOut, false);

    if (hasWallCollision) {
        calcSideCollision(collisionData, hitbox, colInfo);
    }

    processCannon(maskOut);
}

/// @addr{0x8056E930}
/// @brief Sets flags in the provided @ref CollisionData to reflect collisions with moving water KCL
void KartCollide::processMovingWater(CollisionData &collisionData, Field::KCLTypeMask *maskOut) {
    if (!(*maskOut & KCL_TYPE_BIT(COL_TYPE_MOVING_WATER))) {
        return;
    }

    auto *colDir = Field::CollisionDirector::Instance();
    if (!colDir->findClosestCollisionEntry(maskOut, KCL_TYPE_BIT(COL_TYPE_MOVING_WATER))) {
        return;
    }

    state()->status().setBit(eStatus::StickyRoad);

    auto *entry = colDir->closestCollisionEntry();
    switch (entry->variant()) {
    case 1:
        collisionData.bMovingWaterMomentum = true;
        collisionData.bMovingWaterStickyRoad = true;
        collisionData.bMovingWaterDisableAccel = true;
        break;
    case 2:
        collisionData.bMovingWaterDecaySpeed = true;
        break;
    case 3:
        collisionData.bMovingWaterDecaySpeed = true;
        collisionData.bMovingWaterDisableAccel = true;
        break;
    default:
        collisionData.bMovingWaterMomentum = true;
        break;
    }
}

/// @addr{0x8056F184}
/// @brief Checks for wall collisions and updates the kart's @ref CollisionData accordingly
bool KartCollide::processWall(CollisionData &collisionData, Field::KCLTypeMask *maskOut) {
    if (!(*maskOut & KCL_TYPE_DRIVER_WALL_NO_INVISIBLE_WALL2)) {
        return false;
    }

    auto *colDirector = Field::CollisionDirector::Instance();
    if (!colDirector->findClosestCollisionEntry(maskOut, KCL_TYPE_DRIVER_WALL_NO_INVISIBLE_WALL2)) {
        return false;
    }

    if ((*maskOut & KCL_TYPE_DRIVER_WALL_NO_INVISIBLE_WALL) &&
            colDirector->findClosestCollisionEntry(maskOut,
                    KCL_TYPE_DRIVER_WALL_NO_INVISIBLE_WALL)) {
        auto *entry = colDirector->closestCollisionEntry();

        collisionData.closestWallFlags = entry->baseType();
        collisionData.closestWallSettings = entry->variant();

        if (entry->attribute.onBit(Field::CollisionDirector::eCollisionAttribute::Soft)) {
            collisionData.bSoftWall = true;
        }
    }

    return true;
}

/// @addr{0x8056EA04}
/// @brief Processes the floor triangles' attributes.
/// @param collisionData Stores the resulting speed and handling info
/// @param maskOut Stores the flags from the floor KCL
/// @param wheel Differentiates between body and wheel floor collision (boost panels)
/// @details Updates the number of soft wall collisions occurring and the sum of the bottom heights
/// of the soft wall collisions. If the floor is trickable, sets the trickable flag in the collision
/// data. Updates the speed and rotation factors based on the floor's attributes. Checks whether to
/// set various floor-related status flags.
void KartCollide::processFloor(CollisionData &collisionData, Hitbox &hitbox,
        Field::CollisionInfo * /*colInfo*/, Field::KCLTypeMask *maskOut, bool wheel) {
    constexpr Field::KCLTypeMask BOOST_RAMP_MASK = KCL_TYPE_BIT(COL_TYPE_BOOST_RAMP);

    if (collisionData.bSoftWall) {
        ++m_numSoftWallCollisions;
        m_sumSoftwallBottomHeight += hitbox.worldPos().y - hitbox.radius();
    }

    if (!(*maskOut & KCL_TYPE_FLOOR)) {
        return;
    }

    auto *colDirector = Field::CollisionDirector::Instance();

    if (!colDirector->findClosestCollisionEntry(maskOut, KCL_TYPE_FLOOR)) {
        return;
    }

    const auto *closestColEntry = colDirector->closestCollisionEntry();

    if (closestColEntry->attribute.offBit(
                Field::CollisionDirector::eCollisionAttribute::Trickable)) {
        m_surfaceFlags.setBit(eSurfaceFlags::NotTrickable);
    } else {
        collisionData.bTrickable = true;
        m_surfaceFlags.setBit(eSurfaceFlags::Trickable);
    }

    collisionData.speedFactor = std::min(collisionData.speedFactor,
            param()->stats().kclSpeed[closestColEntry->baseType()]);

    collisionData.intensity = closestColEntry->intensity();
    collisionData.rotFactor += param()->stats().kclRot[closestColEntry->baseType()];

    auto &status = KartObjectProxy::status();

    if (closestColEntry->attribute.onBit(
                Field::CollisionDirector::eCollisionAttribute::RejectRoad)) {
        status.setBit(eStatus::RejectRoad);
    }

    collisionData.closestFloorFlags = closestColEntry->typeMask;
    collisionData.closestFloorSettings = closestColEntry->variant();

    if (wheel && !!(*maskOut & KCL_TYPE_BIT(COL_TYPE_BOOST_PAD))) {
        move()->padType().setBit(KartMove::ePadType::BoostPanel);
    }

    if (!!(*maskOut & BOOST_RAMP_MASK) &&
            colDirector->findClosestCollisionEntry(maskOut, BOOST_RAMP_MASK)) {
        closestColEntry = colDirector->closestCollisionEntry();
        move()->padType().setBit(KartMove::ePadType::BoostRamp);
        state()->setBoostRampType(closestColEntry->variant());
        m_surfaceFlags.setBit(eSurfaceFlags::BoostRamp, eSurfaceFlags::Trickable);
    } else {
        state()->setBoostRampType(-1);
        m_surfaceFlags.setBit(eSurfaceFlags::NotTrickable);
    }

    if (!collisionData.bSoftWall) {
        ++m_numFloorOnlyCollisions;
        m_sumFloorBottomHeight += hitbox.worldPos().y - hitbox.radius();
    }

    if (*maskOut & KCL_TYPE_BIT(COL_TYPE_STICKY_ROAD)) {
        status.setBit(eStatus::StickyRoad);
    }

    Field::KCLTypeMask halfPipeRampMask = KCL_TYPE_BIT(COL_TYPE_HALFPIPE_RAMP);
    if ((*maskOut & halfPipeRampMask) &&
            colDirector->findClosestCollisionEntry(maskOut, halfPipeRampMask)) {
        status.setBit(eStatus::HalfPipeRamp);
        state()->setHalfPipeInvisibilityTimer(2);
        if (colDirector->closestCollisionEntry()->variant() == 1) {
            move()->padType().setBit(KartMove::ePadType::BoostPanel);
        }
    }

    Field::KCLTypeMask jumpPadMask = KCL_TYPE_BIT(COL_TYPE_JUMP_PAD);
    if (*maskOut & jumpPadMask && colDirector->findClosestCollisionEntry(maskOut, jumpPadMask)) {
        if (status.offAnyBit(eStatus::TouchingGround, eStatus::JumpPad) &&
                status.offBit(eStatus::JumpPadMushroomVelYInc)) {
            move()->padType().setBit(KartMove::ePadType::JumpPad);
            closestColEntry = colDirector->closestCollisionEntry();
            state()->setJumpPadVariant(closestColEntry->variant());
        }
        collisionData.bTrickable = true;
    }
}

/// @addr{0x8056F490}
/// @brief Checks if we are colliding with a cannon trigger and sets the state flag if so.
void KartCollide::processCannon(Field::KCLTypeMask *maskOut) {
    auto *colDirector = Field::CollisionDirector::Instance();
    if (colDirector->findClosestCollisionEntry(maskOut, KCL_TYPE_BIT(COL_TYPE_CANNON_TRIGGER))) {
        state()->setCannonPointId(colDirector->closestCollisionEntry()->variant());
        status().setBit(eStatus::CannonStart);
    }
}

/// @brief Applies external and angular velocity based on the collision with the floor
/// @addr{0x805B7928}
/// @param down Always 0.1f
/// @param rate Downward velocity? Related to suspension stiffness
/// @param hitboxGroup Used to retrieve CollisionData reference
/// @param forward Current world facing direction of the kart
/// @param nextDir Updated facing direction of the kart
/// @param speed Tire speed
void KartCollide::applySomeFloorMoment(f32 down, f32 rate, CollisionGroup *hitboxGroup,
        const EGG::Vector3f &forward, const EGG::Vector3f &nextDir, const EGG::Vector3f &speed,
        bool b1, bool b2, bool b3) {
    CollisionData &colData = hitboxGroup->collisionData();
    if (!colData.bFloor) {
        return;
    }

    f32 velDotFloorNrm = colData.vel.dot(colData.floorNrm);

    if (velDotFloorNrm >= 0.0f) {
        return;
    }

    EGG::Matrix34f rotMat;
    rotMat.makeQ(dynamics()->mainRot());
    EGG::Matrix34f tmp = rotMat.multiplyTo(dynamics()->invInertiaTensor());
    EGG::Matrix34f rotMatTrans = rotMat.transpose();
    tmp = tmp.multiplyTo(rotMatTrans);

    EGG::Vector3f crossVec = colData.relPos.cross(colData.floorNrm);
    crossVec = tmp.multVector(crossVec);
    crossVec = crossVec.cross(colData.relPos);

    f32 scalar = -velDotFloorNrm / (1.0f + colData.floorNrm.dot(crossVec));
    EGG::Vector3f negSpeed = -speed;
    crossVec = colData.floorNrm.cross(negSpeed);
    crossVec = crossVec.cross(colData.floorNrm);

    if (std::numeric_limits<f32>::epsilon() >= crossVec.squaredLength()) {
        return;
    }

    crossVec.normalise();
    f32 speedDot = std::min(0.0f, speed.dot(crossVec));
    crossVec *= ((scalar * speedDot) / velDotFloorNrm);

    auto [proj, rej] = crossVec.projAndRej(forward);

    f32 projNorm = proj.length();
    f32 rejNorm = rej.length();
    f32 projNorm_ = projNorm;
    f32 rejNorm_ = rejNorm;

    f32 dVar7 = down * EGG::Mathf::abs(scalar);
    if (dVar7 < EGG::Mathf::abs(projNorm)) {
        projNorm_ = dVar7;
        if (projNorm < 0.0f) {
            projNorm_ = -down * EGG::Mathf::abs(scalar);
        }
    }

    f32 dVar5 = rate * EGG::Mathf::abs(scalar);
    if (EGG::Mathf::abs(rejNorm) > dVar5) {
        rejNorm_ = dVar5;
        if (rejNorm < 0.0f) {
            rejNorm_ = -rate * EGG::Mathf::abs(scalar);
        }
    }

    proj.normalise();
    rej.normalise();

    proj *= projNorm_;
    rej *= rejNorm_;

    EGG::Vector3f projRejSum = proj + rej;
    EGG::Vector3f projRejSumOrig = projRejSum;

    if (!b1) {
        projRejSum.x = 0.0f;
        projRejSum.z = 0.0f;
    }
    if (!b2) {
        projRejSum.y = 0.0f;
    }

    projRejSum = projRejSum.rej(nextDir);

    dynamics()->setExtVel(dynamics()->extVel() + projRejSum);

    if (b3) {
        EGG::Vector3f rotation = colData.relPos.cross(projRejSumOrig);
        EGG::Vector3f rotation2 = dynamics()->mainRot().rotateVectorInv(tmp.multVector(rotation));

        EGG::Vector3f angVel = rotation2;
        angVel.y = 0.0f;
        if (!b1) {
            angVel.x = 0.0f;
        }
        dynamics()->setAngVel0(dynamics()->angVel0() + angVel);
    }
}

/// @addr{0x805B6A9C}
/// @brief Called when the body of the kart has a collision.
/// @param collisionData The collision data to update with the results of the collision
/// @param hitbox The hitbox of the kart's body that collided
/// @param minMax The minimum and maximum points of the collision bounding box
/// @param relPos The relative position of the collision to the kart's position
/// @param count Counts the number of body hitbox collisions that have occurred this frame
/// @param mask A mask representing the type of collision that occurred
/// @param colInfo The collision information from the collision check
/// @return Returns true if the collision should be skipped, false otherwise
/// @details Updates the provided @ref CollisionData with the results of the collision. If not
/// colliding with a wall, updates the relative position and bounding box of the collision so that
/// subsequent collision checks for the body's hitboxes reflect this change in position. Finally
/// increments the number of hitbox collisions that have occurred.
bool KartCollide::accumulateBodyCollision(CollisionData &collisionData, const Hitbox &hitbox,
        EGG::BoundBox3f &minMax, EGG::Vector3f &relPos, s32 &count, const Field::KCLTypeMask &mask,
        const Field::CollisionInfo &colInfo) {
    if (mask & KCL_TYPE_WALL) {
        if (!(mask & KCL_TYPE_FLOOR) && status().onBit(eStatus::HWG) &&
                state()->softWallSpeed().dot(colInfo.wallNrm) < 0.3f) {
            return true;
        }

        bool skipWalls = false;

        collisionData.wallNrm += colInfo.wallNrm;

        if (mask & KCL_TYPE_ANY_INVISIBLE_WALL) {
            collisionData.bInvisibleWall = true;

            if (!(mask & KCL_TYPE_4010D000)) {
                collisionData.bInvisibleWallOnly = true;

                if (mask & KCL_TYPE_BIT(COL_TYPE_HALFPIPE_INVISIBLE_WALL)) {
                    skipWalls = true;
                }
            }
        }

        if (!skipWalls) {
            if (mask & KCL_TYPE_BIT(COL_TYPE_WALL_2)) {
                collisionData.bWall3 = true;
            } else {
                collisionData.bWall = true;
            }
        }
    }

    if (mask & KCL_TYPE_FLOOR) {
        collisionData.floorNrm += colInfo.floorNrm;
        collisionData.bFloor = true;
    }

    EGG::Vector3f tangentOff = colInfo.tangentOff;
    minMax.min = minMax.min.minimize(tangentOff);
    minMax.max = minMax.max.maximize(tangentOff);
    tangentOff.normalise();

    relPos += hitbox.relPos();
    relPos += -hitbox.radius() * tangentOff;
    ++count;

    return false;
}

/// @addr{0x805B6D48}
/// @brief Runs when the kart's body has a collision
/// @param collisionData The collision data to update with the results of the collision
/// @param movement The movement vector to apply to the kart's position
/// @param posRel The relative position of the collision to the kart's position
/// @param count The total number of body hitbox collisions that have occurred this frame
/// @details Adds @p movement to the kart's position and also saves it to the provided @ref
/// CollisionData if colliding with both a floor and a wall. Updates the @ref CollisionData's
/// relative position and velocity based on the average of all collisions and the kart's angular
/// velocity.
void KartCollide::applyBodyCollision(CollisionData &collisionData, const EGG::Vector3f &movement,
        const EGG::Vector3f &posRel, s32 count) {
    setPos(pos() + movement);

    if (!collisionData.bFloor && (collisionData.bWall || collisionData.bWall3)) {
        collisionData.movement = movement;
    }

    f32 avgFactor = 1.0f / static_cast<f32>(count);
    EGG::Vector3f avgRelPos = avgFactor * posRel;
    collisionData.rotFactor *= avgFactor;

    EGG::Vector3f scaledAngVel0 = dynamics()->angVel0Factor() * dynamics()->angVel0();
    EGG::Vector3f localRelPos = mainRot().rotateVectorInv(avgRelPos);
    EGG::Vector3f rotVel = scaledAngVel0.cross(localRelPos);
    rotVel = mainRot().rotateVector(rotVel);
    rotVel += extVel();

    collisionData.vel = rotVel;
    collisionData.relPos = avgRelPos;

    if (collisionData.bFloor) {
        f32 intVelY = dynamics()->intVel().y;
        if (intVelY > 0.0f) {
            collisionData.vel.y += intVelY;
        }
        collisionData.floorNrm.normalise();
    }
}

/// @addr{0x805713FC}
/// @brief Calculates the scalar used to affect the kart's floor alignment
/// @details If in a rotating action, the scalar is set to its minimum value of 0.01f. Otherwise, it
/// is incremented by 0.01f up to a maximum of 0.8f.
void KartCollide::calcFloorMomentScalar() {
    m_floorMomentScalar = status().onBit(eStatus::InAction) &&
                    action()->flags().onBit(KartAction::eFlags::Rotating) ?
            0.01f :
            std::min(m_floorMomentScalar + 0.01f, 0.8f);
}

/// @addr{0x80573A2C}
/// @brief Models an elastic collision by applying force to the kart away from the colliding object
Action KartCollide::handleReactRubberWall(size_t idx) {
    constexpr f32 BASE_DIR_FORCE_SCALAR = 0.95f;
    constexpr f32 DIR_FORCE_SCALAR = 0.050000012f;
    constexpr f32 MAX_FORCE = 70.0f;

    const EGG::Vector3f &hitDir = Field::ObjectCollisionKart::GetHitDirection(idx);
    const EGG::Vector3f &zAxis = componentZAxis();

    f32 force = BASE_DIR_FORCE_SCALAR + DIR_FORCE_SCALAR * EGG::Mathf::abs(hitDir.dot(zAxis));
    move()->applyForce(MAX_FORCE * force, hitDir, true);

    return Action::None;
}

} // namespace Kinoko::Kart
