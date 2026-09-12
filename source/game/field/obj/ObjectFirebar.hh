#pragma once

#include "game/field/obj/ObjectFireball.hh"

namespace Kinoko::Field {

/// @brief A rotating group of fireballs split into spokes.
/// @details The number of spokes is based on the fourth param setting. The first param setting
/// defines how many fireballs per spoke.
class ObjectFirebar final : public ObjectCollidable {
public:
    /// @addr{0x807678F4}
    /// @brief Constructor
    /// @param params The parameters used to initialize the object
    /// @details Constructs and loads the required number of @ref ObjectFireball objects based on
    /// the number of spokes (param setting 4) and fireballs per spoke (param setting 1). Computes
    /// and caches the distance from the firebar and angle about the firebar's rotation axis for
    /// each @ref ObjectFireball. Finally, Computes the axis of rotation for the firebar and its
    /// initial tangent direction.
    ObjectFirebar(const System::MapdataGeoObj &params)
        : ObjectCollidable(params),
          m_spokes(std::max<u32>(1, params.setting(3))),
          m_angSpeed(static_cast<f32>(static_cast<s16>(params.setting(1)))),
          m_fireballs(std::max<u32>(1, params.setting(0) * m_spokes)) {
        const f32 ringSpacing = 100.0f * static_cast<f32>(static_cast<s16>(params.setting(2)));

        for (size_t i = 0; i < m_fireballs.size(); ++i) {
            m_fireballs[i] = EGG::egg_new<ObjectFireball>(params);
            m_fireballs[i]->load();

            f32 ring = 1.0f + static_cast<f32>(i / m_spokes);
            m_fireballs[i]->setDistance(ring * ringSpacing);
            m_fireballs[i]->setAngle((360.0f / m_spokes) * (i % m_spokes));
        }

        EGG::Matrix34f mat;
        mat.makeR(rot());
        m_axis = mat.base(2);
        m_axis.normalise();
        m_initDir = m_axis.cross(RotateAxisAngle(F_PI / 2.0f, EGG::Vector3f::ex, m_axis));
        m_initDir.normalise();
    }

    /// @addr{0x807688AC}
    /// @brief Default virtual destructor
    ~ObjectFirebar() override = default;

    /// @addr{0x80767DEC}
    /// @copybrief ObjectBase::init()
    /// @details Resets the firebar's angle to zero.
    void init() override {
        m_currAngle = 0.0f;
    }

    /// @addr{0x80767E04}
    /// @copybrief ObjectBase::calc()
    /// @details Rotates the firebar based on @ref m_angSpeed. Updates the positions of all
    /// underlying fireballs accordingly.
    void calc() override {
        m_currAngle += m_angSpeed / 60.0f;

        if (m_currAngle > 360.0f) {
            m_currAngle -= 360.0f;
        } else if (m_currAngle < 0.0f) {
            m_currAngle += 360.0f;
        }

        for (auto *&fireball : m_fireballs) {
            EGG::Vector3f dir = m_initDir * fireball->distance();
            fireball->setPos(pos() +
                    RotateAxisAngle((m_currAngle + fireball->angle()) * DEG2RAD, m_axis, dir));
        }
    }

    /// @addr{0x807687D8}
    /// @copybrief ObjectBase::loadFlags()
    /// @return Returns @ref eLoadFlags::Calc, so that object is calculated every frame.
    [[nodiscard]] LoadFlags loadFlags() const override {
        return LoadFlags(eLoadFlags::Calc);
    }

    /// @addr{0x808CE358}
    /// @copybrief ObjectBase::getKclName()
    /// @return The model name of the firebar (`WLfirebarGC` or `koopaFirebar`)
    [[nodiscard]] const char *getKclName() const override {
        return id() == ObjectId::WLFirebarGC ? "WLfirebarGC" : "koopaFirebar";
    }

private:
    owning_span<ObjectFireball *> m_fireballs; ///< Array of pointers to underlying fireballs
    const u32 m_spokes;                        ///< The number of fireball "segments"
    const f32 m_angSpeed;                      ///< Angular speed of the fireballs (per second)
    f32 m_currAngle;                           ///< Current angle of rotation modulo 360
    EGG::Vector3f m_axis;                      ///< Axis of rotation
    EGG::Vector3f m_initDir;                   ///< Initial tangent direction
};

} // namespace Kinoko::Field
