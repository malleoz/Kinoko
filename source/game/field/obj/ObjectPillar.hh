
#pragma once

#include "game/field/obj/ObjectCollidable.hh"
#include "game/field/obj/ObjectKCL.hh"

#include "game/kart/KartCollide.hh"

#include "game/system/RaceManager.hh"

namespace Kinoko::Field {

/// @brief The stationary base of the Dry Dry Ruins pillars
/// @details Simply acts as wall collision.
class ObjectPillarBase final : public ObjectKCL {
public:
    /// @addr{Inlined in 0x807FED80}
    /// @copybrief ObjectKCL::ObjectKCL(const System::MapdataGeoObj &)
    ObjectPillarBase(const System::MapdataGeoObj &params) : ObjectKCL(params) {}

    /// @addr{0x807FFAA0}
    /// @brief Default virtual destructor
    ~ObjectPillarBase() override = default;

    /// @addr{0x807FFA94}
    /// @copybrief ObjectBase::getKclName()
    /// @return The model name of the pillar base, `dc_pillar_base`
    [[nodiscard]] const char *getKclName() const override {
        return "dc_pillar_base";
    }
};

/// @brief Represents the part of the Dry Dry Ruins pillar that falls
/// @details Acts as a wall before the pillar starts to fall, acts as a hazard while falling, and
/// disables once the pillar has fallen.
class ObjectPillarC final : public ObjectCollidable {
public:
    /// @addr{0x807FEB68}
    /// @copybrief ObjectCollidable::ObjectCollidable(const System::MapdataGeoObj &)
    /// @param params The parameters used to initialize the object
    /// @details Initializes @ref m_fallStart based on param setting 1.
    ObjectPillarC(const System::MapdataGeoObj &params)
        : ObjectCollidable(params),
          m_fallStart(static_cast<u32>(params.setting(0))) {}

    /// @addr{0x807FFAE0}
    /// @brief Default virtual destructor
    ~ObjectPillarC() override = default;

    void calcCollisionTransform() override;

    /// @addr{0x807FFA74}
    /// @copybrief ObjectBase::getCollisionRadius()
    /// @return The collision radius of the falling pillar, `3000.0f`.
    [[nodiscard]] f32 getCollisionRadius() const override {
        return 3000.0f;
    }

    /// @addr{0x807FFA80}
    /// @copybrief ObjectBase::id()
    /// @return The ID of the falling pillar object, `ObjectId::DCPillarC`.
    [[nodiscard]] ObjectId id() const override {
        return ObjectId::DCPillarC;
    }

    /// @addr{0x807FED5C}
    /// @copybrief ObjectCollidable::onCollision()
    /// @param reactionOnKart The reaction that should be applied to the kart upon collision
    /// @return @ref Kart::Reaction::Wall if the pillar has not started falling, otherwise @ref
    /// Kart::Reaction::Sideways.
    Kart::Reaction onCollision(Kart::KartObject * /*kartObj*/, Kart::Reaction reactionOnKart,
            Kart::Reaction /*reactionOnObj*/, EGG::Vector3f & /*hitDepth*/) override {
        auto *raceMgr = System::RaceManager::Instance();
        return raceMgr->timer() < m_fallStart ? Kart::Reaction::Wall : reactionOnKart;
    }

private:
    const u32 m_fallStart; ///< The number of frames before the pillar will start to fall
};

/// @brief Represents the entirety of a pillar that falls on Dry Dry Ruins
/// @details There are three parts that comprise a pillar: the upright/falling pillar collision
/// (@ref ObjectPillarC), the pilar base collision (@ref ObjectPillarBase), and the trickable pillar
/// KCL once it has fallen (represented with this parent class). The pillar starts to fall after
/// @ref m_fallStart frames. The duration of the fall depends on @ref m_targetRot.
class ObjectPillar final : public ObjectKCL {
public:
    /// @addr{0x807FED80}
    /// @copybrief ObjectKCL::ObjectKCL(const System::MapdataGeoObj &)
    /// @details Initializes the pillar to the @State::Upright state. Sets @ref m_fallStart based on
    /// param setting 1 and sets @ref m_targetRot by converting param setting 2 from degrees to
    /// radians. Caches the object's initial rotation to @ref m_initRot and initializes @ref
    /// m_currRot to zero. Constructs and loads the pillar base (@ref ObjectPillarBase) and the
    /// upright/falling pillar collision object (@ref ObjectPillarC). Finally, sets @ref
    /// m_groundFrame to the maximum possible value.
    ObjectPillar(const System::MapdataGeoObj &params)
        : ObjectKCL(params),
          m_state(State::Upright),
          m_fallStart(static_cast<u32>(params.setting(0))),
          m_targetRot(F_PI * static_cast<f32>(params.setting(1)) / 180.0f),
          m_initRot(rot().x),
          m_currRot(EGG::Vector3f::zero) {
        m_base = EGG::egg_new<ObjectPillarBase>(params);
        m_collidable = EGG::egg_new<ObjectPillarC>(params);

        m_base->load();
        m_collidable->load();

        m_groundFrame = std::numeric_limits<s32>::max();
    }

    /// @addr{0x807FFA34}
    /// @brief Default virtual destructor
    ~ObjectPillar() override = default;

    /// @addr{0x807FEFD8}
    /// @copybrief ObjectBase::init()
    /// @details Calls @ref ObjectBase::init(). Disables collision for @ref ObjectPillarC and this
    /// object (since it has not yet fallen). Finally, caches the current rotation to @ref
    /// m_currRot.
    void init() override {
        ObjectBase::init();

        m_collidable->disableCollision();
        m_currRot = rot();
        disableCollision();
    }

    void calc() override;

    /// @addr{0x807FFA2C}
    /// @copybrief ObjectBase::loadFlags()
    /// @return Returns @ref eLoadFlags::Calc, so that object is calculated every frame.
    [[nodiscard]] LoadFlags loadFlags() const override {
        return LoadFlags(eLoadFlags::Calc);
    }

    /// @addr{0x807FF980}
    /// @copybrief ObjectKCL::colRadiusAdditionalLength()
    /// @return Additional length to be added to the collision radius of the pillar, `4000.0f`
    [[nodiscard]] f32 colRadiusAdditionalLength() const override {
        return 4000.0f;
    }

    /// @addr{0x807FF83C}
    /// @copybrief ObjectKCL::getUpdatedMatrix()
    /// @param timeOffset The time offset used to calculate the current frame's transformation
    /// @return Const ref to the updated transformation matrix for the current frame, taking into
    /// account the time offset.
    [[nodiscard]] const EGG::Matrix34f &getUpdatedMatrix(u32 timeOffset) override {
        f32 rot = calcRot(System::RaceManager::Instance()->timer() - timeOffset);
        m_workMat.makeRT(EGG::Vector3f(rot, m_currRot.y, m_currRot.z), pos());
        return m_workMat;
    }

private:
    /// @brief The state of the pillar in terms of whether it is falling or not
    enum class State {
        Upright = 0, ///< The pillar is upright and has not yet started to fall
        Break = 1,   ///< The pillar is falling and can be collided with as a hazard
        Ground = 2,  ///< The pillar has finished falling and can be collided with as a wall
    };

    /// @addr{0x807FF90C}
    /// @brief Calculates the current rotation of the pillar based on the number of frames elapsed
    /// in the race.
    /// @param frame The current framecount of the race.
    /// @return The current rotation of the pillar in radians.
    /// @details If the pillar has already fallen, then returns @ref m_targetRot. Otherwise, the
    /// rotation is computed as:
    /// \f[ rot = \min(m\_targetRot,\ m\_initRot + 1e^{-7} \cdot (frame - m\_fallStart)^3) \f]
    [[nodiscard]] f32 calcRot(s32 frame) const {
        constexpr f32 STEP = 1e-7f;

        if (m_groundFrame < frame) {
            return m_targetRot;
        }

        frame -= m_fallStart;
        return std::min(m_targetRot, m_initRot + STEP * static_cast<f32>(frame * frame * frame));
    }

    State m_state;               ///< Falling state of the pillar
    const u32 m_fallStart;       ///< The number of frames before the pillar will start to fall
    const f32 m_targetRot;       ///< How much the pillar rotates during the fall, in radians
    const f32 m_initRot;         ///< Initial rotation of the pillar
    EGG::Vector3f m_currRot;     ///< Current rotation of the pillar
    ObjectPillarBase *m_base;    ///< Stationary portion of pillar
    ObjectPillarC *m_collidable; ///< Wall and hazard collision of the upright/falling pillar
    EGG::Matrix34f m_workMat;    ///< Rotation and translation matrix
    s32 m_groundFrame;           ///< Frame the pillar has finished falling
};

} // namespace Kinoko::Field
