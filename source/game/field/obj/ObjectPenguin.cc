#include "ObjectPenguin.hh"

namespace Kinoko::Field {

/// @addr{0x80775624} @addr{0x8077708C}
/// @brief Constructor
/// @param params The parameters used to initialize the object
ObjectPenguin::ObjectPenguin(const System::MapdataGeoObj &params) : ObjectCollidable(params) {}

/// @addr{0x80775670} @addr{0x807774A4}
/// @brief Default virtual destructor
ObjectPenguin::~ObjectPenguin() = default;

/// @addr{0x80775E60}
/// @brief Constructor
/// @param params The parameters used to initialize the object
ObjectPenguinS::ObjectPenguinS(const System::MapdataGeoObj &params) : ObjectPenguin(params) {}

/// @addr{0x80776070}
/// @brief Default virtual destructor
ObjectPenguinS::~ObjectPenguinS() = default;

/// @addr{0x80776198}
/// @copybrief ObjectBase::calc()
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

/// @addr{0x8077739C}
/// @copybrief ObjectBase::loadAnims()
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

    m_railInterpolator->setSpeed(static_cast<f32>(curPoint.setting[1]));
}

} // namespace Kinoko::Field
