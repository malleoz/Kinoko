#pragma once

#include "game/field/StateManager.hh"
#include "game/field/obj/ObjectCollidable.hh"

namespace Field {

class ObjectFlamePoleV;

class ObjectFlamePoleV final : public ObjectCollidable, public StateManager<ObjectFlamePoleV> {
    friend StateManager<ObjectFlamePoleV>;

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
    /// @addr{0x806C40E4}
    void enterState0() {
        enableCollision();
    }

    /// @addr{0x806C4178}
    void enterState1() {
        m_10c = 0.0f;
        FUN_806B5A0C(60.0f, m_scaledHeight, m_100, m_fc);
    }

    /// @addr{0x806C4280}
    void enterState2() {}

    /// @addr{0x806C43BC}
    void enterState3() {
        m_110 = m_10c;
        m_104 = m_scaledHeight / 180.0f;
    }

    /// @addr{0x806C4478}
    void enterState4() {
        disableCollision();
    }

    /// @addr{0x806C4130}
    void calcState0() {
        if (static_cast<f32>(m_currentFrame) >= 50.0f) {
            m_nextStateId = 1;
        }
    }

    /// @addr{0x806C41F0}
    void calcState1() {
        if (static_cast<f32>(m_currentFrame) >= 60.0f) {
            m_nextStateId = 2;
        }

        m_10c = FUN_806B59A8(m_100, m_fc, m_currentFrame);
    }

    void calcState2();
    void calcState3();

    /// @addr{0x806C44C8}
    void calcState4() {
        if (m_currentFrame >= m_state4Duration) {
            m_nextStateId = 0;
        }
    }

    /// @addr{0x806B5A0C}
    static void FUN_806B5A0C(f32 param1, f32 param2, f32 &param3, f32 &param4) {
        f32 asdf = 2.0f * param2;
        param3 = asdf / param1;
        param4 = param3 * param3 / asdf;
    }

    u32 m_initDelay;
    u32 m_e4;
    u32 m_state4Duration;
    f32 m_scaleFactor;
    const f32 m_initPosY;
    f32 m_scaledHeight;
    f32 m_fc;
    f32 m_100;
    f32 m_104;
    bool m_isBig;
    f32 m_10c;
    f32 m_110;

    static constexpr std::array<StateManagerEntry<ObjectFlamePoleV>, 5> STATE_ENTRIES = {{
            {0, &ObjectFlamePoleV::enterState0, &ObjectFlamePoleV::calcState0},
            {1, &ObjectFlamePoleV::enterState1, &ObjectFlamePoleV::calcState1},
            {2, &ObjectFlamePoleV::enterState2, &ObjectFlamePoleV::calcState2},
            {3, &ObjectFlamePoleV::enterState3, &ObjectFlamePoleV::calcState3},
            {4, &ObjectFlamePoleV::enterState4, &ObjectFlamePoleV::calcState4},
    }};
};

} // namespace Field
