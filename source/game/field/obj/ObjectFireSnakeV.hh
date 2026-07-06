#pragma once

#include "game/field/obj/ObjectFireSnake.hh"

namespace Kinoko::Field {

class ObjectFireSnakeV final : public ObjectFireSnake {
public:
    ObjectFireSnakeV(const System::MapdataGeoObj &params);
    ~ObjectFireSnakeV() override;

    void init() override;
    void calc() override;

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

    const u16 m_cycleDuration;
    const f32 m_distFromPipe;
    f32 m_fallSpeed;

    static constexpr f32 RADIUS = 130.0f;
    static constexpr f32 GRAVITY = 3.0f;

    /// How many frames the snake falls before starting to check against floor collision
    static constexpr u32 COL_CHECK_DELAY_FRAMES = 10;
};

} // namespace Kinoko::Field
