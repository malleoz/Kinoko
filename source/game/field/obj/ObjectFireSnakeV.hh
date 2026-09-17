#pragma once

#include "game/field/obj/ObjectFireSnake.hh"

namespace Kinoko::Field {

/// @brief Represents the bouncing fire snakes on Grumble Volcano
/// @details These fire snakes differ in behavior from the base class @ref ObjectFireSnake in that
/// their lifecycle is not controlled by @ref ObjectSunDS or any other @ref ObjectSniper. Instead,
/// these fire snakes repreatedly respawn from pipes.
class ObjectFireSnakeV final : public ObjectFireSnake {
public:
    /// @addr{0x806C2B70}
    /// @copydoc ObjectFireSnake::ObjectFireSnake(const System::MapdataGeoObj &)
    /// @details Sets @ref m_cycleDuration based on param setting 2, @ref m_distFromPipe based on
    /// param setting 3, and @ref m_delayFrame from param setting 1. Sets @ref m_spawnPos to the
    /// object's position. Updates the fire snake's transform and calculates its initial rotation
    /// and landing position.
    ObjectFireSnakeV(const System::MapdataGeoObj &params)
        : StateManager(this, STATE_ENTRIES),
          ObjectFireSnake(params),
          m_cycleDuration(params.setting(1)),
          m_distFromPipe(static_cast<f32>(params.setting(2))),
          m_fallSpeed(0.0f) {
        m_delayFrame = params.setting(0);
        m_spawnPos = pos();

        calcTransform();

        m_initRot = transform().base(0);
        m_initPos = m_spawnPos + m_initRot * m_distFromPipe;
    }

    /// @addr{0x806C3548}
    /// @brief Default virtual destructor
    ~ObjectFireSnakeV() override = default;

    /// @addr{0x806C2CBC}
    /// @copybrief ObjectBase::init()
    /// @details The fire snake starts despawned. Even though @ref m_nextStateId is set to the
    /// falling state, the state machine will not actually run in @ref calcSub() until @ref
    /// m_delayFrame frames have elapsed. Sets @ref m_trajectoryPos to the fire snake's spawn
    /// position and initializes the @ref m_bounceDir of the fire snake.
    void init() override {
        m_nextStateId = 1;
        ObjectFireSnake::enterDespawned();

        m_trajectoryPos = m_spawnPos;
        m_bounceDir = m_initRot;
    }

    /// @addr{0x806C2D54}
    /// @copybrief ObjectBase::calc()
    /// @details Only starts calculating the fire snake's behavior after @ref m_delayFrame frames
    /// have elapsed in the race. Dispatches to @ref calcSub() for the actual calculation.
    void calc() override {
        if (System::RaceManager::Instance()->timer() >= m_delayFrame) {
            calcSub();
        }
    }

private:
    void calcSub();

    /// @addr{0x806C30F0}
    /// @brief Runs once when the fire snake despawns
    /// @details Calls the base class implementation to handle the fire snake's despawn behavior.
    /// This is a separate function definition because @ref StateManagerEntry expects a member
    /// function of the derived type, thus we cannot pass a @ref ObjectFireSnake pointer directly.
    void enterDespawned() {
        ObjectFireSnake::enterDespawned();
    }

    void enterFalling();

    /// @brief Runs once after landing from a jump
    /// @details Calls the base class implementation to handle the fire snake's enter rest behavior.
    /// This is a separate function definition because @ref StateManagerEntry expects a member
    /// function of the derived type, thus we cannot pass a @ref ObjectFireSnake pointer directly.
    void enterRest() {
        ObjectFireSnake::enterRest();
    }

    void calcFalling();
    void calcHighBounce();

    /// @brief Runs every frame while the fire snake is in between jumps
    /// @details Calls the base class implementation to handle the fire snake's rest behavior.
    /// This is a separate function definition because @ref StateManagerEntry expects a member
    /// function of the derived type, thus we cannot pass a @ref ObjectFireSnake pointer directly.
    void calcRest() {
        ObjectFireSnake::calcRest();
    }

    /// @addr{0x806C2254}
    /// @brief Runs every frame the fire snake is bouncing, except the first bounce
    /// @details Calls the base class implementation to handle the fire snake's bounce behavior.
    /// This is a separate function definition because @ref StateManagerEntry expects a member
    /// function of the derived type, thus we cannot pass a @ref ObjectFireSnake pointer directly.
    void calcBounce() {
        ObjectFireSnake::calcBounce();
    }

    /// @brief The enter and calc functions for each @ref StateManager entry
    static constexpr std::array<StateManagerEntry, 6> STATE_ENTRIES = {{
            {StateEntry<ObjectFireSnakeV, &ObjectFireSnakeV::enterDespawned, nullptr>(0)},
            {StateEntry<ObjectFireSnakeV, &ObjectFireSnakeV::enterFalling,
                    &ObjectFireSnakeV::calcFalling>(1)},
            {StateEntry<ObjectFireSnakeV, nullptr, &ObjectFireSnakeV::calcHighBounce>(2)},
            {StateEntry<ObjectFireSnakeV, &ObjectFireSnakeV::enterRest,
                    &ObjectFireSnakeV::calcRest>(3)},
            {StateEntry<ObjectFireSnakeV, nullptr, &ObjectFireSnakeV::calcBounce>(4)},
            {StateEntry<ObjectFireSnakeV, nullptr, nullptr>(5)},
    }};

    const u16 m_cycleDuration; ///< Number of frames between two fire snake spawns
    const f32 m_distFromPipe;  ///< Distance from spawn position to the landing position
    f32 m_fallSpeed;           ///< XZ speed of the fire snake after spawning and before landing

    static constexpr f32 RADIUS = 130.0f; ///< Collision radius of the fire snake
    static constexpr f32 GRAVITY = 3.0f;  ///< Gravity applied during spawn and the first bounce

    /// @brief How many frames the snake falls before starting to check against floor collision
    static constexpr u32 COL_CHECK_DELAY_FRAMES = 10;
};

} // namespace Kinoko::Field
