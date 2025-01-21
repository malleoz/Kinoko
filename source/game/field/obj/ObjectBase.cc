#include "ObjectBase.hh"

#include <game/system/CourseMap.hh>
#include <game/system/map/MapdataPointInfo.hh>

#include <egg/math/Math.hh>

namespace Field {

/// @addr{0x8081F828}
ObjectBase::ObjectBase(const System::MapdataGeoObj &params)
    : m_id(static_cast<ObjectId>(params.id())), m_flags(0x3), m_pos(params.pos()),
      m_rot(params.rot() * DEG2RAD), m_scale(params.scale()), m_transform(EGG::Matrix34f::ident),
      m_globalObj(params) {}

/// @addr{0x8067E3C4}
ObjectBase::~ObjectBase() = default;

/// @addr{0x808217B8}
void ObjectBase::calcModel() {
    calcTransform();
}

/// @addr{0x80820980}
void ObjectBase::loadRail() {
    s16 pathId = m_globalObj.pathId();

    if (pathId == -1) {
        return;
    }

    auto *point = System::CourseMap::Instance()->getPointInfo(pathId);
    f32 speed = static_cast<f32>(m_globalObj.setting(0));

    if (point->setting(0) == 0) {
        m_railInterpolator = new RailLinearInterpolator(speed, pathId);
    } else {
        m_railInterpolator = new RailSmoothInterpolator(speed, pathId);
    }
}

/// @addr{0x806BF434}
u32 ObjectBase::loadFlags() const {
    // TODO: This references LOD to determine load flags
    return 0;
}

/// @addr{0x80681598}
const EGG::Vector3f &ObjectBase::getPosition() const {
    return m_pos;
}

/// @addr{0x8080BDC0}
f32 ObjectBase::getCollisionRadius() const {
    constexpr f32 BASE_RADIUS = 100.0f;

    return BASE_RADIUS;
}

/// @addr{0x80572574}
ObjectId ObjectBase::id() const {
    return m_id;
}

/// @addr{0x80821640}
void ObjectBase::calcTransform() {
    if (m_flags & 2) {
        m_transform.makeRT(m_rot, m_pos);
        m_flags &= ~0x3;
    } else if (m_flags & 1) {
        m_transform.setBase(3, m_pos);
        m_flags |= 4;
    }
}

/// @addr{0x80821910}
void ObjectBase::setMatrixTangentTo(const EGG::Vector3f &up, const EGG::Vector3f &tangent) {
    m_flags |= 4;
    m_transform.setRotTangentHorizontal(up, tangent);
    m_transform.setBase(3, m_pos);
}

/// @addr{0x808218B0}
void ObjectBase::FUN_808218B0(const EGG::Vector3f &v) {
    m_flags |= 4;
    m_transform = FUN_806B3CA4(v);
    m_transform.setBase(3, m_pos);
}

/// @addr{0x806B3CA4}
EGG::Matrix34f ObjectBase::FUN_806B3CA4(const EGG::Vector3f &v) { // v wrong
    EGG::Vector3f local_20 = v;

    if (EGG::Mathf::abs(local_20.y) < 0.001f) {
        local_20.y = 0.001f;
    }

    EGG::Vector3f local_2c = v;
    local_2c.y = 0.0f;
    local_2c.normalise2();

    EGG::Vector3f local_38 = local_2c.cross(local_20);

    if (local_20.y > 0.0f) {
        local_38 = -local_38;
    }

    local_38.normalise2();

    EGG::Matrix34f mat;
    mat.setBase(3, EGG::Vector3f::zero);
    mat.setBase(0, local_38);
    mat.setBase(1, local_20.cross(local_38));
    mat.setBase(2, local_20);

    return mat; // 9th call to this func in base game
}

} // namespace Field
