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
    ObjectTruckWagonCart(const System::MapdataGeoObj &params);
    ~ObjectTruckWagonCart() override;

    void calc() override;

    /// @addr{0x806E260C}
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
    [[nodiscard]] const char *getKclName() const override {
        return "TruckWagon";
    }

    void calcCollisionTransform() override;
    Kart::Reaction onCollision(Kart::KartObject *kartObj, Kart::Reaction reactionOnKart,
            Kart::Reaction reactionOnObj, EGG::Vector3f &hitDepth) override;

    /// @addr{0x806E24FC}
    /// @brief Runs when the cart is signaled by the spawner object to spawn
    void activate() {
        reset(0);
        setActive(true);
        loadAABB(0.0f);
    }

    /// @addr{0x806E1F20}
    /// @brief Runs when the cart is signaled by the spawner object to despawn
    void deactivate() {
        reset(0);
        setActive(false);
        unregisterCollision();
        m_nextStateId = 0;
    }

    void reset(u32 idx);

    [[nodiscard]] bool isActive() const {
        return m_active;
    }

    /// @addr{0x806E2060}
    /// @brief Interface used by @ref ObjectTruckWagon to enable or disable a cart
    void setActive(bool isSet) {
        m_active = isSet;
    }

private:
    void enterStateStub() {}

    void calcStateStub() {}

    void calcRolling();
    void calcSuspended();

    /// @addr{0x806E1894}
    /// @brief Runs every frame that the cart has m_currentStateId equal to 2
    /// @todo It's not clear what the intended difference here is compared to rolling state
    /// (m_currentStateId = 0)
    void calcState2() {
        calcRolling();
    }

    void checkRailPointState();
    void calcRailAndVel();

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
            {StateEntry<ObjectTruckWagonCart, &ObjectTruckWagonCart::enterStateStub,
                    &ObjectTruckWagonCart::calcRolling>(0)},
            {StateEntry<ObjectTruckWagonCart, &ObjectTruckWagonCart::enterStateStub,
                    &ObjectTruckWagonCart::calcSuspended>(1)},
            {StateEntry<ObjectTruckWagonCart, &ObjectTruckWagonCart::enterStateStub,
                    &ObjectTruckWagonCart::calcState2>(2)},
            {StateEntry<ObjectTruckWagonCart, &ObjectTruckWagonCart::enterStateStub,
                    &ObjectTruckWagonCart::calcStateStub>(3)},
    }};
};

/// @brief The "spawner" for minecarts on Wario's Gold Mine.
/// @details Each cycle represents two minecart spawns. The first cart spawns at the start of the
/// cycle, the second cart spawns at @ref m_spawn2Frame, and the cycle ends at @ref m_cycleDuration.
/// @ref m_curCartIdx keeps track of which ObjectTruckWagonCart object is spawning next.
class ObjectTruckWagon final : public ObjectCollidable {
public:
    ObjectTruckWagon(const System::MapdataGeoObj &params);
    ~ObjectTruckWagon() override;

    void init() override;
    void calc() override;

    /// @addr{0x806E24EC}
    /// @copybrief ObjectBase::loadFlags()
    /// @return Returns @ref eLoadFlags::Calc and @ref eLoadFlags::Draw so that the object is
    /// calculated every frame
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
