#pragma once

#include "game/field/StateManager.hh"
#include "game/field/obj/ObjectFlamePole.hh"
#include "game/field/obj/ObjectKCL.hh"

namespace Field {

class ObjectFlamePole;

class ObjectFlamePoleFoot final : public ObjectKCL, public StateManager<ObjectFlamePoleFoot> {
    friend StateManager<ObjectFlamePoleFoot>;

public:
    ObjectFlamePoleFoot(const System::MapdataGeoObj &params);
    ~ObjectFlamePoleFoot() override;

    void init() override;
    void calc() override;

    /// @addr{0x80681590}
    [[nodiscard]] u32 loadFlags() const override {
        return 1;
    }

    /// @addr{0x806814C4}
    [[nodiscard]] f32 getCollisionRadius() const override {
        return 245.0f * static_cast<f32>(m_mapObj->setting(2));
    }

    [[nodiscard]] const EGG::Matrix34f &getUpdatedMatrix(u32 timeOffset) override;
    [[nodiscard]] f32 getScaleY(u32 timeOffset) const override;
    [[nodiscard]] bool checkCollision(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info,
            KCLTypeMask *maskOut, u32 timeOffset) override;
    [[nodiscard]] bool checkCollisionCached(f32 radius, const EGG::Vector3f &pos,
            const EGG::Vector3f &prevPos, KCLTypeMask mask, CollisionInfo *info,
            KCLTypeMask *maskOut, u32 timeOffset) override;

private:
    /// @addr{0x8067F2DC}
    void enterState0() {
        m_1b0 = 0.0f;
    }

    void enterState1();

    /// @addr{0x8067F538}
    void enterState2() {
        m_1b4 = m_1a8;
    }

    /// @addr{0x8067F5EC}
    void enterState3() {
        m_1b4 = m_1b0;
    }

    /// @addr{0x8067F650}
    void enterState4() {
        m_pole->setActive(false);
        m_pole->disableCollision();
    }

    /// @addr{0x8067F6B0}
    void enterState5() {}

    /// @addr{0x8067F2F0}
    void calcState0() {}

    void calcState1();
    void calcState2();

    /// @addr{0x8067F604}
    void calcState3() {
        m_1b0 = m_1b4 - m_1b8 * static_cast<f32>(m_cycleFrame - static_cast<u32>(m_194));
    }

    /// @addr{0x8067F6AC}
    void calcState4() {}

    /// @addr{0x8067F6B4}
    void calcState5() {}

    void calcStates();

    EGG::Matrix34f m_workMatrix;
    ObjectFlamePole *m_pole;
    u32 m_160;       ///< offset 0x160, something timer related. Usually 0??
    u32 m_initDelay; ///< 0x164, frame that the geyser starts calculating
    f32 m_168;       ///< offset 0x168
    s32 m_170;
    s32 m_174;
    s32 m_178;
    s32 m_17c;
    s32 m_180;
    s32 m_18c;
    s32 m_190;
    s32 m_194;
    s32 m_198;
    s32 m_19c;
    f32 m_1a0;
    f32 m_1a8;
    s32 m_cycleFrame; ///< 0x1ac, Frames since @ref m_initDelay, modulo m_160 + 540
    f32 m_1b0;        ///< Might need to be initialized?
    f32 m_1b4;
    f32 m_1b8;
    f32 m_1bc;
    f32 m_1c0;

    static u32 FLAMEPOLE_COUNT;

    static constexpr std::array<StateManagerEntry<ObjectFlamePoleFoot>, 6> STATE_ENTRIES = {{
            {0, &ObjectFlamePoleFoot::enterState0, &ObjectFlamePoleFoot::calcState0},
            {1, &ObjectFlamePoleFoot::enterState1, &ObjectFlamePoleFoot::calcState1},
            {2, &ObjectFlamePoleFoot::enterState2, &ObjectFlamePoleFoot::calcState2},
            {3, &ObjectFlamePoleFoot::enterState3, &ObjectFlamePoleFoot::calcState3},
            {4, &ObjectFlamePoleFoot::enterState4, &ObjectFlamePoleFoot::calcState4},
            {5, &ObjectFlamePoleFoot::enterState5, &ObjectFlamePoleFoot::calcState5},
    }};

    static constexpr u32 DAT_808C0F58 = 540;
};

} // namespace Field
