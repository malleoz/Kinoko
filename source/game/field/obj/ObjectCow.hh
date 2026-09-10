#pragma once

#include "game/field/StateManager.hh"
#include "game/field/obj/ObjectCollidable.hh"

#include "game/system/RaceManager.hh"

namespace Kinoko::Field {

class RailInterpolator;

/// @brief Represents a cow on Moo Moo Meadows
/// @details The base class shared between @ref ObjectCowLeader and @ref ObjectCowFollower
class ObjectCow : public ObjectCollidable {
public:
    /// @addr{0x806BBEC0}
    /// @brief Constructor
    /// @param params The parameters used to initialize the object
    /// @details Sets @ref m_startFrame based off param setting 3.
    ObjectCow(const System::MapdataGeoObj &params)
        : ObjectCollidable(params),
          m_startFrame(params.setting(2)) {}

    /// @addr{0x806BBF24}
    /// @brief Default virtual destructor
    ~ObjectCow() override = default;

    void init() override;

    Kart::Reaction onCollision(Kart::KartObject *kartObj, Kart::Reaction reactionOnKart,
            Kart::Reaction reactionOnObj, EGG::Vector3f &hitDepth) override;

protected:
    virtual void calcFloor();

    void calcPos();

    /// @addr{0x806BCDC4}
    /// @brief Sets the target position and direction for the cow
    /// @param v The target position
    /// @return The distance from the current position to the target position
    f32 setTarget(const EGG::Vector3f &v) {
        m_targetPos = v;
        EGG::Vector3f posDiff = m_targetPos - pos();
        f32 dist = posDiff.normalise();
        m_targetDir = m_targetPos + posDiff * 1000.0f - pos();
        m_targetDir.normalise2();

        return dist;
    }

    const u32 m_startFrame;      ///< The frame the cow will start moving
    EGG::Vector3f m_tangent;     ///< Smoothed forward direction, interpolated toward m_targetDir
    EGG::Vector3f m_prevTangent; ///< The previous frame's @ref m_tangent, used to smooth rotation
    EGG::Vector3f m_up;          ///< Smoothed up direction, interpolated toward the floor normal
    EGG::Vector3f m_velocity;    ///< The current velocity of the cow
    f32 m_xzSpeed;               ///< XZ plane length of m_velocity
    f32 m_tangentAccel; ///< Magnitude of acceleration applied in the direction of @ref m_tangent
    EGG::Vector3f m_floorNrm;  ///< The normal of the floor, used to orient the cow to the ground
    EGG::Vector3f m_targetPos; ///< Position the cow is walking towards
    EGG::Vector3f m_targetDir; ///< Direction from current position to @ref m_targetPos
    EGG::Vector3f m_upForce;   ///< Used by calcPos to counteract gravity
    f32 m_interpRate; ///< Rate at which @ref m_tangent is interpolated toward @ref m_targetDir

    /// @brief The force of gravity applied to the cow when it is not on the ground
    static constexpr EGG::Vector3f GRAVITY_FORCE = EGG::Vector3f(0.0f, 2.0f, 0.0f);
};

/// @brief A cow who its own rail and whose position is not influenced by the path of the others.
/// @details Walks from rail segmment waypoint to waypoint, stopping to eat grass along the way. The
/// time it spends eating grass is a random number between 120 and 240 frames.
class ObjectCowLeader final : public ObjectCow, private StateManager {
    /// @brief Grants the herd class access to the leader's state
    friend class ObjectCowHerd;

public:
    /// @addr{0x806BD080}
    /// @brief Constructor
    /// @param params The parameters used to initialize the object
    ObjectCowLeader(const System::MapdataGeoObj &params)
        : ObjectCow(params),
          StateManager(this, STATE_ENTRIES) {}

    /// @addr{0x806BD1F8}
    /// @brief Default virtual destructor
    ~ObjectCowLeader() override = default;

    void init() override;
    void calc() override;

    /// @addr{0x806BF42C}
    /// @copybrief ObjectBase::loadFlags()
    /// @return Returns @ref eLoadFlags::Calc, so that object is calculated every frame.
    [[nodiscard]] LoadFlags loadFlags() const override {
        return LoadFlags(eLoadFlags::Calc);
    }

private:
    /// @brief Represents the different states of the cow's eating animation
    enum class EatAnmType {
        EatST = 0, ///< The cow is starting to eat grass
        Eat = 1,   ///< The cow is currently eating grass
        EatED = 2, ///< The cow has finished eating grass
    };

    /// @addr{0x806BDCD8}
    /// @brief Calculates the cow's velocity, gravity, and floor normal.
    /// @details Clears the leader's upwards velocity. Applies gravity to the cow's @ref m_upForce.
    /// Finally, computes @ref m_floorNrm to reflect the next rail point's floor normal.
    void calcFloor() override {
        m_velocity.y = 0.0f;
        m_upForce = GRAVITY_FORCE;
        m_floorNrm = m_railInterpolator->floorNrm(m_railInterpolator->nextPointIdx());
    }

    /// @addr{0x806BD6B0}
    /// @brief Called when the cow enters the wait state.
    /// @details Sets the cow's target position based on the current rail interpolator's position
    /// and tangent direction.
    void enterWait() {
        setTarget(m_railInterpolator->curPos() + m_railInterpolator->curTangentDir() * 10.0f);
    }

    /// @addr{0x806BD7D8}
    /// @brief Called when the cow enters the eat state.
    /// @details Initializes the cow's eating animation and sets the duration by generating a random
    /// number in the range `[120, 240]`.
    void enterEat() {
        m_eatAnmType = EatAnmType::EatST;
        u32 rand = System::RaceManager::Instance()->random().getU32(120);
        m_eatFrames = rand + 120;
    }

    /// @addr{0x806BDA1C}
    /// @brief Called when the cow enters the roaming state.
    /// @details Resets the flag indicating whether the cow has reached the end of its current rail
    /// segment.
    void enterRoam() {
        m_endedRailSegment = false;
    }

    /// @addr{0x806BD738}
    /// @brief Called every frame while the cow is in the wait state.
    /// @details Transitions the cow to the eat state once the current frame exceeds the wait
    /// duration.
    void calcWait() {
        if (m_currentFrame > m_railInterpolator->curPoint().setting[0]) {
            m_nextStateId = 2;
        }
    }

    void calcEat();
    void calcRoam();

    f32 m_railSpeed;         ///< Current speed of the cow along its rail
    bool m_endedRailSegment; ///< Set when the cow has reached a waypoint on its rail
    EatAnmType m_eatAnmType; ///< The current state of the cow's eating animation
    u16 m_eatFrames;         ///< How long the cow stays in its EatAnmType::Eat state for

    /// @brief The enter and calc functions for each @ref StateManager entry
    static constexpr std::array<StateManagerEntry, 3> STATE_ENTRIES = {{
            {StateEntry<ObjectCowLeader, &ObjectCowLeader::enterWait, &ObjectCowLeader::calcWait>(
                    0)},
            {StateEntry<ObjectCowLeader, &ObjectCowLeader::enterEat, &ObjectCowLeader::calcEat>(1)},
            {StateEntry<ObjectCowLeader, &ObjectCowLeader::enterRoam, &ObjectCowLeader::calcRoam>(
                    2)},
    }};
};

/// @brief A cow that follows a leader by sharing the same rail.
class ObjectCowFollower final : public ObjectCow, private StateManager {
    /// @brief Grants the herd class access to the follower's state
    friend class ObjectCowHerd;

public:
    /// @addr{0x806BDD48}
    /// @brief Constructor
    /// @param params The parameters used to initialize the object
    /// @param pos The initial position offset for the follower cow
    /// @param initRot The initial rotation for the follower cow
    /// @details Applies the provided position offset to the cow's position and sets its initial yaw
    /// based on the given `initRot`.
    ObjectCowFollower(const System::MapdataGeoObj &params, const EGG::Vector3f &pos, f32 initRot)
        : ObjectCow(params),
          StateManager(this, STATE_ENTRIES),
          m_posOffset(pos),
          m_rail(nullptr) {
        addPos(m_posOffset);
        setRot(EGG::Vector3f(rot().x, initRot, rot().z));
    }

    /// @addr{0x806BDFF4}
    /// @brief Default virtual destructor
    ~ObjectCowFollower() override = default;

    void init() override;
    void calc() override;

    /// @addr{0x806BF424}
    /// @copybrief ObjectBase::loadFlags()
    /// @return Returns @ref eLoadFlags::Calc, so that object is calculated every frame.
    [[nodiscard]] LoadFlags loadFlags() const override {
        return LoadFlags(eLoadFlags::Calc);
    }

    /// @addr{0x806BF420}
    /// @copybrief ObjectBase::loadRail()
    /// @details no-op because the herd's rail is managed by @ref ObjectCowLeader.
    void loadRail() override {}

private:
    /// @addr{0x806BE4E8}
    /// @brief Called when the cow enters the wait state.
    /// @details Sets the cow's wait duration and rail segment threshold using random values.
    void enterWait() {
        constexpr u32 BASE_WAIT_FRAMES = 100;
        constexpr u32 WAIT_FRAMES_VARIANCE = 60;
        constexpr f32 BASE_RAIL_THRESHOLD = 0.2f;
        constexpr f32 RAIL_THRESHOLD_VARIANCE = 0.8f;

        auto &rand = System::RaceManager::Instance()->random();
        m_waitFrames = rand.getU32(WAIT_FRAMES_VARIANCE) + BASE_WAIT_FRAMES;
        m_railSegThreshold = BASE_RAIL_THRESHOLD + rand.getF32(RAIL_THRESHOLD_VARIANCE);
    }

    void enterFreeRoam();

    /// @addr{0x806BE930}
    /// @brief Called when the cow enters the follow leader state.
    /// @details Resets the stopping flag, sets the interpolation rate to `0.01f`, and determines
    /// the cow's top speed using a random value in the range `[2.0f, 4.0f]`.
    void enterFollowLeader() {
        m_bStopping = false;
        m_interpRate = 0.01f;

        auto &rand = System::RaceManager::Instance()->random();
        m_topSpeed = BASE_TOP_SPEED + rand.getF32(TOP_SPEED_VARIANCE);
    }

    /// @addr{0x806BE580}
    /// @brief Called every frame while the cow is in the wait state.
    /// @details Transitions the cow to the free roam state once the state duration is exceeded or
    /// the cow reaches @ref m_railSegThreshold.
    void calcWait() {
        if (m_currentFrame > m_waitFrames) {
            m_nextStateId = 1;
        }

        if (m_rail->segmentT() > m_railSegThreshold) {
            m_nextStateId = 2;
        }
    }

    void calcFreeRoam();
    void calcFollowLeader();

    const EGG::Vector3f m_posOffset; ///< Fixed offset from the leader's position
    RailInterpolator *m_rail;        ///< Pointer to the leader's rail interpolator
    u16 m_waitFrames;                ///< Number of frames the cow will stand still for
    f32 m_topSpeed;                  ///< The speed the cow will accelerate up to
    bool m_bStopping;                ///< Set when the cow is coming to a stop
    f32 m_railSegThreshold; ///< The rail segmentT at which a cow will change to the follow state

    static constexpr f32 BASE_TOP_SPEED = 2.0f;            ///< The minimum top speed for a cow
    static constexpr f32 TOP_SPEED_VARIANCE = 4.0f - 2.0f; ///< The variance of possible top speeds

    /// @brief Distance at which a cow is close enough to the leader's rail to stop moving
    static constexpr f32 DIST_THRESHOLD = 200.0f;

    /// @brief The enter and calc functions for each @ref StateManager entry
    static constexpr std::array<StateManagerEntry, 3> STATE_ENTRIES = {{
            {StateEntry<ObjectCowFollower, &ObjectCowFollower::enterWait,
                    &ObjectCowFollower::calcWait>(0)},
            {StateEntry<ObjectCowFollower, &ObjectCowFollower::enterFreeRoam,
                    &ObjectCowFollower::calcFreeRoam>(1)},
            {StateEntry<ObjectCowFollower, &ObjectCowFollower::enterFollowLeader,
                    &ObjectCowFollower::calcFollowLeader>(2)},
    }};
};

/// @brief The manager class that controls a group of cows.
/// @details The herd is led by the @ref ObjectCowLeader. @ref ObjectCowFollower cows share the same
/// rail as the leader cow but have fluctuations in their position. This class also handles
/// collision checks amongst the cows to make sure that they never walk through each other.
class ObjectCowHerd final : public ObjectCollidable {
public:
    ObjectCowHerd(const System::MapdataGeoObj &params);

    /// @addr{0x806BEFEC}
    /// @brief Default virtual destructor
    ~ObjectCowHerd() override = default;

    /// @addr{0x806BF02C}
    /// @copybrief ObjectBase::init()
    /// @details Assigns the leader's rail to each child.
    void init() override {
        for (auto *&child : m_followers) {
            child->m_rail = m_leader->m_railInterpolator;
        }
    }

    /// @addr{0x806BF064}
    /// @copybrief ObjectBase::calc()
    /// @details Checks for collisions between the cows in the herd. If a follower strays too far
    /// from the leader, it will transition to the follow state.
    void calc() override {
        constexpr f32 MAX_DIST = 4000.0f; // Distance at which a cow will return to its leader

        checkIntraCollision();

        for (auto *&follower : m_followers) {
            EGG::Vector3f posDelta = follower->pos() - m_leader->pos();

            if (posDelta.squaredLength() > MAX_DIST * MAX_DIST) {
                follower->m_nextStateId = 2;
            }
        }
    }

    /// @addr{0x806BF42C}
    /// @copybrief ObjectBase::loadFlags()
    /// @return Returns @ref eLoadFlags::Calc, so that object is calculated every frame.
    [[nodiscard]] LoadFlags loadFlags() const override {
        return LoadFlags(eLoadFlags::Calc);
    }

    /// @addr{0x806BF348}
    /// @copybrief ObjectBase::createCollision()
    /// @details no-op because the herd object itself does not have collision.
    void createCollision() override {}

    /// @addr{0x806BF34C}
    /// @copybrief ObjectBase::loadRail()
    /// @details no-op because the herd's rail is managed by @ref ObjectCowLeader.
    void loadRail() override {}

private:
    void checkIntraCollision();

    ObjectCowLeader *m_leader; ///< Pointer to the single leader that owns the rail
    owning_span<ObjectCowFollower *> m_followers; ///< All cows that follow the leader
};

} // namespace Kinoko::Field
