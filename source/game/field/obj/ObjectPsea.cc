#include "ObjectPsea.hh"

#include "game/field/ObjectDirector.hh"

namespace Kinoko::Field {

/// @addr{0x8082C234}
/// @copybrief ObjectCollidable::ObjectCollidable(const System::MapdataGeoObj &)
/// @details Calculates @ref m_initPosY based off hard-coded offsets. Initializes @ref m_frame to
/// zero. Finally, assigns this instance to the @ref ObjectDirector.
ObjectPsea::ObjectPsea(const System::MapdataGeoObj &params)
    : ObjectCollidable(params),
      m_initPosY(calcInitPosY()) {
    m_frame = 0;

    auto *objDir = ObjectDirector::Instance();
    ASSERT(!objDir->psea());
    objDir->setPsea(this);
}

/// @addr{0x8082C634}
/// @copybrief ObjectBase::calc()
/// @details Computes the position of the rising water using a sine wave with a period of @ref
/// PERIOD. Increments @ref m_frame, wrapping around to 0 after it exceeds @ref CYCLE_DURATION.
/// Finally, updates the Y-position of the rising water based on the computed sine value.
void ObjectPsea::calc() {
    constexpr f32 POS_OFFSET = 9590.399f;
    constexpr f32 AMPLITUDE = 140.0f;

    f32 sin = EGG::Mathf::sin(PERIOD * static_cast<f32>(m_frame));

    if (CYCLE_DURATION < m_frame++) {
        m_frame = 0;
    }

    f32 posY = m_initPosY + AMPLITUDE * sin + POS_OFFSET;
    setPos(EGG::Vector3f(pos().x, posY, pos().z));
}

} // namespace Kinoko::Field
