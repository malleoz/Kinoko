#pragma once

#include "game/field/StateManager.hh"
#include "game/field/obj/ObjectCollidable.hh"

#include "game/kart/KartCollide.hh"
#include "game/kart/KartObject.hh"

namespace Kinoko::Field {

/// @brief Represents the walking Chain Chomps on DS Peach Gardens
class ObjectHwanwan final : public ObjectCollidable, private StateManager {
    /// @brief Grants the manager class access to the Chain Chomp's state
    friend class ObjectHwanwanManager;

public:
    ObjectHwanwan(const System::MapdataGeoObj &params);
    ~ObjectHwanwan() override;

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

    /// @brief The enter and calc functions for each @ref StateManager entry
    static constexpr std::array<StateManagerEntry, 2> STATE_ENTRIES = {{
            {StateEntry<ObjectHwanwan, nullptr, &ObjectHwanwan::calcBounce>(0)},
            {StateEntry<ObjectHwanwan, nullptr, nullptr>(1)},
    }};
};

class ObjectHwanwanManager final : public ObjectCollidable {
public:
    ObjectHwanwanManager(const System::MapdataGeoObj &params);
    ~ObjectHwanwanManager() override;

    void init() override;

    /// @addr{0x806C5AC4}
    /// @copybrief ObjectBase::calc()
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
    void calcState();

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
