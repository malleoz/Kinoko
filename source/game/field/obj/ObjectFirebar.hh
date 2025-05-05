#pragma once

#include "game/field/obj/ObjectCollidable.hh"
#include "game/field/obj/ObjectFireball.hh"

namespace Field {

class ObjectFirebar : public ObjectCollidable {
public:
    ObjectFirebar(const System::MapdataGeoObj &params);
    ~ObjectFirebar() override;

    void init() override;
    void calc() override;

    /// @addr{0x807687D8}
    [[nodiscard]] u32 loadFlags() const override {
        return 1;
    }

private:
    std::span<ObjectFireball *> m_fireballs;
    u32 m_numBars; // The number of fireball "segments"
    f32 m_angSpeed;
    f32 m_degAngle;
    EGG::Vector3f m_cc;
    EGG::Vector3f m_d8;
};

} // namespace Field
