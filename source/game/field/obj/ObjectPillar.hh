
#pragma once

#include "game/field/obj/ObjectCollidable.hh"
#include "game/field/obj/ObjectKCL.hh"

#include "game/kart/KartCollide.hh"

#include "game/system/RaceManager.hh"

namespace Kinoko::Field {

/// @brief The stationary portion of the Dry Dry Ruins pillars. It just acts as a wall.
class ObjectPillarBase final : public ObjectKCL {
public:
    ObjectPillarBase(const System::MapdataGeoObj &params);
    ~ObjectPillarBase() override;

    /// @addr{0x807FFA94}
    /// @copybrief ObjectBase::getKclName()
    /// @return The model name of the pillar base (`dc_pillar_base`)
    [[nodiscard]] const char *getKclName() const override {
        return "dc_pillar_base";
    }
};

/// @brief Represents the part of the Dry Dry Ruins pillar that falls
/// @details Acts as a wall before the pillar starts to fall, acts as a hazard while falling, and
/// disables once the pillar has fallen.
class ObjectPillarC final : public ObjectCollidable {
public:
    ObjectPillarC(const System::MapdataGeoObj &params);
    ~ObjectPillarC() override;

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
/// @details There are really three parts that comprise a pillar: the upright pillar collision, the
/// pilar base collision, and the trickable pillar KCL once it has fallen.
class ObjectPillar final : public ObjectKCL {
public:
    ObjectPillar(const System::MapdataGeoObj &params);
    ~ObjectPillar() override;

    /// @addr{0x807FEFD8}
    /// @copybrief ObjectBase::init()
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
    [[nodiscard]] f32 colRadiusAdditionalLength() const override {
        return 4000.0f;
    }

    [[nodiscard]] const EGG::Matrix34f &getUpdatedMatrix(u32 timeOffset) override;

private:
    /// @brief The state of the pillar in terms of whether it is falling or not
    enum class State {
        Upright = 0, ///< The pillar is upright and has not yet started to fall
        Break = 1,   ///< The pillar is falling and can be collided with as a hazard
        Ground = 2,  ///< The pillar has finished falling and can be collided with as a wall
    };

    [[nodiscard]] f32 calcRot(s32 frame) const;

    State m_state;               ///< Falling state of the pillar
    const u32 m_fallStart;       ///< The number of frames before the pillar will start to fall
    const f32 m_targetRotation;  ///< How much the pillar rotates during the fall, in radians
    const f32 m_initRot;         ///< Initial rotation of the pillar
    EGG::Vector3f m_currRot;     ///< Current rotation of the pillar
    ObjectPillarBase *m_base;    ///< Stationary portion of pillar
    ObjectPillarC *m_collidable; ///< Wall and hazard collision of the upright/falling pillar
    EGG::Matrix34f m_workMat;    ///< Rotation and translation matrix
    s32 m_groundFrame;           ///< Frame the pillar has finished falling
};

} // namespace Kinoko::Field
