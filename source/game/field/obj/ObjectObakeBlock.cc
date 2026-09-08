#include "ObjectObakeBlock.hh"

namespace Kinoko::Field {

/// @addr{0x8080AD20}
/// @brief Constructor
/// @param params The parameters used to initialize the object
ObjectObakeBlock::ObjectObakeBlock(const System::MapdataGeoObj &params)
    : ObjectBase(params),
      m_initialPos(params.pos()),
      m_fallState(FallState::Rest),
      m_fallFrame(static_cast<s32>(
              static_cast<s16>(params.setting(2)) + static_cast<s16>(params.setting(1)) * 60)) {
    constexpr f32 FALL_LINEAR_SPEED = 1.0f;
    constexpr f32 FALL_ANGULAR_SPEED = 0.02f;

    m_framesFallen = 0;
    m_xzFallVel.setZero();
    m_fallAngVel.setZero();

    f32 yRot = params.rot().y;

    if (yRot == 0.0f) {
        m_xzFallVel.z = -FALL_LINEAR_SPEED;
        m_fallAngVel.x = -FALL_ANGULAR_SPEED;
    } else if (yRot == 90.0f) {
        m_xzFallVel.x = -FALL_LINEAR_SPEED;
        m_fallAngVel.z = FALL_ANGULAR_SPEED;
    } else if (yRot == 180.0f) {
        m_xzFallVel.z = FALL_LINEAR_SPEED;
        m_fallAngVel.x = FALL_ANGULAR_SPEED;
    } else if (yRot == -90.0f) {
        m_xzFallVel.x = FALL_LINEAR_SPEED;
        m_fallAngVel.z = -FALL_ANGULAR_SPEED;
    }
}

/// @addr{0x8080D8FC}
/// @brief Default virtual destructor
ObjectObakeBlock::~ObjectObakeBlock() = default;

/// @addr{0x8080BC64}
/// @copybrief ObjectBase::calc()
/// @details When the block is falling, updates position and rotation.
void ObjectObakeBlock::calc() {
    constexpr s32 FALL_DURATION = 255;

    if (m_fallState != FallState::Falling) {
        return;
    }

    setPos(m_initialPos + m_xzFallVel * (static_cast<f32>(m_framesFallen) * 2.0f));
    setRot(m_fallAngVel * static_cast<f32>(m_framesFallen));
    f32 posY = m_initialPos.y -
            (0.5f * static_cast<f32>(m_framesFallen)) * (0.5f * static_cast<f32>(m_framesFallen));
    setPos(EGG::Vector3f(pos().x, posY, pos().z));

    if (++m_framesFallen > FALL_DURATION) {
        m_fallState = FallState::FinishedFalling;
    }
}

} // namespace Kinoko::Field
