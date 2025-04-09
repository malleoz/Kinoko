
#include "ObjectPillar.hh"

#include "game/field/ObjectDirector.hh"

namespace Field {

/// @addr{Inlined in 0x807FED80}
ObjectPillarBase::ObjectPillarBase(const System::MapdataGeoObj &params) : ObjectKCL(params) {}

/// @addr{0x807FFAA0}
ObjectPillarBase::~ObjectPillarBase() = default;

/// @addr{0x807FEB68}
ObjectPillarC::ObjectPillarC(const System::MapdataGeoObj &params) : ObjectCollidable(params) {
    // Assume 150cc
    m_fallStart = static_cast<u32>(params.setting(0));
}

/// @addr{0x807FFAE0}
ObjectPillarC::~ObjectPillarC() = default;

/// @addr{0x807FEC30}
void ObjectPillarC::calcCollisionTransform() {
    if (!m_collision) {
        return;
    }

    EGG::Matrix34f mat = EGG::Matrix34f::zero;
    mat.makeT(EGG::Vector3f(0.0f, 2.0f * (1900.0f * m_scale.y) / 3.0f, 0.0f));

    calcTransform();

    mat = m_transform.multiplyTo(mat);
    EGG::Vector3f speed = m_transform.ps_multVector(EGG::Vector3f::ez * 1000.0f);

    m_collision->transform(mat, m_scale, speed);
}

/// @addr0x807FED80}
ObjectPillar::ObjectPillar(const System::MapdataGeoObj &params) : ObjectKCL(params), m_state(0) {
    // Assume 150cc
    m_fallStart = static_cast<u32>(params.setting(0));
    m_fallRotation = static_cast<f32>(params.setting(1)) * DEG2RAD;
    m_startRot = m_rot;
    m_base = new ObjectPillarBase(params);
    m_collidable = new ObjectPillarC(params);

    m_base->load();
    m_collidable->load();

    m_fallFrame = std::numeric_limits<s32>::max();
}

/// @addr{0x807FFA34}
ObjectPillar::~ObjectPillar() {
    delete m_base;
    delete m_collidable;
}

/// @addr{0x807FEFD8}
void ObjectPillar::init() {
    ObjectBase::init();

    m_collidable->disableCollision();
    m_startRot = m_rot;
    disableCollision();
}

/// @addr{0x807FF17C}
void ObjectPillar::calc() {
    u32 time = System::RaceManager::Instance()->timer();

    if (m_state == 0 && time < m_fallStart) {
        // The pillar is upright.
        m_collidable->enableCollision();
    } else if (m_state == 0 && time == m_fallStart) {
        // The pillar has now started to fall.
        m_state = 1;
    } else if (m_state == 1) {
        // The pillar is currently falling.
        f32 rot;

        if (m_fallFrame < static_cast<s32>(time)) {
            rot = m_fallRotation;
        } else {
            time -= m_fallStart;
            rot = std::min(m_fallRotation,
                    m_startRot.x + 1e-07f * static_cast<f32>(time * time * time));
        }

        if (rot < m_fallRotation) {
            m_flags |= 4;
            m_transform = getUpdatedMatrix(0);
            m_pos = m_transform.base(3);

            m_collidable->setFlag(4);
            m_collidable->setTransform(m_transform);
            m_collidable->setPos(m_pos);

            m_flags |= 2;
            m_rot = EGG::Vector3f(rot, m_startRot.y, m_startRot.z);
        } else {
            // The pillar has finished falling.
            // We can now drive on top of the base and on the pillar itself.
            m_state = 2;

            m_collidable->disableCollision();
            enableCollision();
            m_fallFrame = time;
        }

    } else {
        m_state = 2;
    }
}

/// @addr{0x807FF83C}
const EGG::Matrix34f &ObjectPillar::getUpdatedMatrix(u32 timeOffset) {
    EGG::Vector3f rot;
    rot.y = m_startRot.y;
    rot.z = m_startRot.z;

    s32 time = System::RaceManager::Instance()->timer() - timeOffset;

    if (m_fallFrame < time) {
        rot.x = m_fallRotation;
    } else {
        time -= m_fallStart;
        rot.x = std::min(m_fallRotation,
                1e-7f * m_startRot.x + static_cast<f32>(time * time * time));
    }

    m_rtMtx.makeRT(rot, m_pos);

    return m_rtMtx;
}

} // namespace Field
