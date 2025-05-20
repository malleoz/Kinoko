#pragma once

#include "game/field/obj/ObjectCollidable.hh"

namespace Field {

class ObjectCarTGE;

class ObjectHighwayManager : public ObjectCollidable {
public:
    ObjectHighwayManager();
    ~ObjectHighwayManager() override;

    /// @addr{0x806D332C}
    void init() override {}

    void calc() override;

    /// @addr{0x806D5C68}
    void loadGraphics() override {}

    /// @addr{0x806D5C60}
    void createCollision() override {}

    /// @addr{0x806D5C64}
    void loadRail() override {}

    [[nodiscard]] u32 squashTimer() const {
        return m_squashTimer;
    }

private:
    void calcSquash();

    u32 m_carCount;
    u32 m_truckCount;
    std::span<ObjectCarTGE *> m_cars;
    std::span<ObjectCarTGE *> m_trucks;
    std::span<ObjectCarTGE *> m_all;
    u32 m_squashTimer; ///< Normally an array, one for each player.
};

} // namespace Field
