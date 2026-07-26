#include "ObjectPenguin.hh"

namespace Kinoko::Field {

/// @addr{0x80775624} @addr{0x8077708C}
ObjectPenguin::ObjectPenguin(const System::MapdataGeoObj &params) : ObjectCollidable(params) {}

/// @addr{0x80775670} @addr{0x807774A4}
ObjectPenguin::~ObjectPenguin() = default;

/// @addr{0x807756B0}
void ObjectPenguin::init() {
    m_railInterpolator->init(0.0f, 0);
    m_state = State::Walk;
    m_basis = EGG::Vector3f::ez;
}

/// @addr{0x80775B1C}
void ObjectPenguin::calcRot() {
    constexpr f32 INTERP_RATE = 0.2f;

    m_basis = Interpolate(INTERP_RATE, m_basis, m_railInterpolator->curTangentDir());
    m_basis.normalise();
    setMatrixFromOrthonormalBasisAndPos(m_basis);
}

/// @addr{0x8077588C}
void ObjectPenguin::enterWalk() {
    m_state = State::Walk;
    calcTransform();
    setRotNoFlag(transform().base(2));
}

/// @addr{0x80775E60}
ObjectPenguinS::ObjectPenguinS(const System::MapdataGeoObj &params) : ObjectPenguin(params) {}

/// @addr{0x80776070}
ObjectPenguinS::~ObjectPenguinS() = default;

/// @addr{0x807760B0}
void ObjectPenguinS::init() {
    initAnmTimer();
    m_railInterpolator->init(0.0f, 0);
    m_basis = m_railInterpolator->curTangentDir();
    m_state = State::Walk;
}

/// @addr{0x80776198}
void ObjectPenguinS::calc() {
    switch (m_state) {
    case State::Walk:
        calcWalk();
        break;
    case State::Dive:
        calcDive();
        break;
    case State::Slider:
        calcSlider();
        break;
    case State::SliderSlow:
        calcWalk();
        calcTransform();
        break;
    case State::StandUp:
        calcStandUp();
        break;
    }
}

void ObjectPenguinS::loadAnims() {
    std::array<const char *, 4> names = {{
            "walk",
            "dive",
            "slider",
            "stand_up",
    }};

    std::array<Render::AnmType, 4> types = {{
            Render::AnmType::Chr,
            Render::AnmType::Chr,
            Render::AnmType::Chr,
            Render::AnmType::Chr,
    }};

    linkAnims(names, types);
}

/// @addr{0x80776498}
/// @brief Runs every frame the penguin is sliding on its belly full speed
void ObjectPenguinS::calcSlider() {
    calcRail();
    calcPos();
    calcRot();

    if (--m_anmTimer == 0) {
        enterSliderSlow();
    }
}

/// @addr{0x80776670}
/// @brief Runs every frame the penguin is standing up from a slide
void ObjectPenguinS::calcStandUp() {
    calcRail();
    calcPos();
    calcRot();

    if (--m_anmTimer == 0) {
        enterWalk();
    }
}

/// @addr{0x807768A0}
/// @brief Runs every frame to update the penguin's position along the rail and update its state and
/// current animation timer
void ObjectPenguinS::calcRail() {
    if (m_railInterpolator->calc() != RailInterpolator::Status::SegmentEnd) {
        return;
    }

    const auto &curPoint = m_railInterpolator->curPoint();
    u16 setting = curPoint.setting[0];

    if (setting == 0 && m_state == State::SliderSlow) {
        enterStandUp();
    } else if (setting == 1 && m_state == State::Walk) {
        enterSlider();
    } else if (setting == 2 && m_state == State::Walk) {
        enterDive();
    }

    m_railInterpolator->setCurrVel(static_cast<f32>(curPoint.setting[1]));
}

} // namespace Kinoko::Field
