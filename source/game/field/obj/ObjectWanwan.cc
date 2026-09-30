#include "ObjectWanwan.hh"

#include "game/field/CollisionDirector.hh"
#include "game/field/ObjectDirector.hh"

#include "game/system/RaceConfig.hh"
#include "game/system/RaceManager.hh"

namespace Kinoko::Field {

/// @addr{0x806E4224}
/// @copybrief ObjectCollidable::ObjectCollidable(const System::MapdataGeoObj &)
/// @param params The parameters used to initialize the object
/// @details Initializes @ref m_pitch to zero and @ref m_chainAttachMat to the identity matrix. Sets
/// @ref m_chainLength based on param setting 1, @ref m_attackDistance based on `4800.0f` plus param
/// setting 3, @ref m_attackDirectionX based on `10.0f` plus param setting 4, and @ref
/// m_attackDirectionZ based on `10.0f` plus param setting 5. Calculates @ref m_wanderDuration and
/// @ref m_attackArc based on param settings 6 and 7 respectively, with special handling if the
/// setting is zero.
///
/// Constructs the wooden stake object to which the Chain Chomp is attached. Computes the @ref
/// m_anchor point which is effectively the top of the @ref ObjectWanwanPile. Doubles the Chain
/// Chomp's scale. Computes the number of chains based off @ref m_chainLength. Sets a
/// course-specific wander constraint point. Computes the Chain Chomp's @ref m_initPos such that it
/// is half of @ref m_chainLength away from the @ref m_anchor point in both the X and Z direction,
/// with an upwards vertical displacement of `600.0f` units. Computes the wandering constraint point
/// based on the current course being played. Finally, calls @ref initTransformKeyFrames() to
/// initialize the chain attachment point's animation cycle.
ObjectWanwan::ObjectWanwan(const System::MapdataGeoObj &params)
    : ObjectCollidable(params),
      StateManager(this, STATE_ENTRIES),
      m_pitch(0.0f),
      m_chainLength(static_cast<f32>(params.setting(0))),
      m_attackDistance(4800.0f + static_cast<f32>(params.setting(2))),
      m_attackDirectionX(10.0f * static_cast<f32>(static_cast<s16>(params.setting(3)))),
      m_attackDirectionZ(10.0f * static_cast<f32>(static_cast<s16>(params.setting(4)))),
      m_wanderDuration(initWanderDuration(static_cast<f32>(params.setting(5)))),
      m_attackArc(initAttackArc(static_cast<f32>(params.setting(6)))),
      m_chainAttachMat(EGG::Matrix34f::ident) {
    constexpr EGG::Vector3f ANCHOR_OFFSET = EGG::Vector3f(0.0f, 20.0f, 0.0f);
    constexpr EGG::Vector3f POS_OFFSET_MCWII = EGG::Vector3f(14500.0f, 1300.0f, 44850.0f);
    constexpr EGG::Vector3f POS_OFFSET_RMC = EGG::Vector3f(8012.0f, 1668.0f, -30150.0f);

    auto *pile = EGG::egg_new<ObjectWanwanPile>(pos(), rot(), scale());
    pile->load();

    m_anchor = pos() + ANCHOR_OFFSET;

    setScale(SCALE);

    f32 sqLen = m_chainLength * m_chainLength + 300.0f * (300.0f * scale().y) * scale().y;
    m_chainCount = static_cast<u32>(EGG::Mathf::sqrt(sqLen) / (CHAIN_LENGTH * scale().y));
    if (m_chainCount > 0) {
        --m_chainCount;
    }

    m_initPos = m_anchor + EGG::Vector3f(0.5f * -m_chainLength, 600.0f, 0.5f * -m_chainLength);

    auto course = System::RaceConfig::Instance()->raceScenario().course;
    if (course == Course::Mario_Circuit) {
        EGG::Vector3f pos = POS_OFFSET_MCWII - m_anchor;
        pos.y = 0.0f;
        pos.normalise2();
        m_wanderConstraintPoint = pos * m_chainLength + m_anchor;
        m_wanderConstraintPoint.y = POS_OFFSET_MCWII.y;
    } else if (course == Course::GCN_Mario_Circuit) {
        EGG::Vector3f pos = POS_OFFSET_RMC - m_anchor;
        pos.y = 0.0f;
        pos.normalise2();
        m_wanderConstraintPoint = pos * m_chainLength + m_anchor;
        m_wanderConstraintPoint.y = POS_OFFSET_RMC.y;
    } else {
        m_wanderConstraintPoint = m_anchor;
        m_wanderConstraintPoint.z += m_chainLength;
    }

    initTransformKeyframes();
}

/// @addr{0x806E4B9C}
/// @copybrief ObjectBase::init()
/// @details Sets the Chain Chomp's position and @ref m_chainAttachPos to @ref m_initPos. Resets
/// @ref m_vel, @ref m_accel, and @ref m_speed to zero. Sets the orientation vectors (@ref
/// m_tangent, @ref m_up, @ref m_targetUp, @ref m_targetDir, and @ref m_backDir) to their initial
/// values. Resets @ref m_touchingFloor, @ref m_chainTaut, @ref m_retarget, and @ref m_attackStill
/// to false. Initializes @ref m_frame and @ref m_wanderTimer to zero. Finally, transitions the
/// Chain Chomp to the wait state.
void ObjectWanwan::init() {
    setPos(m_initPos);
    m_chainAttachPos = m_initPos;
    m_vel.setZero();
    m_accel.setZero();
    m_speed = 0.0f;
    m_tangent = EGG::Vector3f::ex;
    m_up = EGG::Vector3f::ey;
    m_targetUp = EGG::Vector3f::ey;
    m_touchingFloor = false;
    m_chainTaut = false;
    m_frame = 0;
    m_target.setZero();
    m_targetDir = EGG::Vector3f::ez;
    m_retarget = false;
    m_wanderTimer = 0;
    m_attackStill = false;
    m_backDir = EGG::Vector3f::ex;
    m_nextStateId = 0;
}

/// @addr{0x806E4F2C}
/// @copybrief ObjectBase::calc()
/// @details Evaluates the Chain Chomp's state machine. Updates the Chain Chomp's position, checks
/// for floor collision, refreshes @ref m_chainAttachMat, and updates the Chain Chomp's chain
/// attachment point. Increments @ref m_frame. If the Chain Chomp has fallen `1000.0f` units below
/// @ref m_anchor, then the Chain Chomp snaps upwards.
void ObjectWanwan::calc() {
    StateManager::calc();

    calcPos();
    calcAttackPos();
    calcCollision();
    calcMat();
    calcChainAttachMat();
    calcChain();

    ++m_frame;

    if (pos().y < m_anchor.y - 1000.0f) {
        setPos(EGG::Vector3f(pos().x, m_anchor.y + 1000.0f, pos().z));
        m_vel.y = 0.0f;
    }
}

/// @addr{0x806E526C}
/// @copybrief ObjectCollidable::onCollision()
/// @param kartObj The kart object involved in the collision
/// @param reactionOnKart The reaction that should be applied to the kart upon collision
/// @return @ref Kart::Reaction::Wall if the kart's speed is below 50% and the Chain Chomp is not
/// lurching forward, otherwise @ref Kart::Reaction::LaunchAwayFlipTwice.
/// @details Chain Chomp collisions are treated as wall collisions if the kart is moving at 50%
/// speed or less and if the Chain Chomp is not lurching forward.
Kart::Reaction ObjectWanwan::onCollision(Kart::KartObject *kartObj, Kart::Reaction reactionOnKart,
        Kart::Reaction /*reactionOnObj*/, EGG::Vector3f & /*hitDepth*/) {
    if (m_currentStateId == 1 && !m_attackStill) {
        return reactionOnKart;
    }

    return kartObj->speedRatioCapped() < 0.5f ? Kart::Reaction::Wall : reactionOnKart;
}

/// @addr{0x806E6208}
/// @brief Runs once when the Chain Chomp enters the wandering state
/// @details Resets @ref m_retarget to false. Resets the X and Z components of @ref m_vel and @ref
/// m_accel to zero. Determines a random angle within `[20, 40]` degrees for the Chain Chomp to
/// wander towards, biased towards the side away from the anchor. Finally, computes @ref m_target
/// based on the random angle, @ref m_chainLength, @ref m_anchor, and an additional scalar depending
/// on the length of the chain.
void ObjectWanwan::enterWait() {
    constexpr f32 ANGLE_RANGE = 0.33f * 60.0f;
    constexpr f32 ANGLE_NORMALIZATION = 0.66f * 60.0f;
    constexpr f32 LONG_CHAIN_LENGTH = 3000.0f;
    constexpr f32 SHORT_CHAIN_TARGET_SCALAR = 0.5f;
    constexpr f32 LONG_CHAIN_TARGET_SCALAR = 0.7f;

    m_retarget = false;
    m_vel.x = 0.0f;
    m_vel.z = 0.0f;
    m_accel.x = 0.0f;
    m_accel.z = 0.0f;
    m_speed = 0.0f;

    f32 randAngle =
            System::RaceManager::Instance()->random().getF32(ANGLE_RANGE) + ANGLE_NORMALIZATION;

    if (CrossXZ(pos() + m_tangent, pos(), m_anchor) >= 0.0f) {
        randAngle *= -1.0f;
    }

    EGG::Vector3f vStack_40 = m_anchor - EGG::Vector3f(pos().x, m_anchor.y, pos().z);
    vStack_40.normalise2();

    EGG::Vector3f vStack_4c = RotateXZByYaw(DEG2RAD * randAngle, vStack_40);
    f32 fVar2 = m_chainLength < LONG_CHAIN_LENGTH ? SHORT_CHAIN_TARGET_SCALAR :
                                                    LONG_CHAIN_TARGET_SCALAR;
    m_target = vStack_4c * m_chainLength * fVar2 + m_anchor;
}

/// @addr{0x806E6EB0}
/// @brief Runs once when the Chain Chomp enters the attacking state.
/// @details Resets the X and Z components of @ref m_vel and @ref m_accel to zero. Resets the Chain
/// Chomp's @ref m_speed, @ref m_pitch, and @ref m_wanderTimer to zero. Resets @ref m_attackStill
/// and @ref m_chainTaut to false. Raises the Y component of @ref m_anchor by `30.0f` units.
/// Finally, calculates a random target position for the Chain Chomp to lunge towards.
void ObjectWanwan::enterAttack() {
    m_vel.x = 0.0f;
    m_vel.z = 0.0f;
    m_accel.x = 0.0f;
    m_accel.z = 0.0f;
    m_speed = 0.0f;
    m_pitch = 0.0f;
    m_wanderTimer = 0;
    m_attackStill = false;
    m_chainTaut = false;
    m_anchor.y += 30.0f;

    calcRandomTarget();
}

/// @addr{0x806E73C4}
/// @brief Runs once when the Chain Chomp enters the retreating state.
/// @details Resets the X and Z components of @ref m_vel and @ref m_accel to zero. Sets @ref m_speed
/// to `15.0f`. Sets @ref m_pitch based on @ref ObjectDirector::WanwanMaxPitch(). Sets the X and Z
/// components of @ref m_backDir based on the difference between @ref m_anchor and the Chain Chomp's
/// position. Resets @ref m_attackStill and @ref m_chainTaut to `false`. Lowers the Y component of
/// @ref m_anchor by `30.0f`.
void ObjectWanwan::enterBack() {
    m_vel.x = 0.0f;
    m_vel.z = 0.0f;
    m_accel.x = 0.0f;
    m_accel.z = 0.0f;
    m_speed = 15.0f;
    m_pitch = -ObjectDirector::Instance()->WanwanMaxPitch();
    m_backDir = m_anchor - pos();
    m_backDir.y = 0.0f;
    m_backDir.normalise2();
    m_attackStill = false;
    m_chainTaut = false;
    m_anchor.y -= 30.0f;
}

/// @addr{0x806E63A4}
/// @brief Runs once per frame when the Chain Chomp is in the wandering state
/// @details Updates @ref m_targetDir based on the Chain Chomp's current position relative to @ref
/// m_target. Determines if the Chain Chomp needs to change its wandering target position if it has
/// wandered close enough to the previous target position. Interpolates @ref m_tangent and @ref m_up
/// towards their targets. Finally, updates the Chain Chomp's speed, checks to see if the Chain
/// Chomp is bouncing off the floor, and handles wandering behavior.
/// @bug If the Chain Chomp's position is exactly equal to the target position, then the local
/// variable `distFromTarget` will be uninitialized. To reflect base game behavior, we observe that
/// `r31` contains `F_PI`, thus we mimic that behavior by initializing `distFromTarget` to `F_PI`.
void ObjectWanwan::calcWait() {
    constexpr f32 RETARGET_DISTANCE_THRESHOLD = 0.55f;
    constexpr f32 ANGLE_RANGE = 0.33f * 0.5f * 60.0f;
    constexpr f32 ANGLE_NORMALIZATION = 0.66f * 0.5f * 60.0f;
    constexpr f32 TANGENT_INTERP_RATE = 0.04f;
    constexpr f32 UP_INTERP_RATE = 0.1f;

    EGG::Vector3f targetDir = m_target - pos();
    targetDir.y = 0.0f;

    // In the base game, if the dot product is less than epsilon, then r31 contains F_PI
    f32 distFromTarget = F_PI;
    if (targetDir.squaredLength() > std::numeric_limits<f32>::epsilon()) {
        distFromTarget = targetDir.normalise();
    }

    if (!m_retarget && distFromTarget < RETARGET_DISTANCE_THRESHOLD * m_chainLength) {
        EGG::Vector3f relTarget = m_target - m_anchor;
        auto &rand = System::RaceManager::Instance()->random();
        f32 angle = rand.getF32(ANGLE_RANGE) + ANGLE_NORMALIZATION;
        if (CrossXZ(m_target, m_wanderConstraintPoint, m_anchor) >= 0.0f) {
            angle *= -1.0f;
        }

        m_target = RotateXZByYaw(angle * DEG2RAD, relTarget) + m_anchor;
        m_retarget = true;
    }

    m_targetDir = targetDir;

    calcTangent(TANGENT_INTERP_RATE);
    calcUp(UP_INTERP_RATE);

    calcSpeed();
    calcBounce();

    calcWander();
}

/// @addr{0x806E6F6C}
/// @brief Runs once per frame when the Chain Chomp is in the attacking state.
/// @details If the Chain Chomp has been attacking for 2 seconds, then it transitions to the
/// retreating state. Updates the Chain Chomp's pitch so that it approaches the maximum pitch
/// defined by @ref ObjectDirector::WanwanMaxPitch(). Interpolates @ref m_tangent and @ref m_up
/// towards their targets. If the chain has become taut, then enforces the Chain Chomp's position is
/// clamped to the chain length. If the chain is taut or if the Chain Chomp is stationary in the
/// attack state, then sets @ref m_attackStill to `true`, sets the Y component of @ref m_vel to
/// zero, and flips and decays @ref m_vel by `0.85f`. Otherwise, if the chain is not taut and the
/// Chain Chomp is still lurching forward, the X and Z component of @ref m_vel are sets based on
/// @ref m_tangent with a speed of `120.0f`.
void ObjectWanwan::calcAttack() {
    constexpr u32 ATTACK_DURATION = 120;
    constexpr f32 PITCH_STEP = 25.0f;
    constexpr f32 TANGENT_INTERP_RATE = 10.0f * 0.04f;
    constexpr f32 UP_INTERP_RATE = 0.1f;
    constexpr f32 STILL_VEL_DECAY = -0.85f;
    constexpr f32 ATTACK_SPEED = 120.0f;

    if (m_currentFrame > ATTACK_DURATION) {
        m_nextStateId = 2;
    }

    f32 maxPitch = ObjectDirector::Instance()->WanwanMaxPitch();
    if (EGG::Mathf::abs(m_pitch) < EGG::Mathf::abs(maxPitch)) {
        m_pitch -= maxPitch / PITCH_STEP;
    }

    calcTangent(TANGENT_INTERP_RATE);
    calcUp(UP_INTERP_RATE);

    if (m_chainTaut || m_attackStill) {
        if (!m_attackStill) {
            m_target = m_anchor + (m_chainAttachPos - m_anchor) * SCALE;
            m_targetDir = m_target - pos();
            m_targetDir.y = 0.0f;
            m_targetDir.normalise2();
            EGG::Vector3f posOffset = pos() - m_chainAttachPos;
            posOffset.y = 0.0f;
            f32 radius = posOffset.length();
            EGG::Vector3f chainDir = m_chainAttachPos - m_anchor;
            chainDir.y = 0.0f;
            chainDir.normalise2();
            EGG::Vector3f nextPos = m_chainAttachPos + chainDir * radius;
            setPos(EGG::Vector3f(nextPos.x, pos().y, nextPos.z));
            EGG::Vector3f tangent = m_tangent + chainDir;
            tangent.y = 0.0f;
            if (tangent.squaredLength() > std::numeric_limits<f32>::epsilon()) {
                tangent.normalise2();
            }

            m_tangent = tangent;
        }

        m_attackStill = true;
        m_vel.y = 0.0f;
        m_vel *= STILL_VEL_DECAY;
    } else {
        m_vel.x = m_tangent.x * ATTACK_SPEED;
        m_vel.z = m_tangent.z * ATTACK_SPEED;
    }
}

/// @addr{0x806E7494}
/// @brief Runs once per frame when the Chain Chomp is in the retreating state
/// @details If the absolute value of the Chain Chomp's pitch is greater than `2.0f`, then
/// it interpolates the pitch back towards the maximum pitch defined by @ref
/// ObjectDirector::WanwanMaxPitch(). Updates the X and Z components of @ref m_vel in the direction
/// of @ref m_backDir with a speed of `1.5f` times @ref m_speed. Checks to see if the Chain Chomp is
/// bouncing off the floor. Finally, if the Chain Chomp has been retreating for more than 90 frames,
/// then it transitions to the wandering state.
void ObjectWanwan::calcBack() {
    constexpr f32 BACK_DURATION = 90;

    if (EGG::Mathf::abs(m_pitch) > 2.0f) {
        m_pitch += ObjectDirector::Instance()->WanwanMaxPitch() / 15.0f;
    }

    m_vel.x = m_backDir.x * m_speed * 1.5f;
    m_vel.z = m_backDir.z * m_speed * 1.5f;

    calcBounce();

    if (m_currentFrame > BACK_DURATION) {
        m_nextStateId = 0;
    }
}

/// @addr{0x806E5A8C}
/// @brief Performs a collision check to see if the Chain Chomp is touching the floor
/// @details If the Chain Chomp is touching the floor, then it bounces upwards with an acceleration
/// of @ref GRAVITY and @ref m_targetUp is set based on the colliding floor's normal vector.
void ObjectWanwan::calcCollision() {
    constexpr EGG::Vector3f POS_OFFSET = EGG::Vector3f(0.0f, -570.0f, 0.0f);

    m_touchingFloor = false;
    CollisionInfo info;
    KCLTypeMask mask;
    EGG::Vector3f colPos = pos() + POS_OFFSET;
    auto *colDir = CollisionDirector::Instance();

    bool hasCol = colDir->checkSphereFullPush(30.0f, colPos, EGG::Vector3f::inf, KCL_TYPE_FLOOR,
            &info, &mask, 0);

    if (!hasCol) {
        return;
    }

    m_touchingFloor = true;
    EGG::Vector3f tangentOff = info.tangentOff;
    f32 scale = tangentOff.normalise();
    addPos(EGG::Vector3f::ey * scale);

    if (info.floorDist > -std::numeric_limits<f32>::min()) {
        m_targetUp = info.floorNrm;
    }

    m_accel += GRAVITY;
}

/// @addr{0x806E5D70}
/// @brief Sets the transform of the Chain Chomp based off its current position, pitch, and tangent
/// @details If the Chain Chomp is in the attacking state, then its orientation is based off of the
/// world up vector and @ref m_tangent. Otherwise, its orientation is based off of the Chain Chomp's
/// current @ref m_up and @ref m_tangent. Updates the Chain Chomp's transformation matrix
/// accordingly.
void ObjectWanwan::calcMat() {
    EGG::Matrix34f mat;
    if (m_currentStateId == 1) {
        SetRotTangentHorizontal(mat, EGG::Vector3f::ey, m_tangent);
    } else {
        SetRotTangentHorizontal(mat, m_up, m_tangent);
    }
    mat.setBase(3, EGG::Vector3f::zero);

    EGG::Vector3f rot = EGG::Vector3f(m_pitch * DEG2RAD, 0.0f, 0.0f);
    EGG::Matrix34f rtMat;
    rtMat.makeRT(rot, EGG::Vector3f::zero);
    mat = mat.multiplyTo(rtMat);
    mat.setBase(3, pos());
    setTransform(mat);
}

/// @addr{0x806E5EDC}
/// @brief Calculates the world transform matrix of the chain attachment point on the Chain Chomp
/// @details The chain attachment point's world transform is calculated based on the Chain Chomp's
/// current transformation matrix and the keyframe position of the chain attachment point. If the
/// Chain Chomp is in the attacking state, then the first keyframe is used. Otherwise, the keyframe
/// is obtained by taking the total race framecount modulo `15`. The result is stored in @ref
/// m_chainAttachMat.
void ObjectWanwan::calcChainAttachMat() {
    u32 idx = m_currentStateId == 1 ? 0 : m_frame % 15;

    calcTransform();

    EGG::Matrix34f mat = transform();
    mat.setBase(3, EGG::Vector3f::zero);
    EGG::Vector3f keyFramePos = m_transformKeyframes[idx].base(3);
    EGG::Vector3f posOffset = mat.ps_multVector(keyFramePos) * SCALE;
    mat.setBase(3, posOffset + pos());

    m_chainAttachMat = mat;
}

/// @addr{0x806E78B0}
/// @brief Calculates the next speed of the Chain Chomp based off its current speed and tangent
/// @details If the Chain Chomp is at its max speed of `8.0f` units per frame, then @ref m_speed is
/// capped at this value, the X and Z components of @ref m_accel are reset to zero, and the X and Z
/// components of @ref m_vel are updated to reflect the capped @ref m_speed. If the Chain Chomp is
/// not at its max speed, then its @ref m_speed accelerates by `0.5f` and the X and Z components of
/// @ref m_accel are updated accordingly.
void ObjectWanwan::calcSpeed() {
    constexpr f32 MAX_SPEED = 8.0f;
    constexpr f32 ACCEL = 0.5f;

    if (m_speed < MAX_SPEED) {
        m_speed += ACCEL;
        m_accel.x = m_tangent.x * ACCEL;
        m_accel.z = m_tangent.z * ACCEL;
    } else {
        m_speed = MAX_SPEED;
        m_accel.x = 0.0f;
        m_accel.z = 0.0f;
        m_vel.x = m_tangent.x * MAX_SPEED;
        m_vel.z = m_tangent.z * MAX_SPEED;
    }
}

/// @addr{0x806E87C8}
/// @brief Calculates a random target position for the Chain Chomp to lunge towards
/// @details The target position is calculated by rotating the vector defined by @ref
/// m_attackDirectionX and @ref m_attackDirectionZ by a random angle in the range `[0, @ref
/// m_attackArc * 2.0f]`. Updates @ref m_targetDir to reflect the new @ref m_target.
void ObjectWanwan::calcRandomTarget() {
    f32 angle = System::RaceManager::Instance()->random().getF32(m_attackArc * 2.0f);
    EGG::Vector3f attackArcTarget = EGG::Vector3f(m_attackDirectionX, 0.0f, m_attackDirectionZ);
    EGG::Vector3f attackArcDir = attackArcTarget - m_anchor;
    attackArcDir.y = 0.0f;
    attackArcDir.normalise2();
    EGG::Vector3f attackTargetDir = RotateXZByYaw((angle - m_attackArc) * DEG2RAD, attackArcDir);
    m_target = m_anchor + attackTargetDir * m_attackDistance;
    m_targetDir = m_target - pos();
    m_targetDir.y = 0.0f;
    m_targetDir.normalise2();
}

/// @addr{0x806E8A60}
/// @brief Pre-computes a 15 frame animation cycle that describes how the Chain Chomp's chain
/// attachment point moves relative to its position
void ObjectWanwan::initTransformKeyframes() {
    std::array<f32, 15> xRot;
    SampleHermiteInterp(0.0f, 10.0f, 2.0f, -1.111111f, std::span(xRot.begin(), 6));
    SampleHermiteInterp(10.0f, -10.0f, -1.111111f, -1.111111f, std::span(xRot.begin() + 5, 5));
    SampleHermiteInterp(-10.0f, 0.0f, -1.111111f, 2.0f, std::span(xRot.begin() + 9, 6));

    std::array<f32, 15> yPos;
    SampleHermiteInterp(0.0f, -27.35f, -9.1166658f, 3.75f, std::span(yPos.begin(), 4));
    SampleHermiteInterp(-27.35f, 33.75f, 3.75f, 2.4863639f, std::span(yPos.begin() + 3, 7));
    SampleHermiteInterp(33.75f, 0.0f, 2.4863639f, -6.75f, std::span(yPos.begin() + 9, 6));

    std::array<f32, 15> zPos;
    SampleHermiteInterp(0.0f, -33.75f, -6.75f, -4.8214278f, std::span(zPos.begin(), 6));
    SampleHermiteInterp(-33.75f, -33.75f, -4.8214278f, 8.4375f, std::span(zPos.begin() + 5, 3));
    SampleHermiteInterp(-33.75f, 0.0f, 8.4375f, 14.464999f, std::span(zPos.begin() + 7, 3));
    SampleHermiteInterp(0.0f, 24.11f, 14.464999f, 0.0f, std::span(zPos.begin() + 9, 3));
    SampleHermiteInterp(24.11f, 0.0f, 0.0f, -8.0366669f, std::span(zPos.begin() + 11, 4));

    for (u8 i = 0; i < m_transformKeyframes.size(); ++i) {
        EGG::Matrix34f mat;
        mat.makeR(EGG::Vector3f(xRot[i] * DEG2RAD, 0.0f, 0.0f));
        mat.setBase(3, EGG::Vector3f(0.0f, yPos[i], zPos[i]));
        m_transformKeyframes[i] = mat;
    }
}

/// @addr{0x806E9084}
/// @brief Clamps the Chain Chomp's position to the chain's length while attacking
/// @details If the Chain Chomp is not in the attack state, this function early returns. Otherwise,
/// it updates the  Chain Chomp's transformation matrix based on its current @ref m_tangent and @ref
/// m_pitch and updates @ref m_chainAttachPos to reflect the Chain Chomp's current position and
/// orientation. Checks to see if the Chain Chomp's chain is taut and adjusts its position
/// accordingly (with an additional `35.0f` unit offset in the forward direction), setting @ref
/// m_chainTaut to `true` in the process.
void ObjectWanwan::calcAttackPos() {
    constexpr f32 SCALED_CHAIN_LENGTH = CHAIN_LENGTH * SCALE;
    constexpr f32 TAUT_POS_OFFSET = 35.0f;

    if (m_currentStateId != 1) {
        return;
    }

    calcMat();
    calcTransform();
    calcChainAttachPos(transform());

    EGG::Vector3f dir = m_chainAttachPos - m_anchor;
    f32 dist = dir.normalise();
    dist -= SCALED_CHAIN_LENGTH * static_cast<f32>(m_chainCount);

    if (dist > 0.0f || m_attackStill) {
        m_chainTaut = true;
        subPos(dir * dist);
        addPos(dir * TAUT_POS_OFFSET);
    }
}

} // namespace Kinoko::Field
