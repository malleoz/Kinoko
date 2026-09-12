#include "ObjectCow.hh"

#include "game/field/CollisionDirector.hh"
#include "game/field/RailManager.hh"

#include "game/kart/KartCollide.hh"

namespace Kinoko::Field {

/// @addr{0x806BC2AC}
/// @copydoc ObjectCollidable::onCollision()
/// @param kartObj The kart object that collided with this object
/// @param reactionOnKart The reaction that should be applied to the kart upon collision
/// @param hitDepth The depth of the collision between the kart and the object
/// @return @ref Kart::Reaction::Wall if the kart's speed is below 50%, otherwise @ref
/// Kart::Reaction::LaunchAwayFlipOnce.
Kart::Reaction ObjectCow::onCollision(Kart::KartObject *kartObj, Kart::Reaction reactionOnKart,
        Kart::Reaction /*reactionOnObj*/, EGG::Vector3f & /*hitDepth*/) {
    return kartObj->speedRatioCapped() < 0.5f ? Kart::Reaction::Wall : reactionOnKart;
}

/// @addr{0x806BBF64}
/// @brief copybrief ObjectBase::init()
/// @details Initializes the cow's position, scale, and rotation based off the object parameters.
void ObjectCow::init() {
    ASSERT(m_mapObj);
    setScale(m_mapObj->scale());
    setPos(m_mapObj->pos());
    setRot(m_mapObj->rot() * DEG2RAD);
    m_tangent = EGG::Vector3f::ez;
    m_prevTangent = EGG::Vector3f::ez;
    m_up = EGG::Vector3f::ey;
    m_velocity = EGG::Vector3f::zero;
    m_xzSpeed = 0.0f;
    m_tangentAccel = 0.0f;
    m_floorNrm = EGG::Vector3f::ey;
    m_targetPos = pos();
    m_targetDir = EGG::Vector3f::ez;
    m_upForce = EGG::Vector3f::zero;
    m_interpRate = 0.05f;
}

/// @addr{0x806BC87C}
/// @brief Calculates the cow's interaction with floors and walls
/// @details If a collision is detected, the cow's position is adjusted, the floor normal is
/// updated, and gravity is applied. If no collision is detected, the cow's upward force is set to
/// zero.
void ObjectCow::calcFloor() {
    constexpr f32 RADIUS = 50.0f;
    constexpr EGG::Vector3f POS_OFFSET = EGG::Vector3f(0.0f, RADIUS, 0.0f);

    CollisionInfo info;

    bool hasCol = CollisionDirector::Instance()->checkSphereFull(RADIUS, pos() + POS_OFFSET,
            EGG::Vector3f::inf, KCL_TYPE_64EBDFFF, &info, nullptr, 0);

    if (hasCol) {
        addPos(info.tangentOff);

        if (info.floorDist > -std::numeric_limits<f32>::min()) {
            m_floorNrm = info.floorNrm;
        }

        m_velocity.y = 0.0f;
        m_upForce = GRAVITY_FORCE;
    } else {
        m_upForce = EGG::Vector3f::zero;
    }
}

/// @addr{0x806BC6D8}
/// @brief Calculates the cow's new position based on its velocity, acceleration, and forces
/// @details Acceleration is calculated based on the cow's tangent direction, change in tangent
/// direction, and upward force. The velocity is then updated accordingly, floored at zero. Finally,
/// the cow's position is adjusted based on the new velocity and @ref m_tangentAccel is cleared.
void ObjectCow::calcPos() {
    EGG::Vector3f accel =
            m_tangent * m_tangentAccel + (m_tangent - m_prevTangent) * m_xzSpeed + m_upForce;
    m_velocity += accel - GRAVITY_FORCE;
    m_xzSpeed = EGG::Mathf::sqrt(m_velocity.x * m_velocity.x + m_velocity.z * m_velocity.z);

    if (m_tangent.z * m_velocity.z + m_tangent.x * m_velocity.x < 0.0f) {
        m_velocity.x = 0.0f;
        m_velocity.z = 0.0f;
        m_xzSpeed = 0.0f;
    }

    addPos(m_velocity);
    m_tangentAccel = 0.0f;
}

/// @addr{0x806BD264}
/// @copybrief ObjectBase::init()
/// @details Initializes the cow leader's rail interpolator, position, target, rail speed, and state
/// variables.
void ObjectCowLeader::init() {
    ObjectCow::init();
    m_railInterpolator->init(0.0f, 0);
    setPos(m_railInterpolator->curPos());

    setTarget(pos() + m_railInterpolator->curTangentDir() * 10.0f);

    m_railInterpolator->setSpeed(static_cast<f32>(m_mapObj->setting(1)));

    m_railSpeed = 0.0f;
    m_endedRailSegment = false;
    m_eatAnmType = EatAnmType::EatST;
    m_eatFrames = 0;
    m_interpRate = 1.0f;
    m_nextStateId = 2;
}

/// @addr{0x806BD388}
/// @copybrief ObjectBase::calc()
/// @details If the current frame is greater than or equal to the cow's start frame, evaluate's the
/// leader's state machine. Updates the cow's position, velocity, gravity, and floor normal.
/// Interpolates the cow's tangent and up vectors.
void ObjectCowLeader::calc() {
    u32 t = System::RaceManager::Instance()->timer();

    if (t >= m_startFrame) {
        StateManager::calc();
    }

    calcPos();
    calcFloor();

    m_prevTangent = m_tangent;
    m_tangent = Interpolate(m_interpRate, m_tangent, m_targetDir);

    if (m_tangent.squaredLength() > std::numeric_limits<f32>::epsilon()) {
        m_tangent.normalise2();
    } else {
        m_tangent = EGG::Vector3f::ez;
    }

    m_up = Interpolate(0.1f, m_up, m_floorNrm);

    if (m_up.squaredLength() > std::numeric_limits<f32>::epsilon()) {
        m_up.normalise2();
    } else {
        m_up = EGG::Vector3f::ey;
    }

    setMatrixTangentTo(m_up, m_tangent);
}

/// @addr{0x806BD84C}
/// @brief Calculates the cow's behavior while in the eat state.
/// @details Manages the transitions between the different eating animation states and updates the
/// cow's state accordingly.
void ObjectCowLeader::calcEat() {
    constexpr u16 EAT_ST_FRAMES = 40;
    constexpr u16 EAT_ED_FRAMES = 60;

    switch (m_eatAnmType) {
    case EatAnmType::EatST: {
        if (m_currentFrame == EAT_ST_FRAMES) {
            m_eatAnmType = EatAnmType::Eat;
        }
    } break;
    case EatAnmType::Eat: {
        if (m_currentFrame > static_cast<u16>(m_eatFrames + EAT_ST_FRAMES)) {
            m_eatAnmType = EatAnmType::EatED;
        }
    } break;
    case EatAnmType::EatED: {
        if (static_cast<u16>(m_eatFrames + EAT_ST_FRAMES + EAT_ED_FRAMES) == m_currentFrame) {
            m_nextStateId = 2;
        }
    } break;
    default:
        break;
    }
}

/// @addr{0x806BDA70}
/// @brief Calculates the cow's behavior while in the roam state.
/// @details If the cow has reached the end of the rail segment, decreases the rail speed by `0.01f`
/// every frame until it comes to a stop. When it comes to a stop, it either transitions to the wait
/// state or the eat state based on the current rail point's settings. Otherwise, if the cow has not
/// reached the end of the rail segment, it increases the rail speed by `0.01f` every frame until
/// reaching the maximum speed of `4.0f`. Updates the rail to reflect the newly derived rail speed.
/// If the cow has reached the end of the segment and the current rail point has specific settings,
/// it sets @ref m_endedRailSegment so that cow will begin coming to a stop on the next frame.
/// Finally, updates the leader's current position and target based on the rail interpolator.
void ObjectCowLeader::calcRoam() {
    constexpr f32 RAIL_ACCEL = 0.1f;
    constexpr f32 RAIL_MAX_SPEED = 4.0f;

    if (m_endedRailSegment) {
        m_railSpeed -= RAIL_ACCEL;

        if (m_railSpeed < 0.0f) {
            m_railSpeed = 0.0f;

            if (m_railInterpolator->curPoint().setting[1] == 0) {
                m_nextStateId = 0;
            } else {
                m_nextStateId = 1;
            }
        }
    } else {
        if (m_railSpeed < RAIL_MAX_SPEED) {
            m_railSpeed += RAIL_ACCEL;
        } else {
            m_railSpeed = RAIL_MAX_SPEED;
        }
    }

    m_railInterpolator->setSpeed(m_railSpeed);

    auto status = m_railInterpolator->calc();
    const auto &curPoint = m_railInterpolator->curPoint();

    if (status == RailInterpolator::Status::SegmentEnd &&
            (curPoint.setting[0] != 0 || curPoint.setting[1] != 0)) {
        m_endedRailSegment = true;
    }

    setPos(m_railInterpolator->curPos() - EGG::Vector3f::ey * 10.0f);
    setTarget(m_railInterpolator->curPos() + m_railInterpolator->curTangentDir() * 10.0f);
}

/// @addr{0x806BE060}
/// @copybrief ObjectBase::init()
/// @details Initializes the cow follower by setting its initial position and orientation relative
/// to the leader's position. Initializes the cow to the waiting state.
void ObjectCowFollower::init() {
    ObjectCow::init();
    addPos(m_posOffset);
    EGG::Vector3f local_1c = m_posOffset;
    local_1c.normalise2();
    setMatrixTangentTo(EGG::Vector3f::ey, local_1c);
    m_nextStateId = 0;

    enterWait();

    m_waitFrames = 0;
    m_bStopping = false;
    m_railSegThreshold = 0.0f;
}

/// @addr{0x806BE1A8}
/// @copybrief ObjectBase::calc()
/// @details If the leader has not started moving yet, then the cow remains in the waiting state and
/// its target position is set to the opposite side of the leader. Otherwise, evaluates the cow's
/// state machine. In either case, the cow's position and orientation are updated accordingly.
void ObjectCowFollower::calc() {
    u32 t = System::RaceManager::Instance()->timer();

    if (t < m_startFrame) {
        calcWait();
        setTarget(pos() + m_posOffset * 2.0f);
    } else {
        StateManager::calc();
    }

    calcPos();
    calcFloor();

    m_prevTangent = m_tangent;
    m_tangent = Interpolate(m_interpRate, m_tangent, m_targetDir);

    if (m_tangent.squaredLength() > std::numeric_limits<f32>::epsilon()) {
        m_tangent.normalise2();
    } else {
        m_tangent = EGG::Vector3f::ez;
    }

    m_up = Interpolate(0.1f, m_up, m_floorNrm);

    if (m_up.squaredLength() > std::numeric_limits<f32>::epsilon()) {
        m_up.normalise2();
    } else {
        m_up = EGG::Vector3f::ey;
    }

    setMatrixTangentTo(m_up, m_tangent);
}

/// @addr{0x806BE62C}
/// @brief Runs when the cow enters the free roam state
/// @details The cow's top speed while roaming is a random number in the range `[2.0f, 4.0f]`. The
/// cow's facing direction is randomly varied in the range `[10.0f, 20.0f]` (in degrees) biased
/// towards the leader's position. The target position is computed accordingly, based off a distance
/// that is randomly chosen in the range `[400.0f, 700.0f]`.
void ObjectCowFollower::enterFreeRoam() {
    constexpr f32 BASE_WALK_DISTANCE = 400.0f;
    constexpr f32 WALK_DISTANCE_VARIANCE = 300.0f;
    constexpr f32 AVG_ANGLE = DEG2RAD360 * 10.0f;
    STATIC_ASSERT(AVG_ANGLE == 0.34906584f);

    m_bStopping = false;
    auto &rand = System::RaceManager::Instance()->random();

    m_topSpeed = BASE_TOP_SPEED + rand.getF32(TOP_SPEED_VARIANCE);

    f32 dVar2 = AVG_ANGLE + rand.getF32(AVG_ANGLE);

    // Adjust the cow's yaw so that it is biased towards the rail's position.
    // This means the cow will always walk in a sort of zig-zag generally following the rail.
    f32 fVar3 = CheckPointAgainstLineSegment(pos(), m_rail->curPoint().pos, m_rail->curPos());
    f32 angle = fVar3 > 0.0f ? -dVar2 : dVar2;

    EGG::Vector3f dir = RotateXZByYaw(angle, m_tangent);
    dir.y = 0.0f;
    dir.normalise2();

    f32 distance = BASE_WALK_DISTANCE + rand.getF32(WALK_DISTANCE_VARIANCE);
    setTarget(pos() + dir * distance);
}

/// @addr{0x806BE794}
/// @brief Calculates the cow's behavior while in the free roam state
/// @details The cow accelerates towards its target position until it reaches its top speed.
/// If it gets within @ref DIST_THRESHOLD of the target, it begins stopping. Otherwise, if the
/// leader has moved past @ref m_railSegThreshold, the follower transitions to the follow state.
void ObjectCowFollower::calcFreeRoam() {
    constexpr f32 ACCEL = 0.1f;

    if (m_bStopping) {
        m_tangentAccel = -ACCEL;

        if (m_xzSpeed == 0.0f) {
            m_nextStateId = 0;
        }
    } else {
        if (m_xzSpeed < m_topSpeed) {
            m_tangentAccel = ACCEL;
        }
    }

    EGG::Vector3f local_28 = m_targetPos - pos();
    if (local_28.x * local_28.x + local_28.z * local_28.z < DIST_THRESHOLD * DIST_THRESHOLD) {
        m_bStopping = true;
    }

    if (m_rail->segmentT() > m_railSegThreshold) {
        m_nextStateId = 2;
    }
}

/// @addr{0x806BE9CC}
/// @brief Calculates the cow's behavior while in the follow state
/// @details The cow accelerates towards the leader until it reaches its top speed.
/// If it gets within @ref DIST_THRESHOLD of the leader, it begins stopping.
void ObjectCowFollower::calcFollowLeader() {
    constexpr f32 ACCEL = 0.1f;
    constexpr f32 ACCEL_INTERP_RATE = 0.05f;

    f32 dist = 0.0f;

    if (m_bStopping) {
        m_tangentAccel = -ACCEL;

        if (m_xzSpeed == 0.0f) {
            m_interpRate = ACCEL_INTERP_RATE;
            m_nextStateId = 0;
        }
    } else {
        dist = setTarget(m_rail->curPos() + m_posOffset);

        if (m_xzSpeed < m_topSpeed) {
            m_interpRate = ACCEL_INTERP_RATE;
            m_tangentAccel = ACCEL;
        }
    }

    if (dist < DIST_THRESHOLD) {
        m_bStopping = true;
    }
}

/// @addr{0x806BEB54}
/// @brief Constructor
/// @param params The parameters used to initialize the object
/// @details Loads the leader and follower cows. Computes each follower's initial position and
/// rotation around the leader. The cows initially form a circular formation around the leader, such
/// that each cow is equally spaced apart and is facing away from the leader. Finally, pre-computes
/// all floor normals along the rail so that they can be fetched without having to perform repeat
/// collision checks.
ObjectCowHerd::ObjectCowHerd(const System::MapdataGeoObj &params) : ObjectCollidable(params) {
    constexpr f32 FOLLOWER_SPACING = 600.0f;

    m_leader = EGG::egg_new<ObjectCowLeader>(params);
    m_leader->load();

    u8 followerCount = params.setting(0);
    m_followers = owning_span<ObjectCowFollower *>(followerCount);

    for (u32 i = 0; i < followerCount; ++i) {
        auto *&child = m_followers[i];
        f32 rot = F_TAU / static_cast<f32>(followerCount) * static_cast<f32>(i);
        f32 z = EGG::Mathf::SinFIdx(RAD2FIDX * rot);
        f32 x = EGG::Mathf::CosFIdx(RAD2FIDX * rot);
        EGG::Vector3f pos = EGG::Vector3f(x, 0.0f, z) * FOLLOWER_SPACING;

        child = EGG::egg_new<ObjectCowFollower>(params, pos, rot);
        child->load();
    }

    auto *rail = RailManager::Instance()->rail(static_cast<size_t>(params.pathId()));
    rail->checkSphereFull();
}

/// @addr{0x806BF114}
/// @brief Prevents cows from walking into each other.
/// @details Checks for collisions between all follower cows and between each follower and the
/// leader cow. Cows are determined to be colliding if the distance between them is less than
/// `400.0f` units. If a collision is detected, the cows are moved apart. This function performs two
/// passes of collision checks: one for follower-to-follower collisions and another for
/// follower-to-leader collisions.
void ObjectCowHerd::checkIntraCollision() {
    constexpr f32 WIDTH = 400.0f;

    for (u32 i = 0; i < m_followers.size() - 1; ++i) {
        auto *iFollower = m_followers[i];

        for (u32 j = i + 1; j < m_followers.size(); ++j) {
            auto *jFollower = m_followers[j];

            EGG::Vector3f posDelta = jFollower->pos() - iFollower->pos();
            f32 length = posDelta.normalise();

            if (length < WIDTH) {
                EGG::Vector3f change = posDelta * (WIDTH - length) * 1.5f;
                jFollower->addPos(change);
                iFollower->subPos(change);
            }
        }
    }

    for (auto *&follower : m_followers) {
        EGG::Vector3f posDelta = m_leader->pos() - follower->pos();
        f32 length = posDelta.normalise();

        if (length < WIDTH) {
            EGG::Vector3f change = posDelta * (WIDTH - length) * 1.5f;

            follower->subPos(change);
        }
    }
}

} // namespace Kinoko::Field
