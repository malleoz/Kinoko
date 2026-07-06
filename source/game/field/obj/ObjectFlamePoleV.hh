#pragma once

#include "game/field/StateManager.hh"
#include "game/field/obj/ObjectCollidable.hh"

namespace Kinoko::Field {

/// @brief Lava geysers without any hump (@ref ObjectFlamePoleFoot)
class ObjectFlamePoleV final : public ObjectCollidable, public StateManager {
public:
    ObjectFlamePoleV(const System::MapdataGeoObj &params);
    ~ObjectFlamePoleV() override;

    void init() override;
    void calc() override;

    /// @addr{0x806C4898}
    [[nodiscard]] u32 loadFlags() const override {
        return 1;
    }

    /// @addr{0x806C488C}
    [[nodiscard]] const char *getResources() const override {
        return "FlamePole_v";
    }

    /// @addr{0x806C4880}
    [[nodiscard]] const char *getKclName() const override {
        return "FlamePoleEff";
    }

private:
    void enterStateStub() {}

    /// @addr{0x806C40E4}
    /// @brief Runs once when the geyser is leaving dormancy
    void enterBeforeErupting() {
        enableCollision();
    }

    /// @addr{0x806C4178}
    /// @brief Runs once when the geyser begins erupting upwards
    void enterErupting() {
        m_currOffsetY = 0.0f;
        CalcEruptionKinematics(ERUPT_FRAMES, m_maxOffsetY, m_initEruptingVel, m_eruptionDecel);
    }

    /// @addr{0x806C43BC}
    /// @brief Runs once when the geyser begins to descend
    void enterLowering() {
        m_loweringStartOffsetY = m_currOffsetY;
        m_fallSpeed = m_maxOffsetY / FALL_FRAMES;
    }

    /// @addr{0x806C4478}
    /// @brief Runs once when the geyser enters dormancy
    void enterDormant() {
        disableCollision();
    }

    /// @addr{0x806C4130}
    /// @brief Runs for the 50 frames before the geyser begins erupting
    void calcBeforeErupting() {
        if (static_cast<f32>(m_currentFrame) >= BEFORE_ERUPT_FRAMES) {
            m_nextStateId = 1;
        }
    }

    /// @addr{0x806C41F0}
    /// @brief Runs every frame that the geyser is erupting
    void calcErupting() {
        if (static_cast<f32>(m_currentFrame) >= ERUPT_FRAMES) {
            m_nextStateId = 2;
        }

        m_currOffsetY =
                CalcParabolicDisplacement(m_initEruptingVel, m_eruptionDecel, m_currentFrame);
    }

    void calcErupted();
    void calcLowering();

    /// @addr{0x806C44C8}
    /// @brief Runs every frame that the geyser lays dormant
    void calcDormant() {
        if (m_currentFrame >= static_cast<u32>(m_dormantFrames)) {
            m_nextStateId = 0;
        }
    }

    /// @addr{0x806B5A0C}
    /// @brief Computes initVel and accel such that it takes t frames and the peak is maxHeight
    /// @details Derived from the kinematic equations \f$y(t) = v_0 t - \frac12 a t^2\f$ and
    ///          \f$v(t) = v_0 - a t\f$. At the peak, \f$v(t) = 0\f$, so \f$a = v_0 / t\f$,
    ///          which reduces the displacement equation to \f$y(t) = \frac12 v_0 t\f$. Solving
    ///          for \f$v_0\f$ with \f$y(t) = \text{maxHeight}\f$ gives
    ///          \f$v_0 = 2 \cdot \text{maxHeight} / t\f$, and substituting back into
    ///          \f$a = v_0 / t\f$ gives \f$a = v_0^2 / (2 \cdot \text{maxHeight})\f$.
    static void CalcEruptionKinematics(f32 t, f32 maxHeight, f32 &initVel, f32 &accel) {
        f32 doublePeak = 2.0f * maxHeight;
        initVel = doublePeak / t;
        accel = initVel * initVel / doublePeak;
    }

    const u32 m_initDelay;     ///< Frames before state lifecycle begins
    const s32 m_cycleDuration; ///< Duration of a full loop of the geyser's state lifecycle
    const s32 m_dormantFrames; ///< Number of frames within a cycle the geyser remains dormant for
    const f32 m_scaleFactor;   ///< Scale multiplier
    const f32 m_initPosY;      ///< Initial y-axis position
    f32 m_maxOffsetY;          ///< Max height offset of the pole
    f32 m_eruptionDecel;       ///< Effectively the gravity coefficient when erupting
    f32 m_initEruptingVel;     ///< Velocity of the geyser when it begins erupting upwards
    f32 m_fallSpeed;           ///< Rate at which the geyser lowers
    const bool m_isBig; ///< Discerns between FlamePole_v and FlamePole_v_big to alter size handling
    f32 m_currOffsetY;  ///< Height offset based off eruption velocity
    f32 m_loweringStartOffsetY; ///< Height offset when the geyser begins lowering to dormancy

    static constexpr f32 ERUPT_FRAMES = 60.0f;        ///< Frames it takes to raise to max height
    static constexpr f32 BEFORE_ERUPT_FRAMES = 50.0f; ///< Delay before pole starts erupting
    static constexpr f32 FALL_FRAMES = 180.0f;        ///< Frames it takes to lower into the ground

    static constexpr std::array<StateManagerEntry, 5> STATE_ENTRIES = {{
            {StateEntry<ObjectFlamePoleV, &ObjectFlamePoleV::enterBeforeErupting,
                    &ObjectFlamePoleV::calcBeforeErupting>(0)},
            {StateEntry<ObjectFlamePoleV, &ObjectFlamePoleV::enterErupting,
                    &ObjectFlamePoleV::calcErupting>(1)},
            {StateEntry<ObjectFlamePoleV, &ObjectFlamePoleV::enterStateStub,
                    &ObjectFlamePoleV::calcErupted>(2)},
            {StateEntry<ObjectFlamePoleV, &ObjectFlamePoleV::enterLowering,
                    &ObjectFlamePoleV::calcLowering>(3)},
            {StateEntry<ObjectFlamePoleV, &ObjectFlamePoleV::enterDormant,
                    &ObjectFlamePoleV::calcDormant>(4)},
    }};
};

} // namespace Kinoko::Field
