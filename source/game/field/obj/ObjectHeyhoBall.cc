#include "ObjectHeyhoBall.hh"

#include "game/field/ObjectDirector.hh"

namespace Kinoko::Field {

/// @addr{0x806D05F0}
/// @copybrief ObjectBase::init()
/// @details Starts the cannonball in the intangible state. Initializes some internal state
/// variables, including resizing the cannonball's @ref BoxColUnit to a radius of `1500.0f`.
/// Computes the ratio between the blast radius and the cannonball's radius. With this ratio,
/// computes @ref m_scaleChangeRate so the blast radius can decay after the explosion.
/// @note @ref m_scaleChangeRate is normally computed in 0x806D0D84 which runs every frame the
/// cannonball begins exploding, however this value never changes. For Kinoko, we take creative
/// liberty to move the calculation here to avoid frequent re-computation.
void ObjectHeyhoBall::init() {
    m_nextStateId = 0;
    m_shipPos.setZero();
    m_workingPos = pos();
    m_intensity = ExplosionIntensity::ExplosionLoseItem;

    resize(INIT_BLAST_RADIUS, 0.0f);

    const auto &flowTable = ObjectDirector::Instance()->flowTable();
    m_blastRadiusRatio = INIT_BLAST_RADIUS /
            static_cast<f32>(parse<s16>(flowTable.set(flowTable.slot(id()))->params.sphere.radius));
    m_scaleChangeRate = (1.2f * m_blastRadiusRatio - 1.0f) / 40.0f / 40.0f;
}

/// @addr{0x806D0880}
/// @copybrief ObjectCollidable::onCollision()
/// @param hitDepth The depth of the collision between the kart and the object
/// @return @ref Kart::Reaction::ExplosionLoseItem if the explosion recently occurred, @ref
/// Kart::Reaction::SpinTwice if the explosion has dissipated a bit, otherwise @ref
/// Kart::Reaction::Wall if the explosion has not yet occurred.
/// @details References the explosion intensity to determine what reaction to apply
Kart::Reaction ObjectHeyhoBall::onCollision(Kart::KartObject * /*kartObj*/,
        Kart::Reaction /*reactionOnKart*/, Kart::Reaction /*reactionOnObj*/,
        EGG::Vector3f &hitDepth) {
    if (m_currentStateId == 3) {
        if (m_intensity == ExplosionIntensity::ExplosionLoseItem) {
            return Kart::Reaction::ExplosionLoseItem;
        }

        hitDepth.setZero();
        return Kart::Reaction::SpinTwice;
    }

    return Kart::Reaction::Wall;
}

/// @addr{0x806D10A4}
/// @brief Init callback function called by the managing @ref ObjectHeyhoShip
/// @details Caches the ship's position to @ref m_shipPos. Computes the XZ-plane direction from the
/// ship to the cannonball's target landing position. Derives the cannonball's @ref m_xzSpeed and
/// @ref m_initYSpeed such that it takes @ref m_airtime frames to land after being launched.
/// Calculates the vertical distance between the cannonball's landing position in the floor and the
/// ship's position. Finally, derives the required @ref m_initYSpeed such that the cannonball
void ObjectHeyhoBall::initProjectile(const EGG::Vector3f &pos) {
    m_shipPos = pos;
    m_xzDir = m_initPos - m_shipPos;
    m_xzDir.y = 0.0f;
    m_xzDir.normalise2();

    EGG::Vector2f xzDist = EGG::Vector2f(m_shipPos.x - m_initPos.x, m_shipPos.z - m_initPos.z);
    m_xzSpeed = EGG::Mathf::sqrt(xzDist.dot()) / m_airtime;
    f32 yDist = m_initPos.y + -BALL_RADIUS - m_shipPos.y;
    m_initYSpeed = yDist / m_airtime + 0.5f * 4.0f * m_airtime;
}

/// @addr{0x806D0AD8}
/// @brief Runs every frame that the cannonball is falling
/// @details Applies a gravitational force of `4.0f` and compares the resulting height with the
/// cannonball's target landing height. If the resulting height is above the target landing height,
/// sets that height to the Y-component of @ref m_workingPos and applies @ref m_xzSpeed to the X and
/// Z components of @ref m_workingPos. If the resulting height is below the target landing height,
/// then transitions the cannonball to the blinking state. To dampen the cannonball's position
/// snapping into the ground, updates @ref m_workingPos to the average between the current @ref
/// m_workingPos and the cannonball's target landing position in the floor.
void ObjectHeyhoBall::calcFalling() {
    constexpr f32 GRAVITY = 4.0f;

    f32 currentHeight =
            m_workingPos.y + (m_initYSpeed - GRAVITY * static_cast<f32>(m_currentFrame));
    if (currentHeight > m_initPos.y + -BALL_RADIUS) {
        m_workingPos.y = currentHeight;
        m_workingPos.x += m_xzDir.x * m_xzSpeed;
        m_workingPos.z += m_xzDir.z * m_xzSpeed;
    } else {
        m_nextStateId = 2;
        m_workingPos = (m_workingPos + m_initPos + EGG::Vector3f::ey * -BALL_RADIUS) * 0.5f;
    }
}

/// @addr{0x806D0F24}
/// @brief Runs every frame the cannonball is exploding.
/// @details Scales the explosion sphere over 46 frames using a capped downward parabola. Determines
/// the @ref m_intensity of the explosion based off of the time elapsed, so that @ref onCollision
/// returns the appropriate reaction.  If the explosion state has elapsed 46 frames, then resets the
/// object's scale, unregisters the object's @ref BoxColUnit to disable collisions, and transitions
/// the cannonball to the intangible state.
void ObjectHeyhoBall::calcExploding() {
    constexpr u32 EXPLODE_FRAMES = 46;

    /// Colliding after this frame does not cause player to launch upwards and lose item
    constexpr u32 SPIN_FRAME = 32;

    if (m_currentFrame >= EXPLODE_FRAMES) {
        calcFinishedExplodingScale();

        if (getUnit()) {
            unregisterCollision();
        }

        m_nextStateId = 0;
        m_intensity = ExplosionIntensity::ExplosionLoseItem;
    } else {
        calcExplodingScale();
        m_intensity = m_currentFrame >= SPIN_FRAME ? ExplosionIntensity::SpinSomeSpeed :
                                                     ExplosionIntensity::ExplosionLoseItem;
    }
}

} // namespace Kinoko::Field
