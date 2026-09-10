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

    /// @addr{0x806B9B8C}
    /// @brief Default virtual destructor
    ~ObjectChoropu() = default;

    void init() override;
    void calc() override;

    /// @addr{0x806BBE34}
    /// @copybrief ObjectBase::loadFlags()
    /// @return Returns @ref eLoadFlags::Calc, so that object is calculated every frame.
    [[nodiscard]] LoadFlags loadFlags() const override {
        return LoadFlags(eLoadFlags::Calc);
    }

    /// @addr{0x806BA144}
    /// @copybrief ObjectCollidable::onCollision
    /// @param reactionOnKart The reaction that should be applied to the kart upon collision
    /// @return @ref Kart::Reaction::SmallBump if the mole is poking its head out, otherwise @ref
    /// Kart::Reaction::SmallLaunch.
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

        setPos(m_isStationary ? m_initRt.base(3) : m_railMat.base(3));
        setRot(EGG::Vector3f(rot().x, rot().y, 0.0f));
    }

    void calcDigging();
    void calcPeeking();
    void calcJumping();

    /// @addr{0x806BBA7C}
    /// @brief Calculates the total length of the dirt trail behind the monty moles on MMM
    void calcGround() {
        m_groundLength += m_railInterpolator->getCurrVel();
        if (m_groundLength > MAX_GROUND_LEN) {
            m_groundLength = MAX_GROUND_LEN - 1.0f;
        }

        calcGroundObjs();
    }

    void calcGroundObjs();

    /// @addr{0x806B46F8}
    /// @brief Calculates position and rotation along the bezier curve of the rail at a given t
    [[nodiscard]] EGG::Matrix34f calcInterpolatedPose(f32 t) const {
        EGG::Vector3f curDir;
        EGG::Vector3f curTanDir;
        m_railInterpolator->evalPositionAndTangentBehind(t, curDir, curTanDir);
        EGG::Matrix34f mat = OrthonormalBasis(curTanDir);
        mat.setBase(3, curDir);
        return mat;
    }

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
    const s16 m_startFrameOffset; ///< Initial delay before the mole starts its behavior cycle
    const u16 m_idleDuration;     ///< Frames that stationary moles stay underground before peeking
    f32 m_groundHeight; ///< Height of a single dirt trail cylinder. Used to space out segments.
    const bool m_isStationary; ///< rPG moles don't move while MMM moles do
    EGG::Matrix34f m_initRt;   ///< Initial transform for statinary moles
    EGG::Matrix34f m_railMat;  ///< Orthonormal basis for moving mole's current pos and orientation
    f32 m_groundLength;        ///< Cumulative length of dirt trail behind moving monty moles on MMM

    static constexpr f32 RADIUS = 300.0f; ///< Radius of the dirt trail segments' collision sphere
    static constexpr f32 MAX_GROUND_LEN = 3000.0f; ///< Max length of the dirt trail

    /// @brief The enter and calc functions for each @ref StateManager entry
    static constexpr std::array<StateManagerEntry, 5> STATE_ENTRIES = {{
            StateEntry<ObjectChoropu, &ObjectChoropu::enterDigging, &ObjectChoropu::calcDigging>(0),
            StateEntry<ObjectChoropu, &ObjectChoropu::enterPeeking, &ObjectChoropu::calcPeeking>(1),
            StateEntry<ObjectChoropu, nullptr, nullptr>(2),
            StateEntry<ObjectChoropu, &ObjectChoropu::enterJumping, &ObjectChoropu::calcJumping>(3),
            StateEntry<ObjectChoropu, nullptr, nullptr>(4),
    }};
};

/// @brief A dirt trail segment left behind by the moving monty moles on Moo Moo Meadows
class ObjectChoropuGround final : public ObjectCollidable {
public:
    ObjectChoropuGround(const EGG::Vector3f &pos, const EGG::Vector3f &rot,
            const EGG::Vector3f &scale);

    /// @addr{0x806BBE6C}
    /// @brief Default virtual destructor
    ~ObjectChoropuGround() override = default;

    /// @addr{0x806BBEB0}
    /// @copybrief ObjectBase::loadFlags()
    /// @return Returns @ref eLoadFlags::Calc, so that object is calculated every frame.
    [[nodiscard]] LoadFlags loadFlags() const override {
        return LoadFlags(eLoadFlags::Calc);
    }

    /// @addr{0x806B9164}
    /// @copybrief ObjectCollidable::onCollision
    /// @param reactionOnKart The reaction that should be applied to the kart upon collision
    /// @param hitDepth The depth of the collision between the kart and the object
    /// @return @ref Kart::Reaction::Offroad
    /// @details Also sets the hit depth to zero.
    Kart::Reaction onCollision(Kart::KartObject * /*kartObj*/, Kart::Reaction reactionOnKart,
            Kart::Reaction /*reactionOnObj*/, EGG::Vector3f &hitDepth) override {
        hitDepth.setZero();
        return reactionOnKart;
    }

    /// @addr{0x806B9274}
    /// @brief Sets the ground object's transformation matrix based off of the provided pose
    void calcPosAndMat(f32 height, const EGG::Matrix34f &mat) {
        EGG::Matrix34f matTemp;
        SetRotTangentHorizontal(matTemp, mat.base(2), EGG::Vector3f::ey);
        matTemp.setBase(1, matTemp.base(1) * (height / m_height));
        matTemp.setBase(3, mat.base(3));
        setTransform(matTemp);
    }

    [[nodiscard]] f32 height() const {
        return m_height;
    }

private:
    f32 m_height; ///< Height of the dirt trail segment. Used to space out segments.
};

/// @brief The hole that the monty mole pops out from
class ObjectChoropuHoll final : public ObjectCollidable {
public:
    /// @addr{0x806B93CC}
    /// @brief Constructor
    /// @param params The parameters used to initialize the object
    ObjectChoropuHoll(const System::MapdataGeoObj &params) : ObjectCollidable(params) {}

    /// @addr{0x806BBE6C}
    /// @brief Default virtual destructor
    ~ObjectChoropuHoll() override = default;

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
    /// @copybrief ObjectCollidable::onCollision
    /// @param reactionOnKart The reaction that should be applied to the kart upon collision
    /// @param hitDepth The depth of the collision between the kart and the object
    /// @return @ref Kart::Reaction::Offroad
    /// @details Also sets the hit depth to zero.
    Kart::Reaction onCollision(Kart::KartObject * /*kartObj*/, Kart::Reaction /*reactionOnKart*/,
            Kart::Reaction /*reactionOnObj*/, EGG::Vector3f &hitDepth) override {
        hitDepth.setZero();
        return Kart::Reaction::Offroad;
    }

private:
    static constexpr f32 RADIUS = 300.0f; ///< Radius of the collision sphere for the mole's hole
};

} // namespace Kinoko::Field
