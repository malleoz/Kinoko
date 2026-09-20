#pragma once

#include "game/field/obj/ObjectKCL.hh"

namespace Kinoko::Field {

/// @brief Distinguishes between light and dark variants of a mushroom object
enum class KinokoType : u16 {
    Light = 0, ///< Light Mushrooms outside of the cave
    Dark = 1,  ///< Dark Mushrooms inside of the cave
};

/// @brief The base class for a mushroom object with jump pad properties
/// @details Implements a "pulse" scaling effect. Also defines a virtual oscillation function so
/// that derived classes can have custom cyclic behavior.
class ObjectKinoko : public ObjectKCL {
public:
    /// @addr{0x8080761C}
    /// @copybrief ObjectKCL::ObjectKCL(const System::MapdataGeoObj &)
    /// @details Initializes @ref m_type based on param setting 1. Caches the mushroom's initial
    /// position and rotation to @ref m_initPos and @ref m_initRot respectively. Initializes @ref
    /// m_restFrame to 0, @ref m_pulseFrame to 1, and @ref m_pulseAmplitude to `0.1f`.
    ObjectKinoko(const System::MapdataGeoObj &params)
        : ObjectKCL(params),
          m_type(static_cast<KinokoType>(params.setting(0))),
          m_initPos(pos()),
          m_initRot(rot()) {
        m_restFrame = 0;
        m_pulseFrame = 1; // (-1*-1) % PULSE_DURATION;
        m_pulseAmplitude = 0.1f;
    }

    /// @addr{0x80807A54}
    /// @brief Default virtual destructor
    ~ObjectKinoko() override = default;

    /// @addr{0x8080782C}
    /// @copybrief ObjectBase::calc()
    /// @details If the Mushroom is not idle, increments @ref m_pulseFrame. Otherwise, increments
    /// @ref m_restFrame. If the pulse or rest frames exceed their respective durations, they are
    /// reset to 0. Updates @ref m_pulseAmplitude and applies the pulse scaling effect before
    /// calling @ref calcOscillation(). Computes @ref m_pulseAmplitude based off the updated @ref
    /// m_pulseFrame and sets the scale accordingly. Finally, calls @ref calcOscillation() to update
    /// the Mushroom's position and orientation.
    void calc() override {
        constexpr s16 REST_DURATION = 10;
        constexpr s16 PULSE_DURATION = 40;
        constexpr f32 PULSE_SCALE = 0.0008f;
        constexpr f32 PULSE_FREQ = 6.0f * F_PI / 40.0f;

        if (m_restFrame == 0) {
            ++m_pulseFrame;
        }
        if (m_pulseFrame == PULSE_DURATION) {
            ++m_restFrame;
        }
        if (m_restFrame > REST_DURATION) {
            m_restFrame = 0;
        }
        if (m_pulseFrame > PULSE_DURATION) {
            m_pulseFrame = 0;
        }

        m_pulseAmplitude = PULSE_SCALE * static_cast<f32>(PULSE_DURATION - m_pulseFrame);
        setScale(m_pulseAmplitude * EGG::Mathf::sin(PULSE_FREQ * static_cast<f32>(m_pulseFrame)) +
                1.0f);
        calcOscillation();
    }

    /// @addr{0x80807DAC}
    /// @copybrief ObjectBase::loadFlags()
    /// @return Returns @ref eLoadFlags::Calc, so that object is calculated every frame.
    [[nodiscard]] LoadFlags loadFlags() const override {
        return LoadFlags(eLoadFlags::Calc);
    }

    /// @addr{0x80807D8C}
    /// @copybrief ObjectBase::getKclName()
    /// @details Loads the KCL for the corresponding light or dark variant
    /// @return The model name of the mushroom (`kinoko_r` for light, `kinoko_d_r` for dark)
    [[nodiscard]] const char *getKclName() const override {
        return m_type == KinokoType::Light ? "kinoko_r" : "kinoko_d_r";
    }

    /// @brief Updates the transformation matrix based off the mushroom's oscillation behavior
    virtual void calcOscillation() = 0;

protected:
    const KinokoType m_type;       ///< The variant of mushroom object (light or dark)
    const EGG::Vector3f m_initPos; ///< Initial position of the mushroom
    const EGG::Vector3f m_initRot; ///< Initial rotation of the mushroom
    s16 m_pulseFrame;              ///< Framecount used for the pulse sine wave
    s16 m_restFrame;               ///< Number of frames the mushroom has been idling for
    f32 m_pulseAmplitude;          ///< Dampens the sine wave amplitude towards the end of the cycle
    u16 m_oscFrame;                ///< Oscillation phase shift
};

/// @brief Mushrooms which oscillate up and down. The stem does not move.
/// @details This represents the first two mushrooms on Mushroom Gorge, even though they don't
/// oscillate up or down.
class ObjectKinokoUd final : public ObjectKinoko {
public:
    /// @addr{0x80807950}
    /// @copydoc ObjectKinoko::ObjectKinoko(const System::MapdataGeoObj &)
    /// @details
    ObjectKinokoUd(const System::MapdataGeoObj &params)
        : ObjectKinoko(params),
          m_period(std::max<u16>(params.setting(2), 2)),
          m_waitDuration(params.setting(4)),
          m_amplitude(params.setting(1)),
          m_angFreq(F_TAU / static_cast<f32>(m_period)) {
        m_waitFrame = 0;
        m_oscFrame = params.setting(3);
    }

    /// @addr{0x80807E1C}
    /// @brief Default virtual destructor
    ~ObjectKinokoUd() override = default;

    /// @addr{0x80807A54}
    /// @details Sinusoidal oscillation with a pause at the bottom of the cycle.
    void calcOscillation() override {
        f32 posY = m_initPos.y +
                static_cast<f32>(m_amplitude) *
                        (EGG::Mathf::cos(m_angFreq * static_cast<f32>(m_oscFrame)) + 1.0f) * 0.5f;
        setPos(EGG::Vector3f(pos().x, posY, pos().z));

        if (m_waitFrame == 0) {
            ++m_oscFrame;
        }
        if (m_oscFrame == (m_period / 2)) {
            ++m_waitFrame;
        }
        if (m_waitFrame > m_waitDuration) {
            m_waitFrame = 0;
        }
        if (m_oscFrame > m_period) {
            m_oscFrame = 0;
        }
    }

    /// @addr{0x80807DFC}
    /// @copybrief ObjectBase::getKclName()
    /// @details The base game does check for the light type, however since m_type never gets set
    /// it'll always be 0 which means it always returns "kinoko_r"
    /// @return The model name of the mushroom (`kinoko_r`)
    [[nodiscard]] const char *getKclName() const override {
        return "kinoko_r";
    }

    /// @addr{0x80807DF8}
    /// @copybrief ObjectKCL::calcScale()
    /// @details This is a nop because these mushrooms maintain constant scale
    void calcScale(u32) override {}

private:
    u16 m_waitFrame;    ///< How long the mushroom has paused in the middle of its oscillation cycle
    const s16 m_period; ///< Total oscillation period
    const s16 m_waitDuration; ///< How long to pause at its lowest point in the cycle
    const s16 m_amplitude;    ///< Maximum vertical displacement above the initial Y position
    const f32 m_angFreq;      ///< Angular frequency of the oscillation cosine wave
};

/// @brief Mushrooms which bend in a certain direction
/// @details This functionality didn't get used in the base game?
class ObjectKinokoBend final : public ObjectKinoko {
public:
    /// @addr{0x80807B7C}
    /// @copydoc ObjectKinoko::ObjectKinoko(const System::MapdataGeoObj &)
    ObjectKinokoBend(const System::MapdataGeoObj &params)
        : ObjectKinoko(params),
          m_period(std::max<u16>(params.setting(2), 2)),
          m_amplitude(static_cast<f32>(params.setting(1)) * DEG2RAD),
          m_angFreq(F_TAU / static_cast<f32>(m_period)) {
        m_currentFrame = params.setting(3);
    }

    /// @addr{0x80807DB4}
    /// @brief Default virtual destructor
    ~ObjectKinokoBend() override = default;

    /// @addr{0x80807C98}
    /// @details Applies an oscillating rotation that bends the mushroom.
    void calcOscillation() override {
        const f32 s = EGG::Mathf::sin(m_angFreq * static_cast<f32>(m_currentFrame));
        EGG::Vector3f rot = m_initRot + (EGG::Vector3f::ez * s) * m_amplitude;

        calcTransform();
        setRot(transform().multVector33(rot));

        if (++m_currentFrame >= m_period) {
            m_currentFrame = 0;
        }
    }

    /// @addr{0x80807D88}
    /// @copybrief ObjectKCL::calcScale()
    /// @details This is a nop because these mushrooms maintain constant scale
    void calcScale(u32) override {}

private:
    s16 m_currentFrame;    ///< Current frame within the bend animation cycle
    const s16 m_period;    ///< Total framecount of the animation cycle
    const f32 m_amplitude; ///< Max bend angle in radians
    const f32 m_angFreq;   ///< Angular frequency of the bend sine wave
};

/// @brief Green mushrooms with normal road properties, for the most part
class ObjectKinokoNm final : public ObjectKCL {
public:
    /// @addr{Inlined at 0x80821FE8}
    /// @copybrief ObjectKCL::ObjectKCL(const System::MapdataGeoObj &)
    /// @details Initializes @ref m_type based on param setting 1.
    ObjectKinokoNm(const System::MapdataGeoObj &params)
        : ObjectKCL(params),
          m_type(static_cast<KinokoType>(params.setting(0))) {}

    /// @addr{0x80827A9C}
    /// @brief Default virtual destructor
    ~ObjectKinokoNm() override = default;

    /// @addr{0x80827A74}
    /// @copybrief ObjectBase::getKclName()
    /// @return The model name of the mushroom (`kinoko_g` for light, `kinoko_d_g` for dark)
    [[nodiscard]] const char *getKclName() const override {
        return m_type == KinokoType::Light ? "kinoko_g" : "kinoko_d_g";
    }

private:
    const KinokoType m_type; ///< The variant of mushroom object (light or dark)
};

} // namespace Kinoko::Field
