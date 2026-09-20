#pragma once

#include "game/field/obj/ObjectCollidable.hh"

namespace Kinoko::Field {

/// @brief The fireball shot by the giant Bowser statue in the half-pipe section of Bowser's Castle
/// @details The fireball is shot out with an initial speed of @ref INITIAL_SPEED with a vertical
/// speed of @ref INITIAL_Y_SPEED. Once it reaches the end of the rail, it begins exploding.
/// The explosion lasts a duration of @ref m_animDuration, where collision is disabled for the
/// last 20 frames of the explosion.
class ObjectKoopaBall final : public ObjectCollidable {
public:
    /// @addr{0x80770384}
    /// @copydoc ObjectCollidable::ObjectCollidable(const System::MapdataGeoObj &)
    /// @details Initializes the fireball's velocity to zero.
    ObjectKoopaBall(const System::MapdataGeoObj &params) : ObjectCollidable(params), {
        m_vel = EGG::Vector3f::zero;
    }

    /// @addr{0x80771F70}
    /// @brief Virtual destructor that destroys the associated bomb core draw model.
    ~ObjectKoopaBall() override {
        EGG::egg_delete(m_bombCoreDrawMdl);
    }

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
    /// @copybrief ObjectCollidable::getCollisionTranslation()
    /// @return Returns the current velocity of the fireball.
    /// @details Extends the collision translation to include the velocity of the fireball, so
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

    /// @addr{0x8077079C}
    /// @brief Initializes the cooldown timer to `221` frames.
    void initCooldownTimer() {
        constexpr u32 START_COOLDOWN = 221;
        m_cooldownTimer = START_COOLDOWN;
    }

    void calcTangible();
    void calcExploding();

    /// @addr{0x80771248}
    /// @brief Runs every frame that the fireball is intangible
    /// @details Once the cooldown timer has expired, the fireball will become tangible.
    /// Reinitialize its rail speed, angular speed, vertical speed, and cooldown timer.
    void calcIntangible() {
        constexpr s32 COOLDOWN_FRAMES = 210;

        if (m_cooldownTimer >= 0) {
            return;
        }

        m_state = State::Tangible;
        m_railInterpolator->setSpeed(INITIAL_SPEED);
        m_angSpeed = INITIAL_ANGULAR_SPEED;
        m_vel.y = INITIAL_Y_SPEED;
        m_cooldownTimer = COOLDOWN_FRAMES;
    }

    /// @addr{0x8077151C}
    /// @brief Updates the fireball's angular rotation and transformation matrix
    /// @details Applies @ref m_angSpeed to the fireball's rotation around the x-axis. Updates the
    /// transformation matrix accordingly.
    void calcRot() {
        m_angleRad += -m_angSpeed * DEG2RAD;

        EGG::Matrix34f mat = EGG::Matrix34f::ident;
        mat.makeR(EGG::Vector3f(m_angleRad, 0.0f, 0.0f));
        mat = transform().multiplyTo(mat);
        mat.setBase(3, pos());
        setTransform(mat);
    }

    /// @addr{0x807719A0}
    /// @brief Slows down the fireball's rail speed once reaching the end of a rail segment
    /// @details This does not occur in the base game, but we implement it in Kinoko for
    /// completeness.
    void calcSlowdown() {
        constexpr f32 VERTICAL_SPEED = 60.0f;

        m_vel.y = VERTICAL_SPEED;
        m_railInterpolator->setSpeed(INITIAL_SPEED / INITIAL_ANGULAR_SPEED);
    }

    /// @addr{0x80771B68}
    /// @brief Runs once when the fireball starts exploding
    /// @details Sets the fireball's scale to @ref INITIAL_EXPLOSION_SCALE.
    void setExplosionScale() {
        m_curScale = INITIAL_EXPLOSION_SCALE;
        setScale(INITIAL_EXPLOSION_SCALE);
    }

    /// @addr{0x80771BCC}
    /// @brief Resets the scale of the fireball to `1.01f`.
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
    s32 m_explodeTimer;                 ///< Remaining explosion duration
    u32 m_animDuration;               ///< Explosion animation duration defined in bombCore.brres
    f32 m_angleRad;                     ///< Total angle of rotation in radians
    f32 m_curScale;                     ///< Current scale of the fireball

    static constexpr f32 RADIUS_AABB = 870.0f * 2.0f; ///< Radius of the BoxColUnit

    /// @brief Scale when it starts exploding. Smaller than before the explosion
    static constexpr f32 INITIAL_EXPLOSION_SCALE = RADIUS_AABB / 940.0f * 0.5f;

    /// @brief Rate at which the explosion scale increases per frame, after an initial wait period
    static constexpr f32 SCALE_DELTA = INITIAL_EXPLOSION_SCALE / 25.0f;

    static constexpr f32 INITIAL_SPEED = 400.0f;       ///< Initial speed of the fireball
    static constexpr f32 INITIAL_ANGULAR_SPEED = 3.0f; ///< Initial angular speed in degrees
    static constexpr f32 INITIAL_Y_SPEED = -30.0f;     ///< Initial vertical speed upon firing
};

} // namespace Kinoko::Field
