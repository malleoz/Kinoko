#pragma once

#include "game/field/obj/ObjectFireSnake.hh"

namespace Kinoko::Field {

/// @brief Represents the bouncing fire snakes on Grumble Volcano
/// @details These fire snakes differ in behavior from the base class @ref ObjectFireSnake in that
/// their lifecycle is not controlled by @ref ObjectSunDS or any other @ref ObjectSniper. Instead,
/// these fire snakes repreatedly respawn from pipes.
class ObjectFireSnakeV final : public ObjectFireSnake {
public:
    ObjectFireSnakeV(const System::MapdataGeoObj &params);
    ~ObjectFireSnakeV() override;

    /// @addr{0x806C2CBC}
    /// @copybrief ObjectBase::init()
    void init() override {
        m_nextStateId = 1;
        ObjectFireSnake::enterDespawned();

        m_trajectoryPos = m_spawnPos;
        m_bounceDir = m_initRot;
    }

    /// @addr{0x806C2D54}
    /// @copybrief ObjectBase::calc()
    void calc() override {
        if (System::RaceManager::Instance()->timer() >= m_delayFrame) {
            calcSub();
        }
    }

private:
    void calcSub();

    void enterStateStub() {}

    /// @addr{0x806C30F0}
    /// @brief Runs once when the fire snake despawns
    void enterDespawned() {
        ObjectFireSnake::enterDespawned();
    }

    void enterFalling();

    /// @brief Runs once after landing from a jump
    void enterRest() {
        ObjectFireSnake::enterRest();
    }

    void calcStateStub() {}
    void calcFalling();
    void calcHighBounce();

    /// @brief Runs every frame while the fire snake is in between jumps
    void calcRest() {
        ObjectFireSnake::calcRest();
    }

    /// @addr{0x806C2254}
    /// @brief Runs every frame the fire snake is bouncing, except the first bounce
    void calcBounce() {
        ObjectFireSnake::calcBounce();
    }

    /// @brief The enter and calc functions for each @ref StateManager entry
    static constexpr std::array<StateManagerEntry, 6> STATE_ENTRIES = {{
            {StateEntry<ObjectFireSnakeV, &ObjectFireSnakeV::enterDespawned,
                    &ObjectFireSnakeV::calcStateStub>(0)},
            {StateEntry<ObjectFireSnakeV, &ObjectFireSnakeV::enterFalling,
                    &ObjectFireSnakeV::calcFalling>(1)},
            {StateEntry<ObjectFireSnakeV, &ObjectFireSnakeV::enterStateStub,
                    &ObjectFireSnakeV::calcHighBounce>(2)},
            {StateEntry<ObjectFireSnakeV, &ObjectFireSnakeV::enterRest,
                    &ObjectFireSnakeV::calcRest>(3)},
            {StateEntry<ObjectFireSnakeV, &ObjectFireSnakeV::enterStateStub,
                    &ObjectFireSnakeV::calcBounce>(4)},
            {StateEntry<ObjectFireSnakeV, &ObjectFireSnakeV::enterStateStub,
                    &ObjectFireSnakeV::calcStateStub>(5)},
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
