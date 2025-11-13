#pragma once

#include "game/field/StateManager.hh"
#include "game/field/obj/ObjectCollidable.hh"

namespace Field {

class ObjectVolcanoBall;

class ObjectVolcanoBall final : public ObjectCollidable, public StateManager<ObjectVolcanoBall> {
    friend class StateManager<ObjectVolcanoBall>;
    friend class ObjectVolcanoBallLauncher;

public:
    ObjectVolcanoBall(f32 param1, f32 param2, f32 param3, const System::MapdataGeoObj &params,
            const EGG::Vector3f &vec);
    ~ObjectVolcanoBall() override;

    void init() override;
    void calc() override;

    /// @addr{0x806E3A7C}
    [[nodiscard]] u32 loadFlags() const override {
        return 1;
    }

    /// @addr{0x806E2CF0}
    Kart::Reaction onCollision(Kart::KartObject * /*kartObj*/, Kart::Reaction reactionOnKart,
            Kart::Reaction /*reactionOnObj*/, EGG::Vector3f & /*hitDepth*/) override {
        return reactionOnKart;
    }

private:
    /// @addr{0x806E2F24}
    void enterState0() {
        init();
    }

    /// @addr{0x806E2F38}
    void enterState1() {
        init();
    }

    /// @addr{0x806E327C}
    void enterState2() {
        ;
    }

    /// @addr{0x806E2F34}
    void calcState0() {}

    void calcState1();
    void calcState2();

    f32 m_initialVel;
    u16 m_state2Duration;
    f32 m_e4;
    f32 m_e8;
    f32 m_ec;
    f32 m_f0;
    f32 m_vel;

    static constexpr std::array<StateManagerEntry<ObjectVolcanoBall>, 3> STATE_ENTRIES = {{
            {0, &ObjectVolcanoBall::enterState0, &ObjectVolcanoBall::calcState0},
            {1, &ObjectVolcanoBall::enterState1, &ObjectVolcanoBall::calcState1},
            {2, &ObjectVolcanoBall::enterState2, &ObjectVolcanoBall::calcState2},
    }};
};

} // namespace Field
