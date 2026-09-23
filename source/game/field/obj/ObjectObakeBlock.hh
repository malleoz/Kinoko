#pragma once

#include "game/field/obj/ObjectBase.hh"

namespace Kinoko::Field {

/// @brief Represents a falling block on SNES Ghost Valley 2.
/// @details Most @ref ObjectBase virtual functions are no-ops because collision is handled by @ref
/// ObjectObakeManager, which manages the lifecycle of these blocks in a spatially indexed cache. If
/// a block's second setting is greater than 0, then it will fall after that many seconds have
/// passed.
class ObjectObakeBlock final : public ObjectBase {
public:
    /// @brief Represents the current state of the block in its falling lifecycle
    enum class FallState {
        Rest = 0,            ///< The block is stationary and has not yet started falling
        Falling = 1,         ///< The block is currently falling
        FinishedFalling = 2, ///< The block has finished falling and is now stationary
    };

    /// @addr{0x8080AD20}
    /// @copybrief ObjectBase::ObjectBase(const System::MapdataGeoObj &)
    /// @param params The parameters used to initialize the object
    /// @details Caches the object's initial position to @ref m_initPos and initializes the block's
    /// fall state to @ref FallState::Rest. Determines the frame at which the block will start to
    /// fall using the param settings, where setting 2 is the number of minutes and setting 3 is the
    /// number of seconds; if both are 0, then the block will never fall. Zeroes @ref
    /// m_framesFallen. Calculates @ref m_xzFallVel and @ref m_fallAngVel based on the initial
    /// rotation of the block.
    ObjectObakeBlock(const System::MapdataGeoObj &params)
        : ObjectBase(params),
          m_initPos(params.pos()),
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
    ~ObjectObakeBlock() override = default;

    /// @addr{0x8080BC64}
    /// @copybrief ObjectBase::calc()
    /// @details If the block is at rest, then does nothing. Otherwise, updates the XZ position of
    /// the block with a velocity of `2.0f` times @ref m_xzFallVel. Updates the block's rotation
    /// based off @ref m_fallAngVel. Updates the blocks height based off its initial height and a
    /// downward gravitational force of `0.25f`. Finally, increments @ref m_framesFallen and
    /// transitions the block to @ref FallState::FinishedFalling if the block has been falling for
    /// more than 255 frames.
    void calc() override {
        constexpr f32 XZ_SPEED_SCALAR = 2.0f;
        constexpr s32 FALL_DURATION = 255;

        if (m_fallState != FallState::Falling) {
            return;
        }

        setPos(m_initPos + m_xzFallVel * (static_cast<f32>(m_framesFallen) * XZ_SPEED_SCALAR));
        setRot(m_fallAngVel * static_cast<f32>(m_framesFallen));
        f32 posY = m_initPos.y -
                (0.5f * static_cast<f32>(m_framesFallen)) *
                        (0.5f * static_cast<f32>(m_framesFallen));
        setPos(EGG::Vector3f(pos().x, posY, pos().z));

        if (++m_framesFallen > FALL_DURATION) {
            m_fallState = FallState::FinishedFalling;
        }
    }

    /// @addr{0x8080BDE0}
    /// @copybrief ObjectBase::load()
    /// @details no-op because collision is handled by @ref ObjectObakeManager
    void load() override {}

    /// @addr{0x8080BDDC}
    /// @copybrief ObjectBase::createCollision()
    /// @details no-op because collision is handled by @ref ObjectObakeManager
    void createCollision() override {}

    /// @addr{0x8080BDD4}
    /// @copybrief ObjectBase::calcCollisionTransform()
    /// @details no-op because collision is handled by @ref ObjectObakeManager
    void calcCollisionTransform() override {}

    /// @beginSetters

    /// @brief Sets the block's fall state
    /// @param state The new fall state to set for the block
    void setFallState(FallState state) {
        m_fallState = state;
    }

    /// @endSetters

    /// @beginGetters

    /// @brief Gets the block's current fall state
    /// @return The current fall state of the block
    [[nodiscard]] FallState fallState() const {
        return m_fallState;
    }

    /// @brief Gets the frame the block starts falling
    /// @return The frame the block starts falling, or 0 if it never falls
    [[nodiscard]] s32 fallFrame() const {
        return m_fallFrame;
    }

    /// @endGetters

private:
    const EGG::Vector3f m_initPos; ///< Inital position of the block
    FallState m_fallState;         ///< Current state of the block in its falling lifecycle
    s32 m_framesFallen;            ///< How long the block has been falling for, capped at 256
    EGG::Vector3f m_xzFallVel;     ///< Lateral velocity while the block is falling
    EGG::Vector3f m_fallAngVel;    ///< Angular velocity while the block is falling
    const s32 m_fallFrame;         ///< Frame the block starts falling, or 0 if it never falls
};

} // namespace Kinoko::Field
