#pragma once

#include "game/field/StateManager.hh"
#include "game/field/obj/ObjectCollidable.hh"

#include "game/kart/KartCollide.hh"
#include "game/kart/KartObject.hh"

namespace Kinoko::Field {

/// @brief Represents the walking Chain Chomps on DS Peach Gardens
/// @details Chain Chomps move along a rail and bounce upwards whenever they collide with the
/// floor.
class ObjectHwanwan final : public ObjectCollidable, private StateManager {
    /// @brief Grants the manager class access to the Chain Chomp's state
    friend class ObjectHwanwanManager;

public:
    /// @addr{0x806E95B0}
    /// @copybrief ObjectCollidable::ObjectCollidable(const System::MapdataGeoObj &)
    /// @param params The parameters used to initialize the object
    /// @details Caches the Chain Chomp's initial position to @ref m_initPos.
    ObjectHwanwan(const System::MapdataGeoObj &params)
        : ObjectCollidable(params),
          StateManager(this, STATE_ENTRIES),
          m_initPos(pos()) {}

    /// @addr{0x806EC6E0}
    /// @brief Default virtual destructor
    ~ObjectHwanwan() override = default;

    void init() override;
    void calc() override;

    /// @addr{0x806EC7B8}
    /// @copybrief ObjectBase::loadFlags()
    /// @return Returns @ref eLoadFlags::Calc, so that object is calculated every frame.
    [[nodiscard]] LoadFlags loadFlags() const override {
        return LoadFlags(eLoadFlags::Calc);
    }

    /// @addr{0x806EC7AC}
    /// @copybrief ObjectBase::getKclName()
    /// @return The model name of the Chain Chomp (`wanwan`)
    [[nodiscard]] const char *getKclName() const override {
        return "wanwan";
    }

    /// @addr{0x806EC7A8}
    /// @copybrief ObjectBase::loadRail()
    /// @details Does nothing since its position is enforced by @ref ObjectHwanwanManager
    void loadRail() override {}

    /// @addr{0x806E9CAC}
    /// @copybrief ObjectCollidable::onCollision()
    /// @param kartObj The kart object that collided with this object
    /// @param reactionOnKart The reaction that should be applied to the kart upon collision
    /// @return @ref Kart::Reaction::Wall if the player's speed is under 50%, otherwise @ref
    /// Kart::Reaction::LaunchAwayFlipTwice.
    /// @details Chain Chomps act like a wall when the kart is driving under 50% of its base speed.
    Kart::Reaction onCollision(Kart::KartObject *kartObj, Kart::Reaction reactionOnKart,
            Kart::Reaction /*reactionOnObj*/, EGG::Vector3f & /*hitDepth*/) override {
        return kartObj->speedRatioCapped() < 0.5f ? Kart::Reaction::Wall : reactionOnKart;
    }

private:
    /// @addr{0x806E9E10}
    /// @brief Runs every frame that the Chain Chomp is walking/bouncing along the rail
    /// @details If the Chain Chomp is touching the ground this frame, induces an upwards bounce
    /// velocity of `12.0f`.
    /// @note For all intents and purposes, this is run every frame. The only time this is not
    /// run is when the Chain Chomp enters a "roll" state. This state is only ever used in the
    /// Rainbow Road tournament, when a Chain Chomp falls off the higher path by the split path
    /// section after the cannon.
    void calcBounce() {
        if (m_touchingGround) {
            m_bounceVel += EGG::Vector3f::ey * 12.0f;
        }
    }

    void checkFloorCollision();

    /// @addr{0x806EAAE8}
    /// @brief Smoothly interpolates the Chain Chomp's up vector towards the target up vector
    /// @details Uses an interpolation rate of `0.1f`.
    void calcUp() {
        constexpr f32 INTERP_RATE = 0.1f;

        m_up = Interpolate(INTERP_RATE, m_up, m_targetUp);
        if (m_up.squaredLength() > std::numeric_limits<f32>::epsilon()) {
            m_up.normalise2();
        } else {
            m_up = EGG::Vector3f::ey;
        }
    }

    const EGG::Vector3f m_initPos; ///< Spawn position
    EGG::Vector3f m_workPos;       ///< Staged position for this frame
    EGG::Vector3f m_extVel;        ///< External velocity (gravity and bounce velocity)
    EGG::Vector3f m_bounceVel;     ///< Upwards velocity impulse when Chain Chomp contacts the floor
    EGG::Vector3f m_tangent;       ///< Forward direction along the rail
    EGG::Vector3f m_up;            ///< Smoothed up vector
    EGG::Vector3f m_targetUp;      ///< Target up vector, which is the normal of the colliding floor
    bool m_touchingGround;         ///< Whether the Chain Chomp is currently touching the floor
    f32 m_targetY; ///< Rail height, used to skip collision checks when far enough in the air

    static constexpr f32 DIAMETER = 400.0f; ///< The diameter of the Chain Chomp's collision sphere

    /// @brief The enter and calc functions for each @ref StateManager entry
    static constexpr std::array<StateManagerEntry, 2> STATE_ENTRIES = {{
            {StateEntry<ObjectHwanwan, nullptr, &ObjectHwanwan::calcBounce>(0)},
            {StateEntry<ObjectHwanwan, nullptr, nullptr>(1)},
    }};
};

/// @brief Manager class for @ref ObjectHwanwan
/// @details For Kinoko, this is effectively a very simple wrapper over a single @ref ObjectHwanwan.
/// However, in the base game, this manager class is responsible for managing a trail of item boxes
/// behind the Chain Chomp.
class ObjectHwanwanManager final : public ObjectCollidable {
public:
    /// @addr{0x806C5354}
    /// @brief Constructor
    /// @param params The parameters used to initialize the object
    /// @details Constructs the underlying @ref ObjectHwanwan obhect, doubles its scale, and loads
    /// it.
    ObjectHwanwanManager(const System::MapdataGeoObj &params) : ObjectCollidable(params) {
        m_hwanwan = EGG::egg_new<ObjectHwanwan>(params);
        m_hwanwan->setScale(2.0f);
        m_hwanwan->load();
    }

    /// @addr{0x806C56DC}
    /// @brief Default virtual destructor
    ~ObjectHwanwanManager() override = default;

    void init() override;

    /// @addr{0x806C5AC4}
    /// @copybrief ObjectBase::calc()
    /// @details Updates the Chain Chomp's state and position along the rail every frame.
    void calc() override {
        calcState();
        calcPosAndTangent();
    }

    /// @addr{0x806C69B8}
    /// @copybrief ObjectBase::loadFlags()
    /// @return Returns @ref eLoadFlags::Calc, so that object is calculated every frame.
    [[nodiscard]] LoadFlags loadFlags() const override {
        return LoadFlags(eLoadFlags::Calc);
    }

    /// @addr{0x806C69B4}
    /// @copybrief ObjectBase::loadGraphics()
    /// @details Does nothing since this is just a manager class
    void loadGraphics() override {}

    /// @addr{0x806C69B0}
    /// @copybrief ObjectBase::createCollision()
    /// @details no-op since this is just a manager class
    void createCollision() override {}

private:
    /// @addr{0x806C5DE0}
    /// @brief Updates the Chain Chomp's state based on the current rail segment
    /// @details If the Chain Chomp has reached the end of the current rail segment and the next
    /// rail point's second setting is set to 1, then transitions to @ref ObjectHwanwan to the
    /// rolling state. Otherwise, if the @ref ObjectHwanwan is in the rolling state and has been
    /// rolling for 60 frames, it transitions back to the idle state.
    /// @note In practice, for Nintendo tracks, this does nothing. Rail point setting 2, which
    /// represents the Chain Chomp entering a roll animation is only ever set to 2 in the Rainbow
    /// Road tournament.
    void calcState() {
        constexpr f32 ROLL_DURATION = 60.0f;

        if (m_railInterpolator->calc() == RailInterpolator::Status::SegmentEnd &&
                m_railInterpolator->curPoint().setting[1] == 1 &&
                m_hwanwan->m_currentStateId != 2) {
            m_hwanwan->m_nextStateId = 1;
        }

        if (m_hwanwan->m_currentStateId == 1 && m_hwanwan->m_currentFrame >= ROLL_DURATION) {
            m_hwanwan->m_nextStateId = 0;
        }
    }

    /// @addr{0x806C6148}
    /// @brief Updates the Chain Chomp's position and tangent
    /// @details Snaps the Chain Chomp's X and Z position to the rail and sets the target Y position
    /// to the rail's height. Also updates the Chain Chomp's tangent direction.
    void calcPosAndTangent() {
        const auto &curPos = m_railInterpolator->curPos();
        m_hwanwan->m_workPos.x = curPos.x;
        m_hwanwan->m_workPos.z = curPos.z;
        m_hwanwan->m_targetY = curPos.y;
        m_hwanwan->m_tangent = m_railInterpolator->curTangentDir();
    }

    ObjectHwanwan *m_hwanwan; ///< Pointer to the underlying Chain Chomp object
};

} // namespace Kinoko::Field
