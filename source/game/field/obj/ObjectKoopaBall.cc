#include "ObjectKoopaBall.hh"

#include "game/field/CollisionDirector.hh"

#include "game/kart/KartCollide.hh"

namespace Kinoko::Field {

/// @addr{0x80770384}
/// @brief Constructor
/// @param params The parameters used to initialize the object
ObjectKoopaBall::ObjectKoopaBall(const System::MapdataGeoObj &params)
    : ObjectCollidable(params),
      m_vel(EGG::Vector3f::zero) {}

/// @addr{0x80771F70}
/// @brief Default virtual destructor
ObjectKoopaBall::~ObjectKoopaBall() {
    EGG::egg_delete(m_bombCoreDrawMdl);
}

/// @addr{0x807703D0}
/// @copybrief ObjectBase::init()
/// @details Initializes the rail velocity, position, and collision scale
void ObjectKoopaBall::init() {
    m_state = State::Intangible;
    initCooldownTimer();

    m_railInterpolator->init(0.0f, 0);
    m_railInterpolator->setCurrVel(INITIAL_VELOCITY);
    m_railInterpolator->calc();

    m_initPosY = m_railInterpolator->curPos().y;
    setPos(EGG::Vector3f(pos().x, m_initPosY, pos().z));

    EGG::Vector3f curTanDirNorm = m_railInterpolator->curTangentDir();
    curTanDirNorm.normalise();
    setMatrixFromOrthonormalBasisAndPos(curTanDirNorm);
    calcTransform();

    m_angleRad = 0.0f;
    m_angSpeed = INITIAL_ANGULAR_SPEED;
    m_vel.y = INITIAL_Y_VEL;

    m_bombCoreDrawMdl = EGG::egg_new<Render::DrawMdl>();

    auto *resMgr = System::ResourceManager::Instance();
    std::span<const u8> file = resMgr->getFile("bombCore.brres", System::ArchiveId::Core);
    ASSERT(!file.empty());
    Abstract::g3d::ResFile resFile = Abstract::g3d::ResFile(file.data());

    m_bombCoreDrawMdl->linkAnims(0, &resFile, "bombCore", Render::AnmType::Chr);
    auto *anmMgr = m_bombCoreDrawMdl->anmMgr();
    anmMgr->playAnim(0.0f, 1.0f, 0);
    m_animFramecount = anmMgr->activeAnim(Render::AnmType::Chr)->frameCount();
    m_explodeTimer = -1;

    EGG::Matrix34f mat;
    mat.makeR(rot());

    resize(RADIUS_AABB, 0.0f);
}

/// @addr{0x80770ADC}
/// @copybrief ObjectBase::calc()
/// @details Calculates collision state-specific behavior, decrements the cycle timer, and updates
/// the velocity based on the rail interpolator.
void ObjectKoopaBall::calc() {
    switch (m_state) {
    case State::Tangible:
        calcTangible();
        break;
    case State::Intangible:
        calcIntangible();
        break;
    case State::Exploding:
        calcExploding();
        break;
    default:
        break;
    }

    --m_cooldownTimer;

    EGG::Vector3f railVel = m_railInterpolator->curTangentDir() * m_railInterpolator->getCurrVel();
    m_vel.x = railVel.x;
    m_vel.z = railVel.z;
}

/// @addr{0x80771BF4}
/// @copybrief ObjectCollidable::onCollision()
/// @param reactionOnKart The reaction that should be applied to the kart upon collision
/// @return @ref Kart::Reaction::ExplosionLoseItem if the fireball is exploding, otherwise @ref
/// Kart::Reaction::Sideways.
/// @details Throws the player upwards and removes items if the koopa ball is exploding, otherwise
/// the player will flip sideways without losing their item.
Kart::Reaction ObjectKoopaBall::onCollision(Kart::KartObject * /*kartObj*/,
        Kart::Reaction reactionOnKart, Kart::Reaction /*reactionOnObj*/,
        EGG::Vector3f & /*hitDepth*/) {
    return m_explodeTimer > 0 ? Kart::Reaction::ExplosionLoseItem : reactionOnKart;
}

/// @addr{0x80770F4C}
/// @brief Runs every frame that the fireball can collide with karts
/// @details Updates the position based off the rail interpolator. Checks if the explosion should
/// start. Checks for floor collision, updates the rotation, then sets the transformation matrix.
void ObjectKoopaBall::calcTangible() {
    constexpr f32 GRAVITY = 2.0f;

    auto railStatus = m_railInterpolator->calc();

    switch (railStatus) {
    case RailInterpolator::Status::SegmentEnd:
        calcSlowdown();
        break;
    case RailInterpolator::Status::ChangingDirection: {
        m_state = State::Exploding;
        setExplosionScale();
        m_explodeTimer = m_animFramecount;
    } break;
    default:
        break;
    }

    m_vel.y = m_vel.y - GRAVITY;
    const auto &railPos = m_railInterpolator->curPos();
    setPos(EGG::Vector3f(railPos.x, m_vel.y + pos().y, railPos.z));

    checkSphereFull();
    calcRot();
}

/// @addr{0x80771324}
/// @brief Runs every frame that the fireball is exploding
/// @details If the explosion still has EXPLOSION_EXPAND_FRAME frames remaining, the scale of the
/// fireball will increase. Once there are only EXPLODE_COLLISION_DURATION frames remaining for the
/// explosion, then collision is disabled. Once the explosion has finished, the scale, position, and
/// collision are reset.
void ObjectKoopaBall::calcExploding() {
    constexpr s32 EXPLODE_COLLISION_DURATION = 20;
    constexpr s32 EXPLOSION_EXPAND_FRAME = 30;
    constexpr f32 INIT_FACTOR = 1.01f;
    constexpr EGG::Vector3f INIT_SCALE = EGG::Vector3f(INIT_FACTOR, INIT_FACTOR, INIT_FACTOR);

    if (m_explodeTimer > EXPLOSION_EXPAND_FRAME) {
        m_curScale += SCALE_DELTA;
        setScale(m_curScale);
    }

    if (m_explodeTimer == EXPLODE_COLLISION_DURATION) {
        disableCollision();
    }

    if (--m_explodeTimer == 0) {
        resetScale();
        m_railInterpolator->init(0.0f, 0);
        const auto &railPos = m_railInterpolator->curPos();
        setPos(EGG::Vector3f(railPos.x, m_initPosY, railPos.z));
        enableCollision();
        m_state = State::Intangible;
    }
}

/// @addr{0x80771248}
/// @brief Runs every frame that the fireball is intangible
/// @details Once the cooldown timer has expired, the fireball will become tangible and reinitialize
/// its velocity, angular speed, and cooldown timer.
void ObjectKoopaBall::calcIntangible() {
    constexpr s32 COOLDOWN_FRAMES = 210;

    if (m_cooldownTimer >= 0) {
        return;
    }

    m_state = State::Tangible;
    m_railInterpolator->setCurrVel(INITIAL_VELOCITY);
    m_angSpeed = INITIAL_ANGULAR_SPEED;
    m_vel.y = INITIAL_Y_VEL;
    m_cooldownTimer = COOLDOWN_FRAMES;
}

/// @addr{0x8077151C}
/// @brief Updates the ball's angular rotation and transformation matrix
void ObjectKoopaBall::calcRot() {
    m_angleRad += -m_angSpeed * DEG2RAD;

    EGG::Matrix34f mat = EGG::Matrix34f::ident;
    mat.makeR(EGG::Vector3f(m_angleRad, 0.0f, 0.0f));
    mat = transform().multiplyTo(mat);
    mat.setBase(3, pos());
    setTransform(mat);
}

/// @addr{0x80771624}
/// @brief Checks if the fireball is colliding with the floor and bounces it upwards if so
void ObjectKoopaBall::checkSphereFull() {
    constexpr EGG::Vector3f POS_OFFSET = EGG::Vector3f(0.0f, -900.0f, 0.0f);
    constexpr f32 RADIUS = 100.0f;
    constexpr f32 BOUNCE_FACTOR = -0.4f;
    constexpr f32 MIN_ANG_SPEED = 10.0f;

    CollisionInfo info;
    EGG::Vector3f colPos = pos() + POS_OFFSET;

    if (CollisionDirector::Instance()->checkSphereFull(RADIUS, colPos, EGG::Vector3f::inf,
                KCL_TYPE_FLOOR, &info, nullptr, 0)) {
        m_vel.y *= BOUNCE_FACTOR;
        m_angSpeed = std::max(MIN_ANG_SPEED, m_angSpeed);
        addPos(info.tangentOff);
    }
}

} // namespace Kinoko::Field
