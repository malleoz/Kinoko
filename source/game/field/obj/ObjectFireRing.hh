#pragma once

#include "game/field/obj/ObjectFireball.hh"

namespace Kinoko::Field {

/// @brief A rotating circle of fireballs that are spaced out equally
/// @details The first param setting is the number of fireballs. The fourth param setting is the
/// radius of the circle. The second param setting is the angular speed of the fire ring. Fire rings
/// pulse inward and outward in a sinusoidal motion, with an amplitude defined by `0.1f` times param
/// setting 3.
class ObjectFireRing final : public ObjectCollidable {
public:
    /// @addr{0x80767FF4}
    /// @brief Constructor
    /// @param params The parameters used to initialize the object
    /// Constructs and loads the required number of @ref ObjectFireball objects based on
    /// param setting 1. Computes and caches the distance from the fire ring and angle about the
    /// ring's rotation axis for each @ref ObjectFireball. Finally, Computes the axis of rotation
    /// for the fire ring and its initial tangent direction.
    ObjectFireRing(const System::MapdataGeoObj &params)
        : ObjectCollidable(params),
          m_fireballs(std::max<u32>(1, params.setting(0))),
          m_angSpeed(static_cast<f32>(static_cast<s16>(params.setting(1)))),
          m_pulseAmplitude(0.1f * static_cast<f32>(params.setting(2))),
          m_phase(0.0f) {
        f32 distance = 100.0f * static_cast<f32>(params.setting(3));

        for (size_t i = 0; i < m_fireballs.size(); ++i) {
            m_fireballs[i] = EGG::egg_new<ObjectFireball>(params);
            m_fireballs[i]->load();
            m_fireballs[i]->setDistance(distance);
            m_fireballs[i]->setAngle(static_cast<f32>(i) * (360.0f / m_fireballs.size()));
        }

        EGG::Matrix34f mat;
        mat.makeR(rot());
        m_axis = mat.base(2);
        m_axis.normalise();
        m_initDir = m_axis.cross(RotateAxisAngle(F_PI / 2.0f, EGG::Vector3f::ex, m_axis));
        m_initDir.normalise();
    }

    /// @addr{0x8076892C}
    /// @brief Default virtual destructor
    ~ObjectFireRing() override = default;

    /// @addr{0x807683F0}
    /// @copybrief ObjectBase::init()
    /// @details Resets the fire ring's angle to zero.
    void init() override {
        m_currAngle = 0.0f;
    }

    /// @addr{0x80768408}
    /// @copybrief ObjectBase::calc()
    /// @details Each frame, increments the phase of the fire ring's pulse by `1.0f`. Rotates the
    /// firebar based on @ref m_angSpeed. Updates the positions of all underlying fireballs
    /// accordingly, factoring in the fire ring's sinusoidal pulsing.
    void calc() override {
        m_phase += 1.0f;
        m_currAngle += m_angSpeed / 60.0f;

        if (m_currAngle > 360.0f) {
            m_currAngle -= 360.0f;
        } else if (m_currAngle < 0.0f) {
            m_currAngle += 360.0f;
        }

        f32 radius = m_pulseAmplitude * EGG::Mathf::sin(m_phase * DEG2RAD);

        for (auto *&fireball : m_fireballs) {
            EGG::Vector3f dir = m_initDir * fireball->distance() * (1.0f + radius);
            fireball->setPos(pos() +
                    RotateAxisAngle((m_currAngle + fireball->angle()) * DEG2RAD, m_axis, dir));
        }
    }

    /// @addr{0x80768740}
    /// @copybrief ObjectBase::loadFlags()
    /// @return Returns @ref eLoadFlags::Calc, so that object is calculated every frame.
    [[nodiscard]] LoadFlags loadFlags() const override {
        return LoadFlags(eLoadFlags::Calc);
    }

    /// @addr{0x80768734}
    /// @copybrief ObjectBase::createCollision()
    /// @details Does not create any collision, since this is effectively a manager class
    void createCollision() override {}

private:
    owning_span<ObjectFireball *> m_fireballs; ///< Pointers to the underlying fireball objects
    const f32 m_angSpeed;                      ///< Angular speed of the fireball circle
    f32 m_currAngle;                           ///< Current rotation angle in degrees modulo 360
    EGG::Vector3f m_axis;                      ///< Axis of rotation
    EGG::Vector3f m_initDir;                   ///< Initial tangent direction
    const f32 m_pulseAmplitude;                ///< Causes the ring's radius to stretch in and out
    f32 m_phase;                               ///< Number of frames elapsed since initialization
};

} // namespace Kinoko::Field
