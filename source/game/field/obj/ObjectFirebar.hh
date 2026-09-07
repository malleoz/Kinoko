#pragma once

#include "game/field/obj/ObjectFireball.hh"

namespace Kinoko::Field {

/// @brief A rotating group of fireballs split into spokes.
/// @details The number of spokes is based on the fourth param setting. The first param setting
/// defines how many fireballs per spoke.
class ObjectFirebar final : public ObjectCollidable {
public:
    ObjectFirebar(const System::MapdataGeoObj &params);
    ~ObjectFirebar() override;

    /// @addr{0x80767DEC}
    /// @copybrief ObjectBase::init()
    void init() override {
        m_currAngle = 0.0f;
    }

    void calc() override;

    /// @addr{0x807687D8}
    /// @copybrief ObjectBase::loadFlags()
    /// @return Returns @ref eLoadFlags::Calc, so that object is calculated every frame.
    [[nodiscard]] LoadFlags loadFlags() const override {
        return LoadFlags(eLoadFlags::Calc);
    }

    /// @addr{0x808CE358}
    [[nodiscard]] const char *getKclName() const override {
        return id() == ObjectId::WLFirebarGC ? "WLfirebarGC" : "koopaFirebar";
    }

private:
    owning_span<ObjectFireball *> m_fireballs; ///< Array of pointers to underlying fireballs
    const u32 m_spokes;                        ///< The number of fireball "segments"
    const f32 m_angSpeed;                      ///< Angular speed of the fireballs (in seconds)
    f32 m_currAngle;                           ///< Current angle of rotation modulo 360
    EGG::Vector3f m_axis;                      ///< Axis of rotation
    EGG::Vector3f m_initDir;                   ///< Initial tangent direction
};

} // namespace Kinoko::Field
