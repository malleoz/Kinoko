#pragma once

#include "game/field/ObjectCollisionSphere.hh"
#include "game/field/StateManager.hh"
#include "game/field/obj/ObjectCollidable.hh"

#include <vector>

namespace Field {

class ObjectChoropu;
class ObjectChoropuGround;
class ObjectChoropuHoll;

template <>
class StateManager<ObjectChoropu> : public StateManagerBase<ObjectChoropu> {
public:
    StateManager(ObjectChoropu *obj);
    ~StateManager() override;

private:
    static const std::array<StateManagerEntry<ObjectChoropu>, 5> STATE_ENTRIES;
};

/// @brief Represents the MMM and rPG monty moles.
/// @details Each mole has an associated "holl" [sic]. Moles which move around (MMM) also have an
/// associated "ground" (the dirt trail).
class ObjectChoropu : public ObjectCollidable, public StateManager<ObjectChoropu> {
    friend StateManager<ObjectChoropu>;

public:
    ObjectChoropu(const System::MapdataGeoObj &params);
    ~ObjectChoropu() override;

    void init() override;
    void calc() override;

    /// @addr{0x806BBE34}
    [[nodiscard]] u32 loadFlags() const override {
        return 1;
    }

    void loadAnims() override;

    Kart::Reaction onCollision(Kart::KartObject *kartObj, Kart::Reaction reactionOnKart,
            Kart::Reaction reactionOnObj, EGG::Vector3f &hitDepth) override;

private:
    void enterStateStub();
    void enterState0();
    void enterState1();
    void enterState3();
    void calcStateStub();
    void calcState0();
    void calcState1();
    void calcState3();

    void calcGround();
    [[nodiscard]] EGG::Matrix34f FUN_806B46F8(f32 t) const;

    std::vector<ObjectChoropuGround *> m_groundObjs;
    ObjectChoropuHoll *m_objHoll;
    s16 m_startFrameOffset;
    f32 m_e4;
    f32 m_e8;
    u16 m_idleDuration;
    f32 m_groundHeight;
    bool m_isStationary; ///< rPG moles don't move while MMM moles do
    EGG::Matrix34f m_transMat;
    EGG::Matrix34f m_railMat;
    bool m_isColliding;
    f32 m_164;
    u32 m_17c;

    static constexpr f32 M_SPEED_RELATED = 3000.0f;
};

class ObjectChoropuGround : public ObjectCollidable {
public:
    ObjectChoropuGround(const EGG::Vector3f &pos, const EGG::Vector3f &rot,
            const EGG::Vector3f &scale);
    ~ObjectChoropuGround() override;

    /// @addr{0x806BBEB0}
    [[nodiscard]] u32 loadFlags() const override {
        return 1;
    }

    void calc(f32 t, const EGG::Matrix34f &mat);

    [[nodiscard]] f32 height() const {
        return m_height;
    }

private:
    f32 m_height;
};

class ObjectChoropuHoll : public ObjectCollidable {
public:
    ObjectChoropuHoll(const System::MapdataGeoObj &params);
    ~ObjectChoropuHoll() override;

    /// @addr{0x806B94A0}
    void init() override {
        resize(300.0f, 0.0f);
    }

    /// @addr{0x806BBE4C}
    [[nodiscard]] const char *getName() const override {
        return "holl";
    }

    /// @addr{0x806BBE64}
    [[nodiscard]] u32 loadFlags() const override {
        return 1;
    }

    /// @addr{0x806BBE58}
    [[nodiscard]] const char *getKclName() const override {
        return "holl";
    }

    /// @addr{0x806B9428}
    void createCollision() override {
        static constexpr f32 COLLISION_RADIUS = 300.0f;
        m_collision = new ObjectCollisionSphere(COLLISION_RADIUS, EGG::Vector3f::zero);
    }

    /// @addr{0x806BBE40}
    void loadRail() override {}
};

} // namespace Field
