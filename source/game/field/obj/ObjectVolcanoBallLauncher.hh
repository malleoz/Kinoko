#pragma once

#include "game/field/obj/ObjectCollidable.hh"

namespace Field {

class ObjectVolcanoBall;

class ObjectVolcanoBallLauncher final : public ObjectCollidable {
public:
    ObjectVolcanoBallLauncher(const System::MapdataGeoObj &params);
    ~ObjectVolcanoBallLauncher() override;

    void init() override;
    void calc() override;

    /// @addr{0x806E3A74}
    [[nodiscard]] u32 loadFlags() const override {
        return 1;
    };

    /// @addr{0x806E3A70}
    void loadGraphics() override {}

    /// @addr{0x806E3A68}
    void createCollision() override {}

    /// @addr{0x806E3A6C}
    void loadRail() override {}

private:
    std::span<ObjectVolcanoBall *> m_balls;
    f32 m_initDelay;
    f32 m_cycleDuration;
    u32 m_c0;
    u32 m_currBallIdx;
    f32 m_c8;
    f32 m_cc;
    f32 m_d0;
    bool m_d4;
};

} // namespace Field
