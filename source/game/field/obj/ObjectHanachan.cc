#include "ObjectHanachan.hh"

#include "game/field/ObjectDirector.hh"

namespace Kinoko::Field {

/// @addr{0x806F2FE8}
/// @brief Constructor
/// @param linkDistances The distances between consecutive links in the chain
/// @details Constructs an array of @ref Field::SphereLink objects for each segment in the chain and
/// initializes their link lengths based on the provided distances, as well as setting the links'
/// next and prev pointers.
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

/// @addr{0x806C8450}
/// @copybrief ObjectBase::calcCollisionTransform()
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

/// @addr{0x806C8908}
/// @copybrief ObjectBase::calcCollisionTransform()
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
/// @copydoc ObjectCollidable::ObjectCollidable(const System::MapdataGeoObj &)
/// @details Constructs the @ref HanachanChainManager using body part distances defined by @ref
/// BODY_PART_DISTANCES. Initializes @ref m_walkSpeed based on param setting 1. Constructs the head
/// and body part objects and scales them up to 3x. After loading all objects, resizes all collision
/// spheres (except the head) to reflect the 3x scale. Finally, caches the distance between each
/// body part and the head to @ref m_partDisplacement.
ObjectHanachan::ObjectHanachan(const System::MapdataGeoObj &params)
    : ObjectCollidable(params),
      StateManager(this, STATE_ENTRIES),
      m_chain(BODY_PART_DISTANCES),
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

/// @addr{0x806C9630}
/// @copybrief ObjectBase::init()
/// @details Initializes the Wiggler's rail, body, and chain. Sets the initial state variables,
/// including the sway amplitude, misalignment frame, and rail alignment. Marks the last body
/// segment.
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
/// @details If the Wiggler should stop moving (designated by @ref m_still), then it sets the rail
/// interpolator's speed to zero and transitions to the waiting state. Otherwise, it updates the
/// rail alignment, clears the chain, calculates the rail alignment motion, and updates the chain.
/// Otherwise, it applies lateral motion to the Wiggler and enforces stretch constraints of the body
/// segments. Finally, resets the sway amplitude to @ref INIT_SWAY_AMPLITUDE.
void ObjectHanachan::calcWalk() {
    if (m_still) {
        m_railInterpolator->setSpeed(0.0f);
        m_nextStateId = 1;
    }

    m_prevRailAlignment = m_railAlignment;
    m_railAlignment = calcRailAlignment();

    clearChain();
    calcRailAlignmentMotion();
    m_chain.calc();

    m_swayAmplitude = INIT_SWAY_AMPLITUDE;
}

/// @addr{0x806CA2F0}
/// @brief Updates the transforms of the Wiggler's body parts based on the chain link positions
/// @details Linearly interpolates the head's forward direction towards the current rail tangent
/// direction. All other body parts' transforms are updated to reflect the exact direction between
/// it and the preceding chain link.
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

/// @addr{0x806CAB5C}
/// @brief Applies a lateral adjustment to the wiggler's body part segments if the Wiggler has moved
/// out of alignment of the rail.
/// @details Checks if the Wiggler has become misaligned this frame and caches the current frame to
/// @ref m_leftMisalignFrame if so. If @ref m_leftMisalignFrame is set and fewer than `80` frames
/// have elapsed since then, applies a larger lateral motion via @ref calcFastLateralMotion() with a
/// phase based on the number of frames since the misalignment occurred. Otherwise, applies the
/// default lateral motion via @ref calcDefaultLateralMotion().
/// @note In the base game, this function checks for both left and right misalignments of the rail
/// relative to the Wiggler. However, on Maple Treeway, the Wigglers always move counter-clockwise.
/// Thus, we only need to implement logic to handle @ref RailAlignment::MisalignedLeft.
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

/// @addr{0x806CACD0}
/// @brief Calculates sinusoidal lateral sway motion for the Wiggler's body parts
/// @param amplitude The amplitude of the lateral sway
/// @param period The period of the lateral sway
/// @param wavelength The wavelength of the lateral sway
/// @param frame The current frame
/// @details For each body part \f$i\f$ with displacement \f$d_i\f$ (@ref m_partDisplacement) from
/// the head, the phase is
/// \f[ \phi_i(t) = 2\pi \left(\frac{t}{T} - \frac{d_i}{\lambda}\right) \f]
/// where \f$t\f$ is `frame`, \f$T\f$ is `period`, and \f$\lambda\f$ is `wavelength`. The lateral
/// offset applied to the part's chain link position is
/// \f[ \Delta \vec{p}_i(t) = A \sin(\phi_i(t)) \, \hat{r}_i \f]
/// where \f$A\f$ is `amplitude` and \f$\hat{r}_i\f$ is the part's `right` unit vector. The
/// corresponding link velocity is the time derivative of this offset,
/// \f[
/// \vec{v}_i(t) = \frac{d}{dt} \Delta \vec{p}_i(t) = \frac{2\pi A}{T} \cos(\phi_i(t)) \, \hat{r}_i
/// \f]
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
/// @return The current rail alignment of the Wiggler, which can be `RailAlignment::Aligned`,
/// `RailAlignment::MisalignedLeft`, or `RailAlignment::MisalignedRight`
/// @details Essentially, this function checks if the rail is straight/unchanged from the Wiggler's
/// perspective. If the dot product of the XZ components of the current and previous rail tangents
/// is greater than or equal to `0.9995f`, the rail is considered aligned. Otherwise, the cross
/// product determines if the misalignment is to the left or right.
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
