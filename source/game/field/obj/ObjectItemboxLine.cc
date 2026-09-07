#include "ObjectItemboxLine.hh"

#include "game/field/obj/ObjectItemboxPress.hh"
#include "game/field/obj/ObjectPress.hh"

namespace Kinoko::Field {

/// @addr{0x8076D044}
/// @brief Constructor
/// @param params The parameters used to initialize the object
ObjectItemboxLine::ObjectItemboxLine(const System::MapdataGeoObj &params)
    : ObjectCollidable(params) {
    constexpr u32 DEFAULT_PRESS_COUNT = 5;

    auto *senko = EGG::egg_new<ObjectPressSenko>(params);
    senko->load();

    u32 pressCount = params.setting(6);
    if (pressCount == 0) {
        pressCount = DEFAULT_PRESS_COUNT;
    }

    m_press = owning_span<ObjectItemboxPress *>(pressCount);

    for (auto *&press : m_press) {
        press = EGG::egg_new<ObjectItemboxPress>(params);
        press->load();
        press->setSenko(senko);
    }
}

/// @addr{0x8076D558}
/// @brief Default virtual destructor
ObjectItemboxLine::~ObjectItemboxLine() = default;

/// @addr{0x8076D604}
/// @copybrief ObjectBase::init()
void ObjectItemboxLine::init() {
    ASSERT(m_mapObj);
    u32 timer = static_cast<u32>(m_mapObj->setting(4));

    if (timer == 0) {
        timer = static_cast<u32>(m_mapObj->setting(5));
    }

    m_stompCooldown = timer;
    m_curPressIdx = 0;
}

/// @addr{0x8076D64C}
/// @copybrief ObjectBase::calc()
/// @details Decrements the cooldown timer and activates the next stomper when it reaches 0
void ObjectItemboxLine::calc() {
    if (--m_stompCooldown > 0) {
        return;
    }

    m_stompCooldown = static_cast<u32>(m_mapObj->setting(5));

    m_press[m_curPressIdx]->startPress();
    m_curPressIdx = (m_curPressIdx + 1) % m_press.size();
}

} // namespace Kinoko::Field
