#pragma once

#include "game/field/ObjectCollisionCylinder.hh"
#include "game/field/StateManager.hh"
#include "game/field/obj/ObjectCollidable.hh"

namespace Field {

class ObjectPile;
class ObjectWanwan;
class ObjectWanwanChain;

template <>
class StateManager<ObjectWanwan> : public StateManagerBase<ObjectWanwan> {
public:
    StateManager(ObjectWanwan *obj);
    ~StateManager() override;

private:
    static const std::array<StateManagerEntry<ObjectWanwan>, 7> STATE_ENTRIES;
};

class ObjectWanwan final : public ObjectCollidable, public StateManager<ObjectWanwan> {
    friend StateManager<ObjectWanwan>;

public:
    ObjectWanwan(const System::MapdataGeoObj &params);
    ~ObjectWanwan() override;

    void init() override;
    void calc() override;

    /// @addr{0x806E94BC}
    [[nodiscard]] u32 loadFlags() const override {
        return 3;
    }

    Kart::Reaction onCollision(Kart::KartObject *kartObj, Kart::Reaction reactionOnKart,
            Kart::Reaction reactionOnObj, EGG::Vector3f &hitDepth) override;

private:
    void calcLurch();
    void checkCollision();
    void FUN_806E7638();
    void FUN_806E79E4();

    void enterStateStub();
    void enterState0();
    void enterState1();
    void enterState2();
    void enterState4();
    void enterState5();
    void enterState6();

    void calcStateStub();
    void calcState0();
    void calcState1();
    void calcState2();
    void calcState3();
    void calcState4();
    void calcState5();
    void calcState6();

    /// @addr{0x806B59A8}
    [[nodiscard]] static f32 FUN_806B59A8(f32 param1, f32 param2, u32 param3) {
        return param1 * static_cast<f32>(param3) -
                param2 * 0.5f * static_cast<f32>(param3) * static_cast<f32>(param3);
    }

    /// @addr{0x806B38A8}
    [[nodiscard]] static f32 FUN_806B38A8(const EGG::Vector3f &param1, const EGG::Vector3f &param2,
            const EGG::Vector3f &param3) {
        return (param3.x - param2.x) * (param1.z - param2.z) -
                (param1.x - param2.x) * (param3.z - param2.z);
    }

    /// @addr{0x806B3900}
    [[nodiscard]] static EGG::Vector3f FUN_806B3900(f32 param1, const EGG::Vector3f &v) {
        f32 sin = EGG::Mathf::SinFIdx(RAD2FIDX * 0.5f * param1);
        f32 cos = EGG::Mathf::CosFIdx(RAD2FIDX * 0.5f * param1);

        EGG::Quatf quat = EGG::Quatf(param1, 0.0f, sin, 0.0f);
        EGG::Vector3f local_3c = EGG::Vector3f(v.x, 0.0f, v.z);
        return quat.rotateVector(local_3c);
    }

    /// @addr{0x806E7EDC}
    [[nodiscard]] static EGG::Vector3f Interpolate(f32 param1, const EGG::Vector3f &v0,
            const EGG::Vector3f &v1) {
        return v0 + (v1 - v0) * param1;
    }

    std::span<ObjectWanwanChain *> m_chain;
    ObjectPile *m_pile;
    EGG::Vector3f m_velocity;
    EGG::Vector3f m_accel;
    f32 m_10c;
    f32 m_110;
    EGG::Vector3f m_tangent;
    EGG::Vector3f m_120;
    EGG::Vector3f m_floorNrm;
    f32 m_lurchDistance;
    EGG::Vector3f m_450;
    EGG::Vector3f m_468;
    EGG::Vector3f m_474;
    EGG::Vector3f m_initPos;
    bool m_bTouchingFloor;
    bool m_494;
    f32 m_49c;
    EGG::Vector3f m_4a0;
    EGG::Vector3f m_4ac;
    bool m_4b8;
    f32 m_4bc;
    f32 m_4c8;
    f32 m_4cc;
    u32 m_4d4;
    bool m_4d8;
    u32 m_idleFrames; ///< How long the chain chomp bounces for before lurching
    f32 m_4e8;
    s32 m_4f8;
    f32 m_4fc;
    f32 m_508;
    bool m_518;
};

class ObjectWanwanChain final : public ObjectCollidable {
public:
    ObjectWanwanChain(const System::MapdataGeoObj &params) : ObjectCollidable(params) {}

    /// @addr{0x806E94FC}
    ~ObjectWanwanChain() override = default;

    /// @addr{0x806E4A64}
    void init() override {
        m_b0 = 0.0f;
        m_b4 = 0.0f;
        m_b8 = 0.0f;
        m_bc = 1.0f;
    }

    /// @addr{0x806E4208}
    void calc() override {
        disableCollision();
    }

    /// @addr{0x806E94DC}
    [[nodiscard]] const char *getName() const override {
        return "wanwan_chn";
    }

    /// @addr{0x806E94F4}
    [[nodiscard]] u32 loadFlags() const override {
        return 1;
    }

    /// @addr{0x806E94E8}
    [[nodiscard]] const char *getKclName() const override {
        return "wanwan_chn";
    }

    /// @addr{0x806E41A4}
    void createCollision() override {
        m_collision = new ObjectCollisionCylinder(30.0f, 0.5f, EGG::Vector3f::zero);
    }

    /// @addr{0x806E4220}
    void calcCollisionTransform() override {}

    /// @addr{0x806E94C4}
    [[nodiscard]] f32 getCollisionRadius() const override {
        constexpr f32 TWO_ROOT_TWO = 1.414f;

        return TWO_ROOT_TWO * 135.0f;
    }

    /// @addr{0x806E4218}
    Kart::Reaction onCollision(Kart::KartObject * /*kartObj*/, Kart::Reaction /*reactionOnKart*/,
            Kart::Reaction /*reactionOnObj*/, EGG::Vector3f & /*hitDepth*/) override {
        return Kart::Reaction::None;
    }

    [[nodiscard]] f32 b0() const {
        return m_b0;
    }

private:
    f32 m_b0;
    f32 m_b4;
    f32 m_b8;
    f32 m_bc;
};

} // namespace Field
