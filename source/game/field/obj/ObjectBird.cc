#include "ObjectBird.hh"

#include "game/field/CollisionDirector.hh"

#include "game/system/RaceManager.hh"

namespace Kinoko::Field {

/// @addr{0x8077BD80}
/// @brief Constructor
/// @param params The parameters used to initialize the object
/// @details Creates the leader and the follower birds based on the provided parameters. If no
/// follower count is specified, defaults to 5.
ObjectBird::ObjectBird(const System::MapdataGeoObj &params) : ObjectCollidable(params) {
    m_leader = EGG::egg_new<ObjectBirdLeader>(params, this);
    m_leader->load();

    u32 count = params.setting(1);
    if (count == 0) {
        count = 5;
    }

    m_followers = owning_span<ObjectBirdFollower *>(count);

    for (u32 i = 0; i < count; ++i) {
        auto *bird = EGG::egg_new<ObjectBirdFollower>(params, this, i);
        m_followers[i] = bird;
        bird->load();
    }
}

/// @addr{0x8077CDC8}
/// @brief Default virtual destructor
ObjectBird::~ObjectBird() = default;

/// @addr{0x8077BFC8}
/// @copybrief ObjectBase::calc()
/// @details Ensures that follower birds maintain a minimum spacing between each other to avoid
/// collisions. Since this class is registered to the @ref ObjectDirector after the followers, all
/// position fetches reflect their current position this frame.
void ObjectBird::calc() {
    constexpr f32 MIN_SPACING = 300.0f;

    for (u32 i = 0; i < m_followers.size() - 1; ++i) {
        const EGG::Vector3f &firstPos = m_followers[i]->pos();

        for (u32 j = i + 1; j < m_followers.size(); ++j) {
            const EGG::Vector3f &secondPos = m_followers[j]->pos();
            EGG::Vector3f posDelta = firstPos - secondPos;
            f32 len = posDelta.length();

            if (len >= MIN_SPACING) {
                continue;
            }

            posDelta.normalise();
            m_followers[j]->setPos(secondPos - posDelta * (MIN_SPACING - len));
        }
    }
}

/// @addr{0x8077C2F4}
/// @brief Constructor
/// @param params The parameters used to initialize the object
/// @param bird The parent @ref ObjectBird instance that this leader belongs to
ObjectBirdLeader::ObjectBirdLeader(const System::MapdataGeoObj &params, ObjectBird *bird)
    : ObjectCollidable(params),
      m_bird(bird) {}

/// @addr{0x8077C384}
/// @copybrief ObjectBase::init()
/// @details Plays the flying animation whose rate is determined randomly scaled between 0 and the
/// animation framecount. Also initializes and steps the rail interpolator, setting the leader's
/// position accordingly and updating the rail's velocity based off object setting 1.
void ObjectBirdLeader::init() {
    auto *anmMgr = m_drawMdl->anmMgr();
    anmMgr->playAnim(0.0f, 1.0f, 0);
    f32 frameCount = anmMgr->activeAnim(Render::AnmType::Chr)->frameCount();

    auto &rand = System::RaceManager::Instance()->random();
    f32 rate = rand.getF32(static_cast<f32>(frameCount));
    anmMgr->playAnim(rate, 1.0f, 0);

    m_railInterpolator->init(0.0f, 0);
    m_railInterpolator->calc();
    setPos(m_railInterpolator->curPos());
    m_railInterpolator->setCurrVel(static_cast<f32>(m_mapObj->setting(0)));
}

/// @addr{0x8077CC78}
/// @copybrief ObjectBase::loadAnims()
/// @details Loads the flying animation for the bird leader.
void ObjectBirdLeader::loadAnims() {
    std::array<const char *, 1> names = {{
            "flying",
    }};

    std::array<Render::AnmType, 1> types = {{
            Render::AnmType::Chr,
    }};

    linkAnims(names, types);
}

/// @addr{0x8077C580}
/// @brief Constructor
/// @param params The parameters used to initialize the object
/// @param bird The parent @ref ObjectBird instance that this follower belongs to
/// @param idx The index of this follower in the flock
/// @details Sets the bird's base speed based off object setting 1
ObjectBirdFollower::ObjectBirdFollower(const System::MapdataGeoObj &params, ObjectBird *bird,
        u32 idx)
    : ObjectBirdLeader(params, bird),
      m_idx(idx),
      m_baseSpeed(static_cast<f32>(params.setting(0))) {}

/// @addr{0x8077C5E0}
/// @copybrief ObjectBase::init()
/// @details Plays the flying animation whose rate is determined randomly scaled between 0 and the
/// animation framecount. Sets the initial velocity of the bird and randomly offsets its position
/// within a defined range.
void ObjectBirdFollower::init() {
    constexpr f32 POS_DELTA_RANGE = 1000.0f;
    constexpr f32 POS_DELTA_CENTER = 500.0f;

    auto *anmMgr = m_drawMdl->anmMgr();
    anmMgr->playAnim(0.0f, 1.0f, 0);
    f32 frameCount = anmMgr->activeAnim(Render::AnmType::Chr)->frameCount();

    auto &rand = System::RaceManager::Instance()->random();
    f32 rate = rand.getF32(static_cast<f32>(frameCount));
    anmMgr->playAnim(rate, 1.0f, 0);

    m_velocity = EGG::Vector3f::ez * m_baseSpeed;

    f32 z = rand.getF32(POS_DELTA_RANGE) - POS_DELTA_CENTER;
    f32 y = rand.getF32(POS_DELTA_RANGE) - POS_DELTA_CENTER;
    f32 x = rand.getF32(POS_DELTA_RANGE) - POS_DELTA_CENTER;
    EGG::Vector3f delta = EGG::Vector3f(x, y, z);

    setPos(m_bird->leader()->pos() + delta);
}

/// @addr{0x8077C7F0}
/// @copybrief ObjectBase::calc()
/// @details Updates the position of the follower bird based off the other birds in the flock and
/// performs collision checks to prevent it from flying through floors.
/// @note This function is the reason that we have to implement birds for Kinoko: they can cause a
/// transformation matrix update for @ref ObjectCrane due to the collision check.
void ObjectBirdFollower::calc() {
    calcPos();

    CollisionInfo info;

    if (CollisionDirector::Instance()->checkSphereFull(100.0f, pos(), EGG::Vector3f::inf,
                KCL_TYPE_FLOOR, &info, nullptr, 0)) {
        addPos(info.tangentOff);
    }
}

/// @addr{0x8077C8F4}
/// @brief Calculates the new position of the follower bird based on its velocity and the positions
/// of the other birds in the flock.
void ObjectBirdFollower::calcPos() {
    constexpr f32 MAX_SPEED_FACTOR = 1.2f;

    EGG::Vector3f leaderPos = m_bird->leader()->pos();
    const auto &follower = m_bird->followers();

    for (u32 i = 0; i < follower.size(); ++i) {
        if (i != m_idx) {
            leaderPos += follower[i]->pos();
        }
    }

    EGG::Vector3f posDelta = leaderPos * (1.0f / static_cast<f32>(follower.size())) - pos();
    posDelta.normalise();
    posDelta *= 0.5f;

    m_velocity += posDelta;

    if (m_velocity.length() > m_baseSpeed * MAX_SPEED_FACTOR) {
        m_velocity.normalise();
        m_velocity *= m_baseSpeed * MAX_SPEED_FACTOR;
    }

    addPos(m_velocity);
}

} // namespace Kinoko::Field
