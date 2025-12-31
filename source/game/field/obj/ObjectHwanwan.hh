#pragma once

#include "game/field/StateManager.hh"
#include "game/field/obj/ObjectCollidable.hh"

namespace Field {

class ObjectHwanwanSubB0 : public ObjectCollidable, public StateManager {
    friend class ObjectHwanwan;

public:
    ObjectHwanwanSubB0(const System::MapdataGeoObj &params);
    ~ObjectHwanwanSubB0() override;

    void init() override;
    void calc() override;

    /// @addr{0x806EC7B8
    [[nodiscard]] u32 loadFlags() const override {
        return 1;
    }

    /// @addr{0x806EC7AC}
    [[nodiscard]] const char *getKclName() const override {
        return "wanwan";
    }

    /// @addr{0x806EC7A8}
    void loadRail() override {}

    Kart::Reaction onCollision(Kart::KartObject *kartObj, Kart::Reaction reactionOnKart,
            Kart::Reaction reactionOnObj, EGG::Vector3f &hitDepth) override;

private:
    /// @addr{0x806E9E04}
    void enterState0() {
        m_3fc = 10;
    }

    /// @addr{0x806E9FE0}
    void enterState1() {}

    void enterState2();

    void calcState0();
    void calcState1();
    void calcState2();

    void calc3F8();

    void checkFloorCollision();
    void FUN_806EAAE8();

    /// @addr{0x806EN184}
    void FUN_806EB184() {
        ++m_3fc;
    }

    void setTangent(const EGG::Vector3f &tangent) {
        m_tangent = tangent;
    }

    const EGG::Vector3f m_initPos;
    EGG::Vector3f m_3ac;
    EGG::Vector3f m_3b8;
    EGG::Vector3f m_3c4;
    EGG::Vector3f m_tangent;
    EGG::Vector3f m_3dc;
    EGG::Vector3f m_3e8;
    f32 m_3f4;
    f32 m_3f8;
    s32 m_3fc;
    bool m_400;
    s32 m_404;
    f32 m_408;
    bool m_40c;
    f32 m_410;
    EGG::Vector3f m_414;
    EGG::Vector3f m_420;
    EGG::Vector3f m_42c;
    EGG::Matrix34f m_438;
    EGG::Matrix34f m_468;
    f32 m_498;
    u32 m_49c;

    static constexpr std::array<StateManagerEntry, 3> STATE_ENTRIES = {{
            {StateEntry<ObjectHwanwanSubB0, &ObjectHwanwanSubB0::enterState0,
                    &ObjectHwanwanSubB0::calcState0>(0)},
            {StateEntry<ObjectHwanwanSubB0, &ObjectHwanwanSubB0::enterState1,
                    &ObjectHwanwanSubB0::calcState1>(1)},
            {StateEntry<ObjectHwanwanSubB0, &ObjectHwanwanSubB0::enterState2,
                    &ObjectHwanwanSubB0::calcState2>(2)},
    }};
};

class ObjectHwanwan : public ObjectCollidable {
public:
    ObjectHwanwan(const System::MapdataGeoObj &params);
    ~ObjectHwanwan() override;

    void init() override;
    void calc() override;

    /// @addr{0x806C69B8}
    [[nodiscard]] u32 loadFlags() const override {
        return 1;
    }

    /// @addr{0x806C69B4}
    void loadGraphics() override {}

    /// @addr{0x806C69B0}
    void createCollision() override {}

private:
    void calcState();

    ObjectHwanwanSubB0 *m_subB0;
};

} // namespace Field
