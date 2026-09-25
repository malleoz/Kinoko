#pragma once

#include "game/field/StateManager.hh"
#include "game/field/obj/ObjectCollidable.hh"

namespace Kinoko::Field {

/// @brief The individual minecarts that spawn from the @ref ObjectTruckWagon spawner
/// @details Carts are toggled active or inactive by the @ref ObjectTruckWagon spawner object. When
/// they are active, they move along a rail. In the base game, they start by rolling along the
/// ground. Once they reach the fork at the end of the mine, they get picked up and move along the
/// rail while suspended mid-air.
class ObjectTruckWagonCart final : public ObjectCollidable, private StateManager {
public:
    /// @addr{0x806DFE9C}
    /// @copybrief ObjectCollidable::ObjectCollidable(const System::MapdataGeoObj &)
    /// @param params The parameters used to initialize the object
    /// @details Initializes @ref m_active to `true`. Zeroes @ref m_vel, @ref m_lastVel, @ref m_up,
    /// @ref m_tangent, and @ref m_pitch.
    ObjectTruckWagonCart(const System::MapdataGeoObj &params)
        : ObjectCollidable(params),
          StateManager(this, STATE_ENTRIES),
          m_active(true),
          m_vel(EGG::Vector3f::zero),
          m_lastVel(EGG::Vector3f::zero),
          m_up(EGG::Vector3f::zero),
          m_tangent(EGG::Vector3f::zero),
          m_pitch(0.0f) {}

    /// @addr{0x806E00F4}
    /// @brief Default virtual destructor
    ~ObjectTruckWagonCart() override = default;

    /// @addr{0x806E03E8}
    /// @copybrief ObjectBase::calc()
    /// @details Early returns if the minecart if currently not tangible. Otherwise, updates the
    /// rail interpolator and sets the minecart's velocity accordingly. Evaluates the minecart's
    /// state machine. Finally, refreshes the minecart's transformation matrix.
    void calc() override {
        if (!m_active) {
            return;
        }

        calcRailAndVel();
        StateManager::calc();
        calcTransform();
    }

    /// @addr{0x806E260C}
    /// @copybrief ObjectBase::getName()
    /// @return The name of the object, `TruckWagon`
    [[nodiscard]] const char *getName() const override {
        return "TruckWagon";
    }

    /// @addr{0x806E2744}
    /// @copybrief ObjectBase::loadFlags()
    /// @return Returns @ref eLoadFlags::Calc, so that object is calculated every frame.
    [[nodiscard]] LoadFlags loadFlags() const override {
        return LoadFlags(eLoadFlags::Calc);
    }

    /// @addr{0x806E2618}
    /// @copybrief ObjectBase::getKclName()
    /// @return The model name of the truck wagon (`TruckWagon`).
    [[nodiscard]] const char *getKclName() const override {
        return "TruckWagon";
    }

    void calcCollisionTransform() override;
    Kart::Reaction onCollision(Kart::KartObject *kartObj, Kart::Reaction reactionOnKart,
            Kart::Reaction reactionOnObj, EGG::Vector3f &hitDepth) override;

    /// @addr{0x806E24FC}
    /// @brief Runs when the cart is signaled by the spawner object to spawn
    /// @details Resets the minecart to its initial state. Marks the minecart as active and loads
    /// its collision.
    void activate() {
        reset(0);
        setActive(true);
        loadAABB(0.0f);
    }

    /// @addr{0x806E1F20}
    /// @brief Runs when the cart is signaled by the spawner object to despawn
    /// @details Resets the minecart to its initial state. Marks the minecart as inactive and
    /// disables its collision. Finally, transitions the minecart to the rolling state so that it
    /// moves along the rail once the minecart becomes active again.
    void deactivate() {
        reset(0);
        setActive(false);
        unregisterCollision();
        m_nextStateId = 0;
    }

    void reset(u32 idx);

    /// @beginGetters

    /// @brief Whether the minecart is currently tangible
    /// @return `true` if the minecart has spawned, `false` if it is intangible
    [[nodiscard]] bool isActive() const {
        return m_active;
    }

    /// @endGetters

    /// @beginSetters

    /// @addr{0x806E2060}
    /// @brief Interface used by @ref ObjectTruckWagon to enable or disable a cart
    /// @param isSet Whether the minecart should be spawned or despawned
    void setActive(bool isSet) {
        m_active = isSet;
    }

    /// @endSetters

private:
    void calcRolling();
    void calcSuspended();

    /// @addr{0x806E1894}
    /// @brief Runs every frame that the cart has m_currentStateId equal to 2
    /// @todo It's not clear what the intended difference here is compared to rolling state
    /// (m_currentStateId = 0)
    void calcState2() {
        calcRolling();
    }

    /// @addr{0x806E1DD0}
    /// @brief Checks whether the minecart should transition to the suspended state where it is
    /// lifted into the air
    /// @details If the minecart is currently already in the suspended state, then early returns.
    /// Otherwise, checks the current rail node's second setting. If it is set to 0, then the
    /// minecart will return to the rolling state. If set to 1, then the minecart will transition to
    /// the suspended state.
    void checkRailPointState() {
        if (m_currentStateId == 1) {
            return;
        }

        u16 setting = m_railInterpolator->curPoint().setting[1];
        if (setting <= 2) {
            m_nextStateId = setting;
        }
    }

    /// @addr{0x806E1E34}
    /// @brief Updates the rail interpolator and the minecart's velocity
    /// @details Evaluates the rail interpolator for the current frame. If the minecart has reached
    /// the end of a rail segment, calls @ref checkRailPointState() to see if the mincart should
    /// transition from/to the rolling/suspended state. Otherwise, if the minecart has reached the
    /// end of the rail, calls @ref deactivate() to despawn the minecart. Regardless, updates @ref m_vel to reflect the rail's current velocity
    void calcRailAndVel() {
        switch (m_railInterpolator->calc()) {
        case RailInterpolator::Status::SegmentEnd:
            checkRailPointState();
            break;
        case RailInterpolator::Status::ChangingDirection:
            deactivate();
            break;
        default:
            break;
        }

        m_vel.x = m_railInterpolator->currSpeed() * m_railInterpolator->curTangentDir().x;
        m_vel.z = m_railInterpolator->currSpeed() * m_railInterpolator->curTangentDir().z;
    }

    bool m_active;           ///< Whether or not the minecart is spawned and has collision
    EGG::Vector3f m_vel;     ///< Current velocity along the rail, with gravity applied
    f32 m_speed;             ///< Speed of the current segment of the rail interpolator
    EGG::Vector3f m_lastVel; ///< Last frame's velocity, used during the suspended state
    EGG::Vector3f m_up;      ///< Up direction vector
    EGG::Vector3f m_tangent; ///< Forward direction vector
    f32 m_pitch;             ///< Current rocking angle of the suspended minecart
    f32 m_angVel;            ///< Angular velocity of the swing of the suspended minecart

    /// @brief The enter and calc functions for each @ref StateManager entry
    static constexpr std::array<StateManagerEntry, 4> STATE_ENTRIES = {{
            {StateEntry<ObjectTruckWagonCart, nullptr, &ObjectTruckWagonCart::calcRolling>(0)},
            {StateEntry<ObjectTruckWagonCart, nullptr, &ObjectTruckWagonCart::calcSuspended>(1)},
            {StateEntry<ObjectTruckWagonCart, nullptr, &ObjectTruckWagonCart::calcState2>(2)},
            {StateEntry<ObjectTruckWagonCart, nullptr, nullptr>(3)},
    }};
};

/// @brief The "spawner" for minecarts on Wario's Gold Mine.
/// @details Each cycle represents two minecart spawns. The first cart spawns at the start of the
/// cycle, the second cart spawns at @ref m_spawn2Frame, and the cycle ends at @ref m_cycleDuration.
/// @ref m_curCartIdx keeps track of which ObjectTruckWagonCart object is spawning next.
class ObjectTruckWagon final : public ObjectCollidable {
public:
    ObjectTruckWagon(const System::MapdataGeoObj &params);

    /// @addr{0x806E21EC}
    /// @brief Default virtual destructor
    ~ObjectTruckWagon() override = default;

    void init() override;
    void calc() override;

    /// @addr{0x806E24EC}
    /// @copybrief ObjectBase::loadFlags()
    /// @return Returns @ref eLoadFlags::Calc and @ref eLoadFlags::Draw so that the object is
    /// calculated every frame
    [[nodiscard]] LoadFlags loadFlags() const override {
        return LoadFlags().setBit(eLoadFlags::Calc, eLoadFlags::Draw);
    }

    /// @addr{0x806E24E8}
    /// @copybrief ObjectBase::loadGraphics()
    /// @details This is a no-op in the base game since the spawner does not have any graphics.
    void loadGraphics() override {}

    /// @addr{0x806E24E0}
    /// @copybrief ObjectBase::createCollision()
    /// @details no-op because the spawner itself does not have any collision.
    void createCollision() override {}

    /// @addr{0x806E24E4}
    /// @copybrief ObjectBase::loadRail()
    /// @details no-op because the spawner itself does not have any rail. The individual carts
    /// manage their own positions along the rail.
    void loadRail() override {}

private:
    owning_span<ObjectTruckWagonCart *> m_carts; ///< Pointers to each cart that spawns
    const s32 m_spawn2Frame;   ///< Frame that the second minecart in a cycle spawns
    const s32 m_cycleDuration; ///< Total duration of a cycle
    s32 m_cycleFrame;          ///< Current frame modulo cycle duration
    s32 m_curCartIdx;          ///< Index into @ref m_carts representing the next cart to spawn
};

} // namespace Kinoko::Field
