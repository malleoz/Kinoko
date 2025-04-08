#pragma once

#include "game/field/obj/ObjectCollidable.hh"

namespace Field {

class ObjectSanbo : public ObjectCollidable {
public:
    ObjectSanbo(const System::MapdataGeoObj &params);
    ~ObjectSanbo() override;

    void init() override;
    void calc() override;

    /// @addr{0x8077BD38}
    [[nodiscard]] u32 loadFlags() const override {
        return 1;
    }

private:
    void calcMove();
    void checkSphere();

    bool m_standstill;       ///< Whether or not the pokey is currently walking (0xc4)
    u32 m_movingDelay;       ///< Frames before the pokey will start to walk (0xc8)
    f32 m_yVelocity;         ///< Falling speed (0xd0)
    EGG::Vector3f m_up;      // 0xd8
    EGG::Vector3f m_tangent; // 0xe4
};

} // namespace Field
