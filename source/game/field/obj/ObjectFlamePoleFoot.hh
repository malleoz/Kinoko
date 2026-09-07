#pragma once

#include "game/field/StateManager.hh"
#include "game/field/obj/ObjectFlamePole.hh"
#include "game/field/obj/ObjectKCL.hh"

#include "game/system/RaceManager.hh"

namespace Kinoko::Field {

class ObjectFlamePole;

/// @brief Represents the expanding geyser humps at the end of Bowser's Castle.
/// @details Each "foot" owns its own @ref ObjectFlamePole. The foot controls updates the pole's
/// scale to effectively cause the pole to appear to erupt out of the ground. The pole erupts
/// upwards, dips a bit, rises back up, and then descends back beneath the foot.
class ObjectFlamePoleFoot final : public ObjectKCL, private StateManager {
    /// @brief Grants access to the singleton so a @ref Host::Context can restore the instance's
    /// state on context switch
    friend class Host::Context;

public:
    ObjectFlamePoleFoot(const System::MapdataGeoObj &params);
    ~ObjectFlamePoleFoot() override;

    void init() override;

    /// @addr{0x8067EF70}
    /// @copybrief ObjectBase::calc()
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
    [[nodiscard]] f32 getCollisionRadius() const override {
        return 245.0f * static_cast<f32>(m_mapObj->setting(2));
    }

    /// @addr{0x8067FBB8}
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
    void enterStateStub() {}

    /// @addr{0x8067F2DC}
    /// @brief Runs once when the geyser begins to expand
    void enterExpanding() {
        m_heightOffset = 0.0f;
    }

    void enterEruptingUp();

    /// @addr{0x8067F538}
    /// @brief Runs once when the fire pole has reached the peak of its eruption
    void enterEruptingStay() {
        m_eruptedHeightOffset = m_maxHeight;
    }

    /// @addr{0x8067F5EC}
    /// @brief Runs once when the fire pole begins descending
    void enterEruptingDown() {
        m_eruptedHeightOffset = m_heightOffset;
    }

    /// @addr{0x8067F650}
    /// @brief Runs once when the geyser finishes contracting
    void enterDormant() {
        m_pole->setActive(false);
        m_pole->disableCollision();
    }

    void calcStateStub() {}

    /// @addr{0x8067F484}
    /// @brief Runs every frame when the flame pole is raising up
    void calcEruptingUp() {
        f32 frame = static_cast<f32>(m_cycleFrame - m_stateStart[1]);
        m_heightOffset =
                std::min(m_maxHeight, m_initEruptVel * frame - frame * 0.5f * m_eruptAccel * frame);
    }

    /// @addr{0x8067F544}
    /// @brief Runs every frame after the flame pole reaches max height and before falling to
    /// dormancy
    void calcEruptingStay() {
        constexpr f32 AMPLITUDE = 50.0f;

        f32 angle = 360.0f * static_cast<f32>(m_cycleFrame - m_stateStart[2]) / 30.0f;
        m_heightOffset = m_eruptedHeightOffset + AMPLITUDE * EGG::Mathf::SinFIdx(DEG2FIDX * angle);
    }

    /// @addr{0x8067F604}
    /// @brief Runs every frame while the flame pole has finished erupting and is descending
    void calcEruptingDown() {
        m_heightOffset = m_eruptedHeightOffset -
                m_scaleDelta * static_cast<f32>(m_cycleFrame - m_stateStart[3]);
    }

    void calcStates();

    /// @addr{0x8067F7C8}
    /// @brief Calculates the height and scale of the flame pole based on the current cycle frame
    void calcHeightAndScale() {
        setScale(getScaleY(0));
        EGG::Vector3f polePos = m_pole->pos();
        m_pole->setPos(
                EGG::Vector3f(polePos.x, m_heightOffset + (pos().y - m_maxHeight), polePos.z));
        m_pole->setScale(m_maxScale);
    }

    ObjectFlamePole *m_pole;         ///< Pointer to the associated flamepole
    const u32 m_extraCycleFrames;    ///< Additional dormancy frames
    const u32 m_initDelay;           //< Frame delay before the state lifecycle begins
    f32 m_maxScale;                  ///< Size/speed of the geyser hump during eruption
    s32 m_eruptUpDuration;           ///< Framecount the flame pole spends erupting upwards
    s32 m_eruptDownDuration;         ///< Framecount the flame pole spends lowering into dormancy
    std::array<s32, 6> m_stateStart; ///< Start frame for intervals of the piecewise state function
    f32 m_eruptDownVel;              ///< Change in scale when the pole is descending
    f32 m_maxHeight;                 ///< Peak height of pole's eruption
    s32 m_cycleFrame;                ///< Current frame within lifecycle
    f32 m_heightOffset;              ///< Cyclical offset that adjusts the pole's height
    f32 m_eruptedHeightOffset; ///< Base height offset for sinusoidal stay and descending motion
    f32 m_scaleDelta;          ///< Change in scale per frame at end of cycle
    f32 m_eruptAccel;          ///< Deceleration applied to height offset as the pole erupts
    f32 m_initEruptVel;        ///< Initial velocity when the pole erupts

    /// @addr{0x809C21E0}
    /// @brief Global variable that tracks the number of flamepole instances
    /// @details If the size factor (third param setting) for a flame pole is set to 0, then the
    /// flame pole's scale is computed as 3 + (s_flamePoleCount % 3)
    static u32 s_flamePoleCount;

    /// @brief The enter and calc functions for each @ref StateManager entry
    static constexpr std::array<StateManagerEntry, 6> STATE_ENTRIES = {{
            {StateEntry<ObjectFlamePoleFoot, &ObjectFlamePoleFoot::enterExpanding,
                    &ObjectFlamePoleFoot::calcStateStub>(0)},
            {StateEntry<ObjectFlamePoleFoot, &ObjectFlamePoleFoot::enterEruptingUp,
                    &ObjectFlamePoleFoot::calcEruptingUp>(1)},
            {StateEntry<ObjectFlamePoleFoot, &ObjectFlamePoleFoot::enterEruptingStay,
                    &ObjectFlamePoleFoot::calcEruptingStay>(2)},
            {StateEntry<ObjectFlamePoleFoot, &ObjectFlamePoleFoot::enterEruptingDown,
                    &ObjectFlamePoleFoot::calcEruptingDown>(3)},
            {StateEntry<ObjectFlamePoleFoot, &ObjectFlamePoleFoot::enterDormant,
                    &ObjectFlamePoleFoot::calcStateStub>(4)},
            {StateEntry<ObjectFlamePoleFoot, &ObjectFlamePoleFoot::enterStateStub,
                    &ObjectFlamePoleFoot::calcStateStub>(5)},
    }};

    /// @brief Duration for a full lifecycle of erupting, descending, and dormancy
    static constexpr u32 CYCLE_DURATION = 540;
};

} // namespace Kinoko::Field
