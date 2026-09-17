#pragma once

#include "game/field/StateManager.hh"
#include "game/field/obj/ObjectFlamePole.hh"
#include "game/field/obj/ObjectKCL.hh"

#include "game/system/RaceManager.hh"

namespace Kinoko::Field {

class ObjectFlamePole;

/** @brief Calculates the start frame for each of the @ref ObjectFlamePoleFoot state boundaries
 * @return An array containing the start frames for each of the six states
 * @relates ObjectFlamePoleFoot
 * @details We extract this logic to a helper function in Kinoko since the frame boundaries can be
 *determined at compile-time. Let \f$t\f$ be @ref m_cycleFrame and \f$s_i\f$ be @ref STATE_STARTS
 *"STATE_STARTS[i]". Then
 * \f[
 * \text{stateId}(t) =
 * \begin{cases}
 *     0 & 0 \le t < s_1 \\
 *     1 & s_1 \le t < s_2 \\
 *     2 & s_2 \le t < s_3 \\
 *     3 & s_3 \le t < s_4 \\
 *     4 & s_4 \le t < s_5 \\
 *     5 & t \ge s_5
 * \end{cases}
 * \f]
 * where
 * \f[
 * \begin{aligned}
 * N &= 6 \cdot \text{CYCLE_FRAMES} \\
 * s_1 &= \left\lfloor \frac{0.3 N}{7} \right\rfloor \\
 * s_2 &= s_1 + \left\lfloor \frac{0.1 N}{7} \right\rfloor \\
 * s_3 &= s_2 + \left\lfloor \frac{\text{CYCLE_FRAMES}}{7} \right\rfloor \\
 * s_4 &= s_3 + \left\lfloor \frac{0.2 N}{7} \right\rfloor \\
 * s_5 &= s_4 + \left\lfloor \frac{0.4 N}{7} \right\rfloor
 * \end{aligned}
 * \f]
 * Simplifying further, we have:
 * \f[
 * \text{stateId}(t) =
 * \begin{cases}
 *     0 & 0 \le t < 138 \\
 *     1 & 138 \le t < 184 \\
 *     2 & 184 \le t < 261 \\
 *     3 & 261 \le t < 353 \\
 *     4 & 353 \le t < 538 \\
 *     5 & t \ge 538
 * \end{cases}
 * \f]
 **/
[[nodiscard]] static consteval std::array<s32, 6> FlamePoleFootStateFrameBoundaries() {
    constexpr u32 CYCLE_DURATION = 540;
    constexpr f32 NORMALIZATION = static_cast<f32>(CYCLE_DURATION) * (7.0f - 1.0f);
    constexpr s32 ERUPT_UP_DURATION = static_cast<s32>(0.1f * NORMALIZATION / 7.0f);
    constexpr s32 ERUPT_DOWN_DURATION = static_cast<s32>(0.2f * NORMALIZATION / 7.0f);

    s32 state1 = static_cast<s32>(0.3f * NORMALIZATION / 7.0f);
    s32 state2 = state1 + ERUPT_UP_DURATION;
    s32 state3 = state2 + static_cast<s32>(static_cast<f32>(CYCLE_DURATION) * 1.0f / 7.0f);
    s32 state4 = state3 + ERUPT_DOWN_DURATION;
    s32 state5 = state4 + static_cast<s32>(0.4f * NORMALIZATION / 7.0f);
    return {{0, state1, state2, state3, state4, state5}};
}

/// @brief Represents the expanding geyser humps at the end of Bowser's Castle.
/// @details Each "foot" owns a @ref ObjectFlamePole. The foot controls updates the pole's scale to
/// effectively cause the pole to appear to erupt out of the ground. The pole erupts upwards, dips a
/// bit, rises back up, and then descends back beneath the foot.
class ObjectFlamePoleFoot final : public ObjectKCL, private StateManager {
    /// @brief Grants access to the singleton so a @ref Host::Context can restore the instance's
    /// state on context switch
    friend class Host::Context;

public:
    ObjectFlamePoleFoot(const System::MapdataGeoObj &params);

    /// @addr{0x8067EBE0}
    /// @brief Default virtual destructor
    ~ObjectFlamePoleFoot() override {
        s_flamePoleCount = 0;
    }

    void init() override;

    /// @addr{0x8067EF70}
    /// @copybrief ObjectBase::calc()
    /// @details Only starts calculating the geyser's state after @ref m_initDelay frames have
    /// elapsed in the race. If so, calls @ref calcStates() to determine the current state of the
    /// geyser, evaluates the geyser's state machine, and updates the height and scale of the @ref
    /// ObjectFlamePole accordingly.
    void calc() override {
        if (System::RaceManager::Instance()->timer() < m_initDelay) {
            return;
        }

        calcStates();
        StateManager::calc();
        calcHeightAndScale();
    }

    /// @addr{0x80681590}
    /// @copybrief ObjectBase::loadFlags()
    /// @return Returns @ref eLoadFlags::Calc, so that object is calculated every frame.
    [[nodiscard]] LoadFlags loadFlags() const override {
        return LoadFlags(eLoadFlags::Calc);
    }

    /// @addr{0x806814C4}
    /// @copybrief ObjectBase::getCollisionRadius()
    /// @return The collision radius of the flame pole foot, which is `245.0f` times param
    /// setting 3.
    [[nodiscard]] f32 getCollisionRadius() const override {
        return 245.0f * static_cast<f32>(m_mapObj->setting(2));
    }

    /// @addr{0x8067FBB8}
    /// @copydoc ObjectKCL::getUpdatedMatrix()
    [[nodiscard]] const EGG::Matrix34f &getUpdatedMatrix(u32 /*timeOffset*/) override {
        calcTransform();
        return transform();
    }

    [[nodiscard]] f32 getScaleY(u32 timeOffset) const override;
    [[nodiscard]] bool checkCollision(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info,
            KCLTypeMask *maskOut, u32 timeOffset) override;
    [[nodiscard]] bool checkCollisionCached(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info,
            KCLTypeMask *maskOut, u32 timeOffset) override;

private:
    /// @addr{0x8067F2DC}
    /// @brief Runs once when the geyser begins to expand
    /// @details Resets @ref m_heightOffset to `0.0f`.
    void enterExpanding() {
        m_heightOffset = 0.0f;
    }

    /// @addr{0x8067F2F4}
    /// @brief Runs once when the geyser begins raising the flame pole
    /// @details Sets the pole to active, enables its collision, and resets @ref m_heightOffset to
    /// zero.
    /// @note Normally @ref m_initEruptVel and @ref m_eruptAccel are computed in this function.
    /// However, since these values do not change, we instead set them in @ref init().
    void enterEruptingUp() {
        m_pole->setActive(true);
        m_pole->enableCollision();
        m_heightOffset = 0.0f;
    }

    /// @addr{0x8067F538}
    /// @brief Runs once when the fire pole has reached the peak of its eruption
    /// @details Sets @ref m_eruptedHeightOffset to @ref m_maxHeight to cache the height of the
    /// flame pole so that it can oscillate around this height.
    void enterEruptingStay() {
        m_eruptedHeightOffset = m_maxHeight;
    }

    /// @addr{0x8067F5EC}
    /// @brief Runs once when the fire pole begins descending
    /// @details Sets @ref m_eruptedHeightOffset to the current @ref m_heightOffset to cache the
    /// height of the flame pole before it starts descending.
    void enterEruptingDown() {
        m_eruptedHeightOffset = m_heightOffset;
    }

    /// @addr{0x8067F650}
    /// @brief Runs once when the geyser finishes contracting
    /// @details Deactivates the flame pole and disables its collision.
    void enterDormant() {
        m_pole->setActive(false);
        m_pole->disableCollision();
    }

    /// @addr{0x8067F484}
    /// @brief Runs every frame when the flame pole is raising up
    /// @details Updates @ref m_heightOffset based on the current frame and the eruption velocity
    /// and acceleration.
    void calcEruptingUp() {
        f32 frame = static_cast<f32>(m_cycleFrame - STATE_STARTS[1]);
        m_heightOffset =
                std::min(m_maxHeight, m_initEruptVel * frame - frame * 0.5f * m_eruptAccel * frame);
    }

    /// @addr{0x8067F544}
    /// @brief Runs every frame after the flame pole reaches max height and before falling to
    /// dormancy
    /// @details Oscillates the flame pole around its peak height using a sinusoidal function with
    /// an amplitude of `50.0f` and a period of 60 frames.
    void calcEruptingStay() {
        constexpr f32 HALF_PERIOD = 30.0f;
        constexpr f32 AMPLITUDE = 50.0f;

        f32 angle = 360.0f * static_cast<f32>(m_cycleFrame - STATE_STARTS[2]) / HALF_PERIOD;
        m_heightOffset = m_eruptedHeightOffset + AMPLITUDE * EGG::Mathf::SinFIdx(DEG2FIDX * angle);
    }

    /// @addr{0x8067F604}
    /// @brief Runs every frame while the flame pole has finished erupting and is descending
    /// @details Updates @ref m_heightOffset based on the cached erupted height and @ref
    /// m_scaleDelta.
    void calcEruptingDown() {
        m_heightOffset = m_eruptedHeightOffset -
                m_scaleDelta * static_cast<f32>(m_cycleFrame - STATE_STARTS[3]);
    }

    void calcStates();

    /// @addr{0x8067F7C8}
    /// @brief Calculates the height and scale of the flame pole based on the current cycle frame
    /// @details The pole's y-position is updated based on @ref m_heightOffset, and the pole's scale
    /// is set to @ref m_maxScale.
    void calcHeightAndScale() {
        setScale(getScaleY(0));
        EGG::Vector3f polePos = m_pole->pos();
        m_pole->setPos(
                EGG::Vector3f(polePos.x, m_heightOffset + (pos().y - m_maxHeight), polePos.z));
        m_pole->setScale(m_maxScale);
    }

    void initEruptKinematics();

    ObjectFlamePole *m_pole;      ///< Pointer to the associated flamepole
    const u32 m_extraCycleFrames; ///< Additional dormancy frames
    const u32 m_initDelay;        //< Frame delay before the state lifecycle begins
    f32 m_maxScale;               ///< Size/speed of the geyser hump during eruption
    f32 m_eruptDownVel;           ///< Change in scale when the pole is descending
    f32 m_maxHeight;              ///< Peak height of pole's eruption
    s32 m_cycleFrame;             ///< Current frame within lifecycle
    f32 m_heightOffset;           ///< Cyclical offset that adjusts the pole's height
    f32 m_eruptedHeightOffset;    ///< Base height offset for sinusoidal stay and descending motion
    f32 m_scaleDelta;             ///< Change in scale per frame at end of cycle
    f32 m_eruptAccel;             ///< Deceleration applied to height offset as the pole erupts
    f32 m_initEruptVel;           ///< Initial velocity when the pole erupts

    /// @addr{0x809C21E0}
    /// @brief Global variable that tracks the number of flamepole instances
    /// @details If the size factor (third param setting) for a flame pole is set to 0, then the
    /// flame pole's scale is computed as 3 + (s_flamePoleCount % 3)
    static u32 s_flamePoleCount;

    /// @brief Duration for a full lifecycle of erupting, descending, and dormancy
    static constexpr u32 CYCLE_DURATION = 540;

    /// @brief The numerator for fractional state boundary timing in @ref StateFrameBoundaries()
    static constexpr f32 NORMALIZATION = static_cast<f32>(CYCLE_DURATION) * (7.0f - 1.0f);

    /// @brief Framecount the pole spends erupting upwards
    static constexpr s32 ERUPT_UP_DURATION = static_cast<s32>(0.1f * NORMALIZATION / 7.0f);

    /// @brief Framecount the pole spends lowering into dormancy
    static constexpr s32 ERUPT_DOWN_DURATION = static_cast<s32>(0.2f * NORMALIZATION / 7.0f);

    /// @brief The enter and calc functions for each @ref StateManager entry
    static constexpr std::array<StateManagerEntry, 6> STATE_ENTRIES = {{
            {StateEntry<ObjectFlamePoleFoot, &ObjectFlamePoleFoot::enterExpanding, nullptr>(0)},
            {StateEntry<ObjectFlamePoleFoot, &ObjectFlamePoleFoot::enterEruptingUp,
                    &ObjectFlamePoleFoot::calcEruptingUp>(1)},
            {StateEntry<ObjectFlamePoleFoot, &ObjectFlamePoleFoot::enterEruptingStay,
                    &ObjectFlamePoleFoot::calcEruptingStay>(2)},
            {StateEntry<ObjectFlamePoleFoot, &ObjectFlamePoleFoot::enterEruptingDown,
                    &ObjectFlamePoleFoot::calcEruptingDown>(3)},
            {StateEntry<ObjectFlamePoleFoot, &ObjectFlamePoleFoot::enterDormant, nullptr>(4)},
            {StateEntry<ObjectFlamePoleFoot, nullptr, nullptr>(5)},
    }};

    /// @brief Start frame for intervals of the piecewise state function
    static constexpr std::array<s32, 6> STATE_STARTS = FlamePoleFootStateFrameBoundaries();
};

} // namespace Kinoko::Field
