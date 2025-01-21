#include "ObjectPenguin.hh"

namespace Field {

/// @addr{0x80775624}
ObjectPenguin::ObjectPenguin(const System::MapdataGeoObj &params) : ObjectCollidable(params) {}

/// @addr{0x80775670}
ObjectPenguin::~ObjectPenguin() = default;

/// @addr{0x807756B0}
void ObjectPenguin::init() {
    m_railInterpolator->init(0.0f, 0);
    m_state = 0;
    m_b0 = EGG::Vector3f::ez;
}

/// @addr{0x80775764}
void ObjectPenguin::calc() {
    if (m_state == 0) {
        calcWalk();
    } else if (m_state == 5) {
        ; // TODO
    }
}

/// @addr{0x80777324}
u32 ObjectPenguin::loadFlags() const {
    return 1;
}

/// @addr{0x807757A0}
/// @brief This is a virtual function but it's never overridden.
void ObjectPenguin::calcWalk() {
    m_railInterpolator->calc();
    calcPos();
    calcRot();
}

/// @addr{0x80775C2C}
/// @brief This is a virtual function but it's never overridden.
void ObjectPenguin::calcPos() {
    m_flags |= 1;
    m_pos = m_railInterpolator->curPos();
}

/// @addr{0x80775B1C}
/// @brief This is a virtual function but it's never overridden.
void ObjectPenguin::calcRot() { // penguin in question at 4782 is 80f97e5c.
    m_rot = FUN_80775BA4(0.2f, m_rot, m_railInterpolator->curTangentDir()); // tangentDir incorrect, for railInterpolator addr 0x80f99e98 in-game having railIdx = 1
    m_rot.normalise();
    FUN_808218B0(m_rot); // m_rot wrong 4782 for 2nd penguin
}

/// @addr{0x80775BA4}
EGG::Vector3f ObjectPenguin::FUN_80775BA4(f32 scale, const EGG::Vector3f &v0,
        const EGG::Vector3f &v1) {
    return v0 + (v1 - v0) * scale;
}

/// @addr{0x8077708C}
ObjectPenguinM::ObjectPenguinM(const System::MapdataGeoObj &params) : ObjectPenguin(params) {}

/// @addr{0x807774A4}
ObjectPenguinM::~ObjectPenguinM() = default;

} // namespace Field
