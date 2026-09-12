#pragma once

#include "game/field/obj/ObjectCollidable.hh"

namespace Kinoko::Field {

/// @brief The fireball shot by the giant Bowser statue in the half-pipe section of Bowser's Castle
class ObjectKoopaBall final : public ObjectCollidable {
public:
    ObjectKoopaBall(const System::MapdataGeoObj &params);
    ~ObjectKoopaBall() override;

    void init() override;
    void calc() override;

    /// @addr{0x80771F68}
    /// @copybrief ObjectBase::loadFlags()
    /// @return Returns @ref eLoadFlags::Calc, so that object is calculated every frame.
    [[nodiscard]] LoadFlags loadFlags() const override {
        return LoadFlags(eLoadFlags::Calc);
    }

    Kart::Reaction onCollision(Kart::KartObject *kartObj, Kart::Reaction reactionOnKart,
            Kart::Reaction reactionOnObj, EGG::Vector3f &hitDepth) override;

    /// @addr{0x80771F60}
    /// @copydoc ObjectCollidable::getCollisionTranslation()
    /// @details Extends the collision translation to include the velocity of the koopa ball, so
    /// collision detection happens from further away when the ball is moving towards the player.
    const EGG::Vector3f &getCollisionTranslation() const override {
        return m_vel;
    }

private:
    /// @brief Describes the current collision state of the fireball
    enum class State {
        Tangible = 0,   ///< The fireball is active and can collide with karts
        Intangible = 1, ///< The fireball is inactive and cannot collide with karts
        Exploding = 2,  ///< The fireball is exploding and will throw the player upwards
    };

    void initCooldownTimer() {
        constexpr u32 START_COOLDOWN = 221;
        m_cooldownTimer = START_COOLDOWN;
    }

    void calcTangible();
    void calcExploding();
    void calcIntangible();
    void calcRot();

    /// @addr{0x807719A0}
    /// @brief Slows down the fireball's rail speed once reaching the end of a rail segment
    /// @details This does not occur in the base game
    void calcSlowdown() {
        constexpr f32 VERTICAL_VELOCITY = 60.0f;

        m_vel.y = VERTICAL_VELOCITY;
        m_railInterpolator->setSpeed(INITIAL_VELOCITY / INITIAL_ANGULAR_SPEED);
    }

    /// @addr{0x80771B68}
    /// @brief Runs once when the fireball starts exploding
    void setExplosionScale() {
        m_curScale = INITIAL_EXPLOSION_SCALE;
        setScale(INITIAL_EXPLOSION_SCALE);
    }

    /// @addr{0x80771BCC}
    /// @brief Resets the scale of the fireball to its initial value
    void resetScale() {
        constexpr f32 INIT_FACTOR = 1.01f;

        m_curScale = INIT_FACTOR;
        setScale(INIT_FACTOR);
    }

    void checkSphereFull();

    State m_state;                      ///< Current collision state of the fireball
    f32 m_initPosY;                     ///< Starting height
    EGG::Vector3f m_vel;                ///< Current velocity of the fireball
    f32 m_angSpeed;                     ///< Angular speed of the fireball in degrees
    s32 m_cooldownTimer;                ///< Frames until koopa will start to shoot the fireball
    Render::DrawMdl *m_bombCoreDrawMdl; ///< DrawMdl for the bomb animation
    s32 m_explodeTimer;   ///< Remaining number of frames the fireball is exploding for
    u32 m_animFramecount; ///< Duration of the explosion animation defined in bombCore.brres
    f32 m_angleRad;       ///< Total angle of rotation in radians
    f32 m_curScale;       ///< Scale of the fireball, which increases during the explosion

    static constexpr f32 RADIUS_AABB = 870.0f * 2.0f; ///< Radius of the BoxColUnit

    /// @brief Scale when it starts exploding. Smaller than before the explosion
    static constexpr f32 INITIAL_EXPLOSION_SCALE = RADIUS_AABB / 940.0f * 0.5f;

    /// @brief Rate at which the explosion scale increases per frame, after an initial wait period
    static constexpr f32 SCALE_DELTA = INITIAL_EXPLOSION_SCALE / 25.0f;

    static constexpr f32 INITIAL_VELOCITY = 400.0f;    ///< Initial speed of the fireball
    static constexpr f32 INITIAL_ANGULAR_SPEED = 3.0f; ///< Initial angular speed in degrees
    static constexpr f32 INITIAL_Y_VEL = -30.0f;       ///< Initial vertical velocity upon firing
};

} // namespace Kinoko::Field
