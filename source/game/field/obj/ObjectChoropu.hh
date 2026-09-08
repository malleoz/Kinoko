#pragma once

#include "game/field/ObjectCollisionSphere.hh"
#include "game/field/StateManager.hh"
#include "game/field/obj/ObjectCollidable.hh"

#include "game/kart/KartCollide.hh"

namespace Kinoko::Field {

class ObjectChoropuGround;
class ObjectChoropuHoll;

/// @brief Represents the MMM and rPG monty moles.
/// @details Each mole has an associated "holl" [sic]. Moles which move around (MMM) also have an
/// associated "ground" (the dirt trail).
class ObjectChoropu final : public ObjectCollidable, private StateManager {
public:
    ObjectChoropu(const System::MapdataGeoObj &params);
    ~ObjectChoropu() override;

    void init() override;
    void calc() override;

    /// @addr{0x806BBE34}
    /// @copybrief ObjectBase::loadFlags()
    /// @return Returns @ref eLoadFlags::Calc, so that object is calculated every frame.
    [[nodiscard]] LoadFlags loadFlags() const override {
        return LoadFlags(eLoadFlags::Calc);
    }

    /// @addr{0x806BA144}
    Kart::Reaction onCollision(Kart::KartObject * /*kartObj*/, Kart::Reaction reactionOnKart,
            Kart::Reaction /*reactionOnObj*/, EGG::Vector3f & /*hitDepth*/) override {
        return m_currentStateId == 1 ? Kart::Reaction::SmallBump : reactionOnKart;
    }

private:
    void enterDigging();
    void enterPeeking();

    /// @addr{0x806BB39C}
    /// @brief Runs once when the mole jumps out of its hole
    void enterJumping() {
        enableCollision();

        setPos(m_isStationary ? m_transMat.base(3) : m_railMat.base(3));
        setRot(EGG::Vector3f(rot().x, rot().y, 0.0f));
    }

    void calcDigging();
    void calcPeeking();
    void calcJumping();

    void calcGround();
    void calcGroundObjs();
    [[nodiscard]] EGG::Matrix34f calcInterpolatedPose(f32 t) const;

    /// @addr{0x806BBB14}
    /// @brief Calculates the current height of the mole in its parabolic jump curve
    /// @details Follows a parabolic trajectory defined by
    /// \f$ y = -1.35t^2 + 65.0t \f$
    /// where \f$t\f$ is the current frame of the jump.
    [[nodiscard]] f32 calcJumpHeight() const {
        constexpr f32 JUMP_LINEAR_COEFFICIENT = 65.0f;
        constexpr f32 JUMP_QUADRATIC_COEFFICIENT = 2.7f;

        return JUMP_LINEAR_COEFFICIENT * static_cast<f32>(m_currentFrame) -
                static_cast<f32>(m_currentFrame) * 0.5f * JUMP_QUADRATIC_COEFFICIENT *
                static_cast<f32>(m_currentFrame);
    }

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

    /// @brief The enter and calc functions for each @ref StateManager entry
    static constexpr std::array<StateManagerEntry, 5> STATE_ENTRIES = {{
            StateEntry<ObjectChoropu, &ObjectChoropu::enterDigging, &ObjectChoropu::calcDigging>(0),
            StateEntry<ObjectChoropu, &ObjectChoropu::enterPeeking, &ObjectChoropu::calcPeeking>(1),
            StateEntry<ObjectChoropu, nullptr,
                    nullptr>(2),
            StateEntry<ObjectChoropu, &ObjectChoropu::enterJumping, &ObjectChoropu::calcJumping>(3),
            StateEntry<ObjectChoropu, nullptr,
                    nullptr>(4),
    }};
};

/// @brief A dirt trail segment left behind by the moving monty moles on Moo Moo Meadows
class ObjectChoropuGround final : public ObjectCollidable {
public:
    ObjectChoropuGround(const EGG::Vector3f &pos, const EGG::Vector3f &rot,
            const EGG::Vector3f &scale);
    ~ObjectChoropuGround() override;

    /// @addr{0x806BBEB0}
    /// @copybrief ObjectBase::loadFlags()
    /// @return Returns @ref eLoadFlags::Calc, so that object is calculated every frame.
    [[nodiscard]] LoadFlags loadFlags() const override {
        return LoadFlags(eLoadFlags::Calc);
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
class ObjectChoropuHoll final : public ObjectCollidable {
public:
    ObjectChoropuHoll(const System::MapdataGeoObj &params);
    ~ObjectChoropuHoll() override;

    /// @addr{0x806B94A0}
    /// @copybrief ObjectBase::init()
    void init() override {
        resize(RADIUS, 0.0f);
    }

    /// @addr{0x806BBE4C}
    /// @copybrief ObjectBase::getName()
    /// @return The name of the object, `holl`
    [[nodiscard]] const char *getName() const override {
        return "holl";
    }

    /// @addr{0x806BBE64}
    /// @copybrief ObjectBase::loadFlags()
    /// @return Returns @ref eLoadFlags::Calc, so that object is calculated every frame.
    [[nodiscard]] LoadFlags loadFlags() const override {
        return LoadFlags(eLoadFlags::Calc);
    }

    /// @addr{0x806BBE58}
    /// @copybrief ObjectBase::getKclName()
    /// @return The name of the hole object, `holl`
    [[nodiscard]] const char *getKclName() const override {
        return "holl";
    }

    /// @addr{0x806B9428}
    /// @copybrief ObjectBase::createCollision()
    void createCollision() override {
        m_collision = EGG::egg_new<ObjectCollisionSphere>(RADIUS, EGG::Vector3f::zero);
    }

    /// @addr{0x806BBE40}
    /// @copybrief ObjectBase::loadRail()
    /// @details no-op because the hole is managed by the @ref ObjectChoropu, not a rail.
    void loadRail() override {}

    /// @addr{0x806B9594}
    Kart::Reaction onCollision(Kart::KartObject * /*kartObj*/, Kart::Reaction /*reactionOnKart*/,
            Kart::Reaction /*reactionOnObj*/, EGG::Vector3f &hitDepth) override {
        hitDepth.setZero();
        return Kart::Reaction::Offroad;
    }

private:
    static constexpr f32 RADIUS = 300.0f; ///< Radius of the collision sphere for the mole's hole
};

} // namespace Kinoko::Field
