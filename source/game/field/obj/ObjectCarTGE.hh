#pragma once

#include "game/field/StateManager.hh"
#include "game/field/obj/ObjectCollidable.hh"

namespace Kinoko::Field {

class ObjectHighwayManager;

/// @brief Represents a vehicle (car, truck, or bomb car) on Moonview Highway
/// @details Interfaces with a @ref ObjectHighwayManager to enforce a squish cooldown for the
/// player. Bomb cars are not present in Time Trial mode and are thus not implemented in Kinoko.
/// Each vehicle has two GJK collision primitives in order for the collision detection to better
/// reflect the shape of the vehicle.
class ObjectCarTGE final : public ObjectCollidable, private StateManager {
public:
    /// @brief The type of vehicle represented by the object
    enum class CarType {
        Normal = 0,
        Truck = 1,
        BombCar = 2, ///< Unused in time trials
    };

    ObjectCarTGE(const System::MapdataGeoObj &params);
    ~ObjectCarTGE() override;

    void init() override;
    void calc() override;

    /// @addr{0x806DA7AC}
    /// @copybrief ObjectBase::loadFlags()
    /// @return Returns @ref eLoadFlags::Calc, so that object is calculated every frame.
    [[nodiscard]] LoadFlags loadFlags() const override {
        return LoadFlags(eLoadFlags::Calc);
    }

    /// @addr{0x806D68B0}
    /// @copybrief ObjectBase::getResources()
    /// @details Returns the resource name of the car variant
    /// @return The resource name of the car variant
    [[nodiscard]] const char *getResources() const override {
        return m_carName;
    }

    /// @addr{0x806DA7A4}
    [[nodiscard]] const char *getKclName() const override {
        return m_mdlName;
    }

    void createCollision() override;
    void calcCollisionTransform() override;

    /// @addr{0x806D69C0}
    [[nodiscard]] f32 getCollisionRadius() const override {
        constexpr f32 NORMAL_RADIUS = 600.0f;
        constexpr f32 TRUCK_RADIUS = 1100.0f;

        ASSERT(m_carType == CarType::Truck || m_carType == CarType::Normal);
        return (m_carType == CarType::Truck) ? TRUCK_RADIUS : NORMAL_RADIUS;
    }

    Kart::Reaction onCollision(Kart::KartObject *kartObj, Kart::Reaction reactionOnKart,
            Kart::Reaction reactionOnObj, EGG::Vector3f &hitDepth) override;
    bool checkCollision(ObjectCollisionBase *lhs, EGG::Vector3f &dist) override;
    [[nodiscard]] const EGG::Vector3f &collisionCenter() const override;

    /// @beginSetters
    void setHighwayManager(const ObjectHighwayManager *highwayMgr) {
        m_highwayMgr = highwayMgr;
    }

    /// @addr{0x806D9A04}
    void reset() {
        m_squashed = false;
    }
    /// @endSetters

    /// @beginGetters
    [[nodiscard]] bool squashed() const {
        return m_squashed;
    }
    /// @endGetters

private:
    /// @brief Rate of speed increase when a vehicle enters and exits the highway
    static constexpr f32 TOLL_BOOTH_ACCEL = 200.0f;

    void enterStateStub() {}
    void calcStateStub() {}
    void calcSpeedup();
    void calcSlowdown();

    void calcPos();
    void calcStateFromRailPointSetting();

    const ObjectHighwayManager *m_highwayMgr; ///< Manager that handles squish cooldowns
    ObjectCollisionBase *m_auxCollision; ///< Secondary collision cylinder for more accurate shape
    f32 m_highwayVel;                    ///< Speed while on the highway
    f32 m_localVel;                      ///< Speed while off the highway
    char m_carName[32];                  ///< Resource (.brres) name
    char m_mdlName[32];                  ///< Model/KCL name
    CarType m_carType;                   ///< Car, truck, or bomb car
    ObjectId m_dummyId;                  ///< Dummy id (CarBody or KartTruck) used for hit reaction
    EGG::Vector3f m_scaledTangentDir;    ///< %Rail tangent scaled by current speed
    f32 m_currSpeed;                     ///< Current speed of the vehicle along the rail
    EGG::Vector3f m_up;                  ///< Smoothed up vector
    EGG::Vector3f m_tangent;             ///< Smoothed forward direction along the rail
    bool m_squashed;                     ///< Set if this vehicle squashed player in last 200 frames
    bool m_hasAuxCollision; ///< Set when a collision was the result of @ref m_auxCollision
    f32 m_hitAngle; ///< Angular threshold for determining if player should be launched or squished

    /// @brief The enter and calc functions for each @ref StateManager entry
    static constexpr std::array<StateManagerEntry, 3> STATE_ENTRIES = {{
            {StateEntry<ObjectCarTGE, &ObjectCarTGE::enterStateStub, &ObjectCarTGE::calcStateStub>(
                    0)},
            {StateEntry<ObjectCarTGE, &ObjectCarTGE::enterStateStub, &ObjectCarTGE::calcSpeedup>(
                    1)},
            {StateEntry<ObjectCarTGE, &ObjectCarTGE::enterStateStub, &ObjectCarTGE::calcSlowdown>(
                    2)},
    }};
};

} // namespace Kinoko::Field
