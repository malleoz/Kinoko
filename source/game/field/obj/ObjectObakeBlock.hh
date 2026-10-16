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

    ObjectObakeBlock(const System::MapdataGeoObj &params);
    ~ObjectObakeBlock() override;

    void calc() override;
    void load() override;
    void createCollision() override;
    void calcCollisionTransform() override;

    /// @beginSetters

    void setFallState(FallState state);

    /// @endSetters

    /// @beginGetters

    [[nodiscard]] FallState fallState() const;
    [[nodiscard]] s32 fallFrame() const;

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
