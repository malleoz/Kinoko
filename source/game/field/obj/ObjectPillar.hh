
#pragma once

#include "game/field/obj/ObjectCollidable.hh"
#include "game/field/obj/ObjectKCL.hh"

#include "game/kart/KartCollide.hh"

#include "game/system/RaceManager.hh"

namespace Field {

/// @brief The stationary portion of the pillar. It always just acts as a wall.
class ObjectPillarBase : public ObjectKCL {
public:
    ObjectPillarBase(const System::MapdataGeoObj &params);
    ~ObjectPillarBase() override;

    /// @addr{0x807FFA88}
    [[nodiscard]] virtual f32 colRadiusAdditionalLength() const override {
        return 0.0f;
    }
};

/// @brief Represents collision for the part of the pillar that falls.
/// @details This collision acts as a wall before the pillar starts to fall, acts as a hazard while
/// it's falling, and disables once the pillar has fallen.
class ObjectPillarC : public ObjectCollidable {
public:
    ObjectPillarC(const System::MapdataGeoObj &params);
    ~ObjectPillarC() override;

    void calcCollisionTransform() override;

    /// @addr{0x807FFA74}
    [[nodiscard]] f32 getCollisionRadius() const override {
        return 3000.0f;
    }

    /// @addr{0x807FFA80}
    [[nodiscard]] ObjectId id() const override {
        return ObjectId::DCPillarC;
    }

    /// @addr{0x807FED5C}
    Kart::Reaction onCollision(Kart::KartObject * /*kartObj*/, Kart::Reaction reactionOnKart,
            Kart::Reaction /*reactionOnObj*/, EGG::Vector3f & /*hitDepth*/) override {
        auto *raceMgr = System::RaceManager::Instance();
        return raceMgr->timer() < m_fallStart ? Kart::Reaction::WallAllSpeed : reactionOnKart;
    }

private:
    u16 m_pillarId;
    u32 m_fallStart; ///< The number of frames before the pillar will start to fall.
};

class ObjectPillar : public ObjectKCL {
public:
    ObjectPillar(const System::MapdataGeoObj &params);
    ~ObjectPillar() override;

    void init() override;
    void calc() override;

    /// @addr{0x807FFA2C}
    [[nodiscard]] u32 loadFlags() const override {
        return 1;
    }

    /// @addr{0x807FF980}
    [[nodiscard]] f32 colRadiusAdditionalLength() const override {
        return 4000.0f;
    }

    [[nodiscard]] const EGG::Matrix34f &getUpdatedMatrix(u32 timeOffset) override;

private:
    u32 m_state;                 ///< 0 when upright, 1 when falling, and 2 afterwards.
    u32 m_fallStart;             ///< The number of frames before the pillar will start to fall.
    f32 m_fallRotation;          ///< How much the pillar rotates during the fall, in radians.
    EGG::Vector3f m_startRot;    ///< Initial rotation of the pillar.
    ObjectPillarBase *m_base;    ///< Stationary portion of pillar.
    ObjectPillarC *m_collidable; ///< Wall and hazard collision of the upright/falling pillar.
    EGG::Matrix34f m_rtMtx;      ///< Rotation and translation matrix;
    s32 m_fallFrame;             ///< Frame the pillar has started falling.
};

} // namespace Field
