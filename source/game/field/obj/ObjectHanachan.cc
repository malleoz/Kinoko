#include "ObjectHanachan.hh"

#include "game/field/ObjectDirector.hh"

namespace Kinoko::Field {

/// @addr{0x806F2FE8}
HanachanChainManager::HanachanChainManager(const std::span<const f32> &linkDistances) {
    size_t count = linkDistances.size() + 1;

    m_links = owning_span<Field::SphereLink>(count);
    m_links[0].initLinkLen(linkDistances[0]);

    for (size_t i = 1; i < count - 1; ++i) {
        m_links[i].initLinkLen(linkDistances[i]);
        m_links[i - 1].setNext(&m_links[i]);
        m_links[i].setPrev(&m_links[i - 1]);
    }

    auto &last = m_links.back();
    auto &secondToLast = m_links[count - 2];
    last.initLinkLen(0.0f);
    secondToLast.setNext(&last);
    last.setPrev(&secondToLast);
}

/// @addr{0x806F31F4}
HanachanChainManager::~HanachanChainManager() = default;

/// @addr{0x806F49BC}
/// @brief Applies spring force and updates positions for all links in the chain, enforcing
/// constraints when the chain is taut. Also checks for floor collision.
/// @details Runs two iterations of the spring force and position calculations to reduce oscillation
/// and prevent links from stretching past their maximum length.
void HanachanChainManager::calc() {
    for (u32 i = 0; i < 2; ++i) {
        for (auto &link : m_links) {
            link.calc();
        }

        for (auto &link : m_links) {
            link.calcPos();
        }
    }

    for (size_t i = 0; i < m_links.size() - 1; ++i) {
        EGG::Vector3f delta = m_links[i].pos() - m_links[i + 1].pos();
        f32 len = m_links[i].linkLen();

        if (delta.squaredLength() - len * len > 0.0f) {
            calcConstraints();
            break;
        }
    }

    for (auto &link : m_links) {
        link.checkCollision();
    }
}

/// @addr{0x806C7D74}
ObjectHanachanHead::ObjectHanachanHead(const char *name, const EGG::Vector3f &pos,
        const EGG::Vector3f &rot, const EGG::Vector3f &scale)
    : ObjectHanachanPart(name, pos, rot, scale), m_lastPos(EGG::Vector3f::zero) {}

/// @addr{0x806CCB94}
ObjectHanachanHead::~ObjectHanachanHead() = default;

/// @addr{0x806C8450}
/// @details Applies a forward and upwards offset so that the collision sphere sits above and in
/// front of the head's origin.
void ObjectHanachanHead::calcCollisionTransform() {
    calcTransform();

    EGG::Matrix34f trans = transform();
    EGG::Vector3f scaledUp = trans.base(1) * 330.0f * scale().y;
    EGG::Vector3f scaledForward = trans.base(2) * 100.0f * scale().y;
    trans.setBase(3, pos() + scaledUp + scaledForward);
    EGG::Vector3f speed = pos() - m_lastPos;

    m_collision->transform(trans, scale(), speed);
    m_lastPos = pos();
}

ObjectHanachanBody::ObjectHanachanBody(const System::MapdataGeoObj &params, const char *mdlName)
    : ObjectHanachanPart(params), m_mdlName(mdlName), m_lastSegment(false),
      m_lastPos(EGG::Vector3f::zero) {}

ObjectHanachanBody::ObjectHanachanBody(const char *name, const EGG::Vector3f &pos,
        const EGG::Vector3f &rot, const EGG::Vector3f &scale, const char *mdlName)
    : ObjectHanachanPart(name, pos, rot, scale), m_mdlName(mdlName), m_lastSegment(false),
      m_lastPos(EGG::Vector3f::zero) {}

/// @addr{0x806CCAD8}
ObjectHanachanBody::~ObjectHanachanBody() = default;

/// @addr{0x806C8908}
/// @details Uses base class implementation for all segments except the last. For the last segment,
/// applies a forward offset so that the collision sphere sits in front of the body part's origin.
void ObjectHanachanBody::calcCollisionTransform() {
    if (!m_lastSegment) {
        ObjectCollidable::calcCollisionTransform();
        return;
    }

    EGG::Matrix34f trans = transform();
    trans.setBase(3, pos() + trans.base(2) * 80.0f * scale().y);
    EGG::Vector3f posDelta = pos() - m_lastPos;

    m_collision->transform(trans, scale(), posDelta);
    m_lastPos = pos();
}

/// @addr{0x806C8A5C}
ObjectHanachan::ObjectHanachan(const System::MapdataGeoObj &params)
    : ObjectCollidable(params), StateManager(this, STATE_ENTRIES), m_chain(BODY_PART_DISTANCES),
      m_walkSpeed(static_cast<f32>(static_cast<s16>(params.setting(0)))) {
    constexpr f32 SCALE = 3.0f;
    constexpr EGG::Vector3f SCALE_VEC = EGG::Vector3f(SCALE, SCALE, SCALE);

    auto *&head = headPart();
    head = EGG::egg_new<ObjectHanachanHead>("BossHanachanHead", EGG::Vector3f::zero,
            EGG::Vector3f::ez, EGG::Vector3f::ez);
    head->setScale(SCALE_VEC);

    const auto &mdlNames = ObjectHanachanBody::MDL_NAMES;
    size_t mdlNameCount = ObjectHanachanBody::MDL_NAMES.size();
    auto parts = bodyParts();
    for (size_t i = 0; i < parts.size(); ++i) {
        auto *&part = parts[i];
        part = EGG::egg_new<ObjectHanachanBody>(params, mdlNames[i % mdlNameCount]);
        part->setScale(SCALE_VEC);
    }

    head->load();

    const auto &flowTable = ObjectDirector::Instance()->flowTable();
    for (size_t i = 0; i < parts.size(); ++i) {
        auto *&part = parts[i];
        part->load();

        const auto *collisionSet = flowTable.set(flowTable.slot(part->id()));
        f32 radius = SCALE * static_cast<f32>(parse<s16>(collisionSet->params.sphere.radius));
        part->resize(radius, 0.0f);
    }

    m_partDisplacement[0] = 0.0f;
    for (size_t i = 1; i < m_partDisplacement.size(); ++i) {
        m_partDisplacement[i] = m_partDisplacement[i - 1] + BODY_PART_DISTANCES[i - 1];
    }
}

/// @addr{0x806C9598}
ObjectHanachan::~ObjectHanachan() = default;

/// @addr{0x806C9630}
void ObjectHanachan::init() {
    initRail();
    initBody();

    m_still = false;
    m_swayAmplitude = INIT_SWAY_AMPLITUDE;
    m_leftMisalignFrame = 0;
    m_prevRailTangent = EGG::Vector3f::ez;
    m_railAlignment = RailAlignment::Unknown;
    m_prevRailAlignment = RailAlignment::Unknown;

    reinterpret_cast<ObjectHanachanBody *>(m_parts.back())->m_lastSegment = true;

    initChain();
}

/// @addr{0x806C9D38}
/// @brief Runs every frame when the Wiggler is walking along its rail
void ObjectHanachan::calcWalk() {
    if (m_still) {
        m_railInterpolator->setCurrVel(0.0f);
        m_nextStateId = 1;
    }

    m_prevRailAlignment = m_railAlignment;
    m_railAlignment = calcRailAlignment();

    clearChain();
    calcRailAlignmentMotion();
    m_chain.calc();

    m_swayAmplitude = INIT_SWAY_AMPLITUDE;
}

/// @addr{0x806C9F98}
/// @brief Runs every frame when the Wiggler is standing still
void ObjectHanachan::calcWait() {
    if (shouldStartMoving()) {
        m_nextStateId = 0;
    }

    clearChain();
    calcSway();
    m_chain.calc();
}

/// @addr{0x806CA2F0}
/// @brief Updates the transforms of the Wiggler's body parts based on the chain link positions
void ObjectHanachan::calcBody() {
    auto *&head = headPart();
    head->calcTransform();
    EGG::Vector3f forward = head->transform().base(2);
    EGG::Vector3f dir = Interpolate(0.1f, forward, m_railInterpolator->curTangentDir());
    head->calcTransformFromUpAndTangent(m_chain.pos(0), m_chain.up(0), dir);

    auto parts = bodyParts();
    for (size_t i = 0; i < parts.size(); ++i) {
        const EGG::Vector3f &pos = m_chain.pos(i + 1);
        dir = m_chain.pos(i) - pos;
        dir.normalise2();
        parts[i]->calcTransformFromUpAndTangent(pos, m_chain.up(i + 1), dir);
    }
}

/// @addr{0x806CA72C}
/// @brief Initializes the positions of the Wiggler's body parts based on the initial rail position
void ObjectHanachan::initBody() {
    headPart()->setPos(m_railInterpolator->curPos());

    const EGG::Vector3f &curTanDir = m_railInterpolator->curTangentDir();
    for (size_t i = 1; i < m_parts.size(); ++i) {
        m_parts[i]->setPos(m_parts[i - 1]->pos() - curTanDir * BODY_PART_DISTANCES[i - 1]);
        m_parts[i]->setMatrixTangentTo(EGG::Vector3f::ey, curTanDir);
    }
}

/// @addr{0x806CAB5C}
/// @brief If the wiggler has moved out of alignment of the rail, then applies a lateral adjustment
/// to the wiggler's body part segments.
void ObjectHanachan::calcRailAlignmentMotion() {
    constexpr u16 MISALIGNMENT_CORRECTION_DURATION = 80;

    bool becameUnaligned = m_prevRailAlignment == RailAlignment::Aligned &&
            m_railAlignment != RailAlignment::Aligned;

    if (becameUnaligned && m_railAlignment == RailAlignment::MisalignedLeft) {
        m_leftMisalignFrame = m_currentFrame;
    }

    if (m_leftMisalignFrame != 0 && m_currentFrame >= m_leftMisalignFrame &&
            m_currentFrame <
                    static_cast<u32>(m_leftMisalignFrame + MISALIGNMENT_CORRECTION_DURATION)) {
        calcFastLateralMotion(m_currentFrame - m_leftMisalignFrame);
    } else {
        calcDefaultLateralMotion();
    }
}

/// @addr{0x806CB67C}
/// @brief Updates the sway amplitude and calculates the lateral sway motion
void ObjectHanachan::calcSway() {
    if (m_swayAmplitude >= 0.0f) {
        m_swayAmplitude -= 0.25f;
    } else {
        m_swayAmplitude += 0.25f;
    }

    if (EGG::Mathf::abs(m_swayAmplitude) <= 1.0f) {
        m_swayAmplitude = 0.0f;
    }

    calcDefaultLateralMotion();
}

/// @addr{0x806CACD0}
/// @brief Calculates sinusoidal lateral sway motion for the Wiggler's body parts
void ObjectHanachan::calcLateralMotion(f32 amplitude, f32 period, f32 wavelength, s16 frame) {
    f32 velAmplitude = F_TAU * amplitude / period;

    auto parts = bodyParts();
    for (size_t i = 0; i < parts.size(); ++i) {
        auto *&part = parts[i];
        f32 phase =
                F_TAU * (static_cast<f32>(frame) / period - m_partDisplacement[i + 1] / wavelength);
        part->calcTransform();
        EGG::Vector3f right = RotateXZByYaw(HALF_PI, part->transform().base(2));
        EGG::Vector3f posOffset = right * (amplitude * EGG::Mathf::SinFIdx(RAD2FIDX * phase));
        m_chain.setPos(i + 1, m_parts[i + 1]->pos() + posOffset);
        m_chain.setVel(i + 1, right * (velAmplitude * EGG::Mathf::CosFIdx(RAD2FIDX * phase)));
    }
}

/// @addr{0x806CAFB8}
/// @brief Calculates whether the Wiggle is misaligned with the rail
ObjectHanachan::RailAlignment ObjectHanachan::calcRailAlignment() const {
    constexpr f32 EPSILON = 0.9995f;

    EGG::Vector3f curTanDir = m_railInterpolator->curTangentDir();
    EGG::Vector2f railTan = EGG::Vector2f(curTanDir.x, curTanDir.z);
    EGG::Vector2f prevTan = EGG::Vector2f(m_prevRailTangent.x, m_prevRailTangent.z);
    railTan.normalise2();
    prevTan.normalise2();

    if (railTan.dot(prevTan) >= EPSILON) {
        return RailAlignment::Aligned;
    }

    return prevTan.cross(railTan) < 0.0f ? RailAlignment::MisalignedLeft :
                                           RailAlignment::MisalignedRight;
}

} // namespace Kinoko::Field
