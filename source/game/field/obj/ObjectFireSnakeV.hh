#pragma once

#include "game/field/obj/ObjectFireSnake.hh"

namespace Field {

class ObjectFireSnakeV;

class ObjectFireSnakeV final : public ObjectFireSnake, public StateManager<ObjectFireSnakeV> {
    friend StateManager<ObjectFireSnakeV>;

public:
    ObjectFireSnakeV(const System::MapdataGeoObj &params);
    ~ObjectFireSnakeV() override;

    void init() override;
    void calc() override;

private:
    /// @addr{0x806C30F0}
    void enterDespawned() {
        ObjectFireSnake::enterDespawned();
    }

    void enterFalling();

    /// @addr{0x806C33B4}
    void enterHighBounce() {}

    /// @addr{0x806C30F4}
    void calcDespawned() {}

    void calcFalling();
    void calcHighBounce();

    void FUN_806C2DA4();

    static constexpr std::array<StateManagerEntry<ObjectFireSnakeV>, 6> STATE_ENTRIES = {{
            {0, &ObjectFireSnakeV::enterDespawned, &ObjectFireSnakeV::calcDespawned},
            {1, &ObjectFireSnakeV::enterFalling, &ObjectFireSnakeV::calcFalling},
            {2, &ObjectFireSnakeV::enterHighBounce, &ObjectFireSnakeV::calcHighBounce},
            {3, &ObjectFireSnake::enterRest, &ObjectFireSnake::calcRest},
            {4, &ObjectFireSnake::enterBounce, &ObjectFireSnake::calcBounce},
            {5, &ObjectFireSnake::enterDespawning, &ObjectFireSnake::calcDespawning},
    }};

    EGG::Vector3f m_nextPos; ///< In the base game it's in the base class but never used there
    const u16 m_cycleDuration;
    const f32 m_distFromPipe;
    f32 m_fallSpeed;

    static constexpr f32 RADIUS = 130.0f;
    static constexpr f32 GRAVITY = 3.0f;
};

} // namespace Field
