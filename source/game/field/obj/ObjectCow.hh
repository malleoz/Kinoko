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
    ObjectCow(const System::MapdataGeoObj &params);
    ~ObjectCow() override;

    Kart::Reaction onCollision(Kart::KartObject *kartObj, Kart::Reaction reactionOnKart,
            Kart::Reaction reactionOnObj, EGG::Vector3f &hitDepth) override;

protected:
    virtual void calcFloor();

    void setup();
    void calcPos();
    f32 setTarget(const EGG::Vector3f &v);

    u32 m_startFrame;            ///< The frame the cow will start moving
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
    friend class ObjectCowHerd;

public:
    ObjectCowLeader(const System::MapdataGeoObj &params);
    ~ObjectCowLeader() override;

    void init() override;
    void calc() override;

    /// @addr{0x806BF42C}
    [[nodiscard]] u32 loadFlags() const override {
        return 1;
    }

private:
    /// @brief Represents the different states of the cow's eating animation
    enum class EatAnmType {
        EatST = 0,
        Eat = 1,
        EatED = 2,
    };

    /// @addr{0x806BDCD8}
    void calcFloor() override {
        m_velocity.y = 0.0f;
        m_upForce = GRAVITY_FORCE;
        m_floorNrm = m_railInterpolator->floorNrm(m_railInterpolator->nextPointIdx());
    }

    /// @addr{0x806BD6B0}
    void enterWait() {
        setTarget(m_railInterpolator->curPos() + m_railInterpolator->curTangentDir() * 10.0f);
    }

    /// @addr{0x806BD7D8}
    void enterEat() {
        m_eatAnmType = EatAnmType::EatST;
        u32 rand = System::RaceManager::Instance()->random().getU32(120);
        m_eatFrames = rand + 120;
    }

    /// @addr{0x806BDA1C}
    void enterRoam() {
        m_endedRailSegment = false;
    }

    /// @addr{0x806BD738}
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
    friend class ObjectCowHerd;

public:
    ObjectCowFollower(const System::MapdataGeoObj &params, const EGG::Vector3f &pos, f32 initRot);
    ~ObjectCowFollower() override;

    void init() override;
    void calc() override;

    /// @addr{0x806BF424}
    [[nodiscard]] u32 loadFlags() const override {
        return 1;
    }

    /// @addr{0x806BF420}
    void loadRail() override {}

private:
    /// @addr{0x806BE4E8}
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
    void enterFollowLeader() {
        m_bStopping = false;
        m_interpRate = 0.01f;

        auto &rand = System::RaceManager::Instance()->random();
        m_topSpeed = BASE_TOP_SPEED + rand.getF32(TOP_SPEED_VARIANCE);
    }

    void calcWait();
    void calcFreeRoam();
    void calcFollowLeader();

    const EGG::Vector3f m_posOffset; ///< Fixed offset from the leader's position
    RailInterpolator *m_rail;        ///< Pointer to the leader's rail interpolator
    u16 m_waitFrames;                ///< Number of frames the cow will stand still for
    f32 m_topSpeed;                  ///< The speed the cow will accelerate up to
    bool m_bStopping;                ///< Set when the cow is coming to a stop
    f32 m_railSegThreshold;          ///< The rail segmentT at which a cow will change to state 2

    static constexpr f32 BASE_TOP_SPEED = 2.0f;
    static constexpr f32 TOP_SPEED_VARIANCE = 4.0f - 2.0f;

    /// @brief Distance at which a cow is considered close enough to the rail to stop moving.
    static constexpr f32 DIST_THRESHOLD = 200.0f;

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
    ~ObjectCowHerd() override;

    /// @addr{0x806BF02C}
    /// @brief Assigns the herd's rail to each child.
    void init() override {
        for (auto *&child : m_followers) {
            child->m_rail = m_leader->m_railInterpolator;
        }
    }

    void calc() override;

    /// @addr{0x806BF42C}
    [[nodiscard]] u32 loadFlags() const override {
        return 1;
    }

    /// @addr{0x806BF348}
    void createCollision() override {}

    /// @addr{0x806BF34C}
    void loadRail() override {}

private:
    void checkIntraCollision();

    ObjectCowLeader *m_leader; ///< Pointer to the single leader that owns the rail
    owning_span<ObjectCowFollower *> m_followers; ///< All cows that follow the leader
};

} // namespace Kinoko::Field
