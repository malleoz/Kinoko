#pragma once

#include "game/field/ObjectCollisionSphere.hh"
#include "game/field/StateManager.hh"
#include "game/field/obj/ObjectCollidable.hh"

#include "game/kart/KartCollide.hh"

#include <vector>

namespace Kinoko::Field {

class ObjectChoropuGround;
class ObjectChoropuHoll;

/// @brief Represents the MMM and rPG monty moles.
/// @details Each mole has an associated "holl" [sic]. Moles which move around (MMM) also have an
/// associated "ground" (the dirt trail).
class ObjectChoropu : public ObjectCollidable, public StateManager {
public:
    ObjectChoropu(const System::MapdataGeoObj &params);
    ~ObjectChoropu() override;

    void init() override;
    void calc() override;

    /// @addr{0x806BBE34}
    [[nodiscard]] u32 loadFlags() const override {
        return 1;
    }

    Kart::Reaction onCollision(Kart::KartObject *kartObj, Kart::Reaction reactionOnKart,
            Kart::Reaction reactionOnObj, EGG::Vector3f &hitDepth) override;

private:
    void enterStateStub() {}
    void enterDigging();
    void enterPeeking();
    void enterJumping();
    void calcStateStub() {}
    void calcDigging();
    void calcPeeking();
    void calcJumping();

    void calcGround();
    void calcGroundObjs();
    [[nodiscard]] EGG::Matrix34f calcInterpolatedPose(f32 t) const;
    [[nodiscard]] f32 calcJumpHeight() const;

    owning_span<ObjectChoropuGround *> m_groundObjs; ///< Dirt trail segments behind moles on MMM
    ObjectChoropuHoll *m_objHoll;                    ///< The hole the mole emerges from
    s16 m_startFrameOffset; ///< Initial delay before the mole starts its behavior cycle
    u16 m_idleDuration;     ///< Frames that stationary moles stay underground before peeking
    f32 m_groundHeight;     ///< Height of a single dirt trail cylinder. Used to space out segments.
    bool m_isStationary;    ///< rPG moles don't move while MMM moles do
    EGG::Matrix34f m_transMat; ///< Initial transform for statinary moles
    EGG::Matrix34f m_railMat;  ///< Orthonormal basis for moving mole's current pos and orientation
    f32 m_groundLength;        ///< Cumulative length of dirt trail behind moving monty moles on MMM

    static constexpr f32 RADIUS = 300.0f; ///< Radius of the dirt trail segments' collision sphere
    static constexpr f32 MAX_GROUND_LEN = 3000.0f; ///< Max length of the dirt trail

    static constexpr std::array<StateManagerEntry, 5> STATE_ENTRIES = {{
            StateEntry<ObjectChoropu, &ObjectChoropu::enterDigging, &ObjectChoropu::calcDigging>(0),
            StateEntry<ObjectChoropu, &ObjectChoropu::enterPeeking, &ObjectChoropu::calcPeeking>(1),
            StateEntry<ObjectChoropu, &ObjectChoropu::enterStateStub,
                    &ObjectChoropu::calcStateStub>(2),
            StateEntry<ObjectChoropu, &ObjectChoropu::enterJumping, &ObjectChoropu::calcJumping>(3),
            StateEntry<ObjectChoropu, &ObjectChoropu::enterStateStub,
                    &ObjectChoropu::calcStateStub>(4),
    }};
};

/// @brief A dirt trail segment left behind by the moving monty moles on Moo Moo Meadows
class ObjectChoropuGround : public ObjectCollidable {
public:
    ObjectChoropuGround(const EGG::Vector3f &pos, const EGG::Vector3f &rot,
            const EGG::Vector3f &scale);
    ~ObjectChoropuGround() override;

    /// @addr{0x806BBEB0}
    [[nodiscard]] u32 loadFlags() const override {
        return 1;
    }

    /// @addr{0x806B9164}
    Kart::Reaction onCollision(Kart::KartObject * /*kartObj*/, Kart::Reaction reactionOnKart,
            Kart::Reaction /*reactionOnObj*/, EGG::Vector3f &hitDepth) override {
        hitDepth.setZero();
        return reactionOnKart;
    }

    void calcPosAndMat(f32 height, const EGG::Matrix34f &mat);

    [[nodiscard]] f32 height() const {
        return m_height;
    }

private:
    f32 m_height; ///< Height of the dirt trail segment. Used to space out segments.
};

/// @brief The hole that the monty mole pops out from
class ObjectChoropuHoll : public ObjectCollidable {
public:
    ObjectChoropuHoll(const System::MapdataGeoObj &params);
    ~ObjectChoropuHoll() override;

    /// @addr{0x806B94A0}
    void init() override {
        resize(RADIUS, 0.0f);
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
        m_collision = EGG::egg_new<ObjectCollisionSphere>(RADIUS, EGG::Vector3f::zero);
    }

    /// @addr{0x806BBE40}
    void loadRail() override {}

    /// @addr{0x806B9594}
    Kart::Reaction onCollision(Kart::KartObject * /*kartObj*/, Kart::Reaction /*reactionOnKart*/,
            Kart::Reaction /*reactionOnObj*/, EGG::Vector3f &hitDepth) override {
        hitDepth.setZero();
        return Kart::Reaction::Wall;
    }

private:
    static constexpr f32 RADIUS = 300.0f; ///< Radius of the collision sphere for the mole's hole
};

} // namespace Kinoko::Field
