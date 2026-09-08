#include "ObjectHeyhoBall.hh"

#include "game/field/ObjectDirector.hh"

namespace Kinoko::Field {

/// @addr{0x806D02C4}
/// @brief Constructor
/// @param params The parameters used to initialize the object
ObjectHeyhoBall::ObjectHeyhoBall(const System::MapdataGeoObj &params)
    : ObjectProjectile(params),
      StateManager(this, STATE_ENTRIES),
      m_airtime(static_cast<f32>(params.setting(1))),
      m_initPos(params.pos()) {
    registerManagedObject();
}

/// @addr{0x806D1820}
/// @brief Default virtual destructor
ObjectHeyhoBall::~ObjectHeyhoBall() = default;

/// @addr{0x806D05F0}
/// @copybrief ObjectBase::init()
void ObjectHeyhoBall::init() {
    m_nextStateId = 0;
    m_shipPos.setZero();
    m_workingPos = pos();
    m_intensity = ExplosionIntensity::ExplosionLoseItem;

    resize(INIT_BLAST_RADIUS, 0.0f);

    const auto &flowTable = ObjectDirector::Instance()->flowTable();
    m_blastRadiusRatio = INIT_BLAST_RADIUS /
            static_cast<f32>(parse<s16>(flowTable.set(flowTable.slot(id()))->params.sphere.radius));
}

/// @addr{0x806D0880}
/// @brief Called when a collision occurs between a kart and this object
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
/// @brief Callback function called by the managing @ref ObjectHeyhoShip.
void ObjectHeyhoBall::initProjectile(const EGG::Vector3f &pos) {
    m_shipPos = pos;
    m_xzDir = (m_initPos + EGG::Vector3f::ey * -BALL_RADIUS) - m_shipPos;
    m_xzDir.y = 0.0f;
    m_xzDir.normalise2();

    EGG::Vector2f xzDist = EGG::Vector2f(m_shipPos.x - m_initPos.x, m_shipPos.z - m_initPos.z);
    m_xzSpeed = EGG::Mathf::sqrt(xzDist.dot()) / m_airtime;
    m_yDist = m_initPos.y + -BALL_RADIUS - m_shipPos.y;
    m_initYSpeed = m_yDist / m_airtime + 0.5f * 4.0f * m_airtime;
}

/// @addr{0x806D0AD8}
/// @brief Runs every frame that the cannonball is falling
void ObjectHeyhoBall::calcFalling() {
    f32 currentHeight = m_workingPos.y + (m_initYSpeed - 4.0f * static_cast<f32>(m_currentFrame));
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
/// @brief Scales the explosion sphere over 46 frames using a capped downward parabola
///
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
