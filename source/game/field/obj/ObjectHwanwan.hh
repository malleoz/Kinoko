#pragma once

#include "game/field/StateManager.hh"
#include "game/field/obj/ObjectCollidable.hh"

#include "game/kart/KartCollide.hh"
#include "game/kart/KartObject.hh"

namespace Kinoko::Field {

/// @brief Represents the walking Chain Chomps on DS Peach Gardens
class ObjectHwanwan : public ObjectCollidable, public StateManager {
    friend class ObjectHwanwanManager;

public:
    ObjectHwanwan(const System::MapdataGeoObj &params);
    ~ObjectHwanwan() override;

    void init() override;
    void calc() override;

    /// @addr{0x806EC7B8}
    [[nodiscard]] u32 loadFlags() const override {
        return 1;
    }

    /// @addr{0x806EC7AC}
    [[nodiscard]] const char *getKclName() const override {
        return "wanwan";
    }

    /// @addr{0x806EC7A8}
    /// @brief Does nothing since its position is enforced by @ref ObjectHwanwanManager
    void loadRail() override {}

    /// @addr{0x806E9CAC}
    /// @brief Acts as a wall if the player is moving slowly, otherwise flips the player.
    Kart::Reaction onCollision(Kart::KartObject *kartObj, Kart::Reaction reactionOnKart,
            Kart::Reaction /*reactionOnObj*/, EGG::Vector3f & /*hitDepth*/) override {
        return kartObj->speedRatioCapped() < 0.5f ? Kart::Reaction::WallAllSpeed : reactionOnKart;
    }

private:
    void enterStateStub() {}
    void calcStateStub() {}

    /// @addr{0x806E9E10}
    /// @brief Runs every frame that the Chain Chomp is walking/bouncing along the rail
    /// @details For all intents and purposes, this is run every frame. The only time this is not
    /// run is when the Chain Chomp enters a "roll" state. This state is only ever used in the
    /// Rainbow Road tournament, when a Chain Chomp falls off the higher path by the split path
    /// section after the cannon.
    void calcBounce() {
        if (m_touchingGround) {
            m_bounceVel += EGG::Vector3f::ey * 12.0f;
        }
    }

    void checkFloorCollision();
    void calcUp();

    const EGG::Vector3f m_initPos; ///< Spawn position
    EGG::Vector3f m_workPos;       ///< Current position
    EGG::Vector3f m_extVel;        ///< External velocity (gravity and bounce velocity)
    EGG::Vector3f m_bounceVel;     ///< Upwards velocity impulse when Chain Chomp contacts the floor
    EGG::Vector3f m_tangent;       ///< Forward direction along the rail
    EGG::Vector3f m_up;            ///< Smoothed up vector
    EGG::Vector3f m_targetUp;      ///< Target up vector, which is the normal of the colliding floor
    bool m_touchingGround;         ///< Whether the Chain Chomp is currently touching the floor
    f32 m_targetY; ///< Rail height, used to skip collision checks when far enough in the air

    static constexpr f32 DIAMETER = 400.0f; ///< The diameter of the Chain Chomp's collision sphere

    static constexpr std::array<StateManagerEntry, 2> STATE_ENTRIES = {{
            {StateEntry<ObjectHwanwan, &ObjectHwanwan::enterStateStub, &ObjectHwanwan::calcBounce>(
                    0)},
            {StateEntry<ObjectHwanwan, &ObjectHwanwan::enterStateStub,
                    &ObjectHwanwan::calcStateStub>(1)},
    }};
};

class ObjectHwanwanManager : public ObjectCollidable {
public:
    ObjectHwanwanManager(const System::MapdataGeoObj &params);
    ~ObjectHwanwanManager() override;

    void init() override;
    void calc() override;

    /// @addr{0x806C69B8}
    [[nodiscard]] u32 loadFlags() const override {
        return 1;
    }

    /// @addr{0x806C69B4}
    /// @brief Does nothing since this is just a manager class
    void loadGraphics() override {}

    /// @addr{0x806C69B0}
    /// @brief Does nothing since this is just a manager class
    void createCollision() override {}

private:
    void calcState();

    ObjectHwanwan *m_hwanwan; ///< Pointer to the underlying Chain Chomp object
};

} // namespace Kinoko::Field
