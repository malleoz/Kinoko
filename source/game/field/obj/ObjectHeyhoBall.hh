#pragma once

#include "game/field/StateManager.hh"
#include "game/field/obj/ObjectProjectile.hh"

namespace Kinoko::Field {

/// @brief The cannonball projectiles on GBA Shy Guy Beach
/// @details Cannonballs are fired from a @ref ObjectHeyhoShip and have a parabolic flight path.
/// When they land, they blink for a few seconds before exploding. The explosion can either knock
/// the player into the air and make them lose their items or just spin them out without losing
/// items, depending on how much time has elapsed since the explosion. The synchronization between a
/// cannonball and the ship is managed by @ref ObjectHeyhoShipManager.
class ObjectHeyhoBall final : public ObjectProjectile, private StateManager {
public:
    ObjectHeyhoBall(const System::MapdataGeoObj &params);
    ~ObjectHeyhoBall() override;

    void init() override;

    /// @addr{0x806D0780}
    /// @copybrief ObjectBase::calc()
    void calc() override {
        StateManager::calc();
        setPos(m_workingPos);
    }

    [[nodiscard]] Kart::Reaction onCollision(Kart::KartObject *kartObj,
            Kart::Reaction reactionOnKart, Kart::Reaction reactionOnObj,
            EGG::Vector3f &hitDepth) override;

    /// @addr{0x806D18EC}
    /// @copybrief ObjectBase::loadFlags()
    /// @return Returns @ref eLoadFlags::Calc, so that object is calculated every frame.
    [[nodiscard]] LoadFlags loadFlags() const override {
        return LoadFlags(eLoadFlags::Calc);
    }

    void initProjectile(const EGG::Vector3f &pos) override;

    /// @addr{0x806D188C}
    void onLaunch() override {
        m_nextStateId = 1;
    }

private:
    /// @brief Describes the intensity of the explosion when a kart collides with the explosion
    enum class ExplosionIntensity {
        ExplosionLoseItem = 0, ///< The kart loses its items and is knocked upwards
        SpinSomeSpeed = 1,     ///< The kart spins out but keeps its items
    };

    void enterStateStub() {}

    /// @addr{0x806D0A3C}
    /// @brief Runs once when the cannonball is fired
    void enterFalling() {
        if (!getUnit()) {
            loadAABB(0.0f);
            resize(INIT_BLAST_RADIUS, 0.0f);
        }
    }

    /// @addr{0x806D0C0C}
    /// @brief Runs once when the cannonball has landed and is blinking before exploding
    void enterBlinking() {
        m_workingPos = m_initPos + EGG::Vector3f::ey * -BALL_RADIUS;
    }

    /// @addr{0x806D0D84}
    /// @brief Runs once when the cannonball has started exploding
    void enterExploding() {
        m_scaleChangeRate = (1.2f * m_blastRadiusRatio - 1.0f) / 40.0f / 40.0f;
    }

    /// @addr{0x806D0A14}
    /// @brief Runs every frame that the cannonball is not visible
    void calcIntangible() {
        m_workingPos = m_shipPos;
    }

    void calcFalling();

    /// @addr{0x806D0CD8}
    /// @brief Runs every frame that the cannonball is blinking before exploding
    void calcBlinking() {
        constexpr u32 REST_FRAMES = 180;

        if (m_currentFrame >= REST_FRAMES) {
            m_nextStateId = 3;
        }
    }

    void calcExploding();

    /// @addr{0x806D14D8}
    /// @brief Runs after the explosion has finished to set the final scale of the cannonball
    void calcFinishedExplodingScale() {
        constexpr EGG::Vector3f BALL_SCALE = EGG::Vector3f(1.001f, 1.001f, 1.001f);

        setScale(BALL_SCALE);
    }

    /// @addr{0x806D14D8}
    /// @brief Calculates the cannonball scale on the first 46 frames of the explosion
    /// @todo Describe the parabola once we better understand the significance of the "40" term
    void calcExplodingScale() {
        f32 shrinkFrames = static_cast<f32>(m_currentFrame) - 40.0f;
        f32 scale = 1.2f * m_blastRadiusRatio + -m_scaleChangeRate * shrinkFrames * shrinkFrames;

        scale = std::min(m_blastRadiusRatio, scale);
        setScale(scale);
    }

    const f32 m_airtime;            ///< Number of frames between shooting and landing
    EGG::Vector3f m_shipPos;        ///< Position of the ship firing the ball
    const EGG::Vector3f m_initPos;  ///< Target landing position
    EGG::Vector3f m_xzDir;          ///< XZ direction of the cannonball's flight path
    f32 m_yDist;                    ///< Vertical distance between the ship and the landing position
    f32 m_xzSpeed;                  ///< XZ speed of the cannonball's flight path
    f32 m_initYSpeed;               ///< Initial vertical projectile speed
    f32 m_blastRadiusRatio;         ///< Ratio between the blast radius and the shell's radius
    f32 m_scaleChangeRate;          ///< Quadratic reduction rate for the explosion scale
    EGG::Vector3f m_workingPos;     ///< Intermediate position of the cannonball
    ExplosionIntensity m_intensity; ///< Collision reaction strength

    static constexpr f32 BALL_RADIUS = 50.0f;         ///< Radius of the cannonball
    static constexpr f32 INIT_BLAST_RADIUS = 1500.0f; ///< Initial radius of the explosion

    /// @brief The enter and calc functions for each @ref StateManager entry
    static constexpr std::array<StateManagerEntry, 4> STATE_ENTRIES = {{
            {StateEntry<ObjectHeyhoBall, &ObjectHeyhoBall::enterStateStub,
                    &ObjectHeyhoBall::calcIntangible>(0)},
            {StateEntry<ObjectHeyhoBall, &ObjectHeyhoBall::enterFalling,
                    &ObjectHeyhoBall::calcFalling>(1)},
            {StateEntry<ObjectHeyhoBall, &ObjectHeyhoBall::enterBlinking,
                    &ObjectHeyhoBall::calcBlinking>(2)},
            {StateEntry<ObjectHeyhoBall, &ObjectHeyhoBall::enterExploding,
                    &ObjectHeyhoBall::calcExploding>(3)},
    }};
};

} // namespace Kinoko::Field
