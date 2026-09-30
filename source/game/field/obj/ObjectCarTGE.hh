#pragma once

#include "game/field/StateManager.hh"
#include "game/field/obj/ObjectCollidable.hh"

namespace Kinoko::Field {

class ObjectHighwayManager;

/// @brief Represents a vehicle (car, truck, or bomb car) on Moonview Highway
/// @details Interfaces with @ref ObjectHighwayManager to enforce a squish cooldown for the player.
/// Bomb cars are not present in Time Trial mode and are thus not implemented in Kinoko. Each
/// vehicle has two GJK collision primitives in order for the collision detection to better reflect
/// the shape of the vehicle.
class ObjectCarTGE final : public ObjectCollidable, private StateManager {
public:
    /// @brief The type of vehicle represented by the object
    enum class CarType {
        Normal = 0,  ///< A regular car
        Truck = 1,   ///< A truck
        BombCar = 2, ///< Bob-omb car, unused in time trials
    };

    ObjectCarTGE(const System::MapdataGeoObj &params);

    /// @addr{0x806D691C}
    /// @brief Virtual destructor that destroys the auxiliary collision object
    ~ObjectCarTGE() override {
        EGG::egg_delete(m_auxCollision);
    }

    void init() override;

    /// @addr{0x806D6ECC}
    /// @copybrief ObjectBase::calc()
    /// @details Evaluates the car's state machine and updates the rail interpolator. Calls @ref
    /// calcState() to determine if the car should transition to a different state. Updates the
    /// car's position along the rail via @ref calcPos(). Finally, resets @ref m_hasAuxCollision to
    /// `false`.
    void calc() override {
        StateManager::calc();

        if (m_railInterpolator->calc() == RailInterpolator::Status::SegmentEnd) {
            calcState();
        }

        calcPos();

        m_hasAuxCollision = false;
    }

    /// @addr{0x806DA7AC}
    /// @copybrief ObjectBase::loadFlags()
    /// @return Returns @ref eLoadFlags::Calc, so that the object is calculated every frame.
    [[nodiscard]] LoadFlags loadFlags() const override {
        return LoadFlags(eLoadFlags::Calc);
    }

    /// @addr{0x806D68B0}
    /// @copybrief ObjectBase::getResources()
    /// @details Returns the resource name of the car variant
    /// @return The resource name of the car variant, @ref m_carName
    [[nodiscard]] const char *getResources() const override {
        return m_carName;
    }

    /// @addr{0x806DA7A4}
    /// @copybrief ObjectBase::getKclName()
    /// @return The model name of the car variant, @ref m_mdlName
    [[nodiscard]] const char *getKclName() const override {
        return m_mdlName;
    }

    void createCollision() override;
    void calcCollisionTransform() override;

    /// @addr{0x806D69C0}
    /// @copybrief ObjectBase::getCollisionRadius()
    /// @return The collision radius of the vehicle, `600.0f` for cars and `1100.0f` for trucks.
    [[nodiscard]] f32 getCollisionRadius() const override {
        constexpr f32 NORMAL_RADIUS = 600.0f;
        constexpr f32 TRUCK_RADIUS = 1100.0f;

        ASSERT(m_carType == CarType::Truck || m_carType == CarType::Normal);
        return (m_carType == CarType::Truck) ? TRUCK_RADIUS : NORMAL_RADIUS;
    }

    Kart::Reaction onCollision(Kart::KartObject *kartObj, Kart::Reaction reactionOnKart,
            Kart::Reaction reactionOnObj, EGG::Vector3f &hitDepth) override;

    /// @addr{0x806DA660}
    /// @brief Checks kart collision against both the primary and auxiliary collision objects
    /// @param lhs The collision object to check against this car
    /// @param dist The vector to store the collision depth
    /// @return `true` if a collision occurred, `false` otherwise
    bool checkCollision(ObjectCollisionBase *lhs, EGG::Vector3f &dist) override {
        dist = EGG::Vector3f::zero;
        bool hasCol = lhs->check(*m_collision, dist);

        if (!hasCol) {
            hasCol = lhs->check(*m_auxCollision, dist);
            m_hasAuxCollision = hasCol;
        }

        return hasCol;
    }

    /// @addr{0x806D7CF8}
    /// @copydoc ObjectCollidable::collisionCenter()
    /// @details The center of the car's collision volume is based on its type. Normal cars have an
    /// upwards offset of `100.0f` units, whereas trucks have an upwards offset of `300.0f` units.
    [[nodiscard]] const EGG::Vector3f &collisionCenter() const override {
        static constexpr EGG::Vector3f CENTER_TRUCK = EGG::Vector3f(0.0f, 300.0f, 0.0f);
        static constexpr EGG::Vector3f CENTER_NORMAL = EGG::Vector3f(0.0f, 100.0f, 0.0f);
        static constexpr EGG::Vector3f CENTER_DEFAULT = EGG::Vector3f(0.0f, 0.0f, 0.0f);

        switch (m_carType) {
        case CarType::Truck:
            return CENTER_TRUCK;
        case CarType::Normal:
            return CENTER_NORMAL;
        default:
            return CENTER_DEFAULT;
        }
    }

    /// @beginSetters

    /// @brief Sets the @ref ObjectHighwayManager associated with this object
    /// @param highwayMgr The highway manager to oversee this object
    void setHighwayManager(const ObjectHighwayManager *highwayMgr) {
        m_highwayMgr = highwayMgr;
    }

    /// @addr{0x806D9A04}
    /// @brief Resets @ref m_squashed to `false`
    /// @details This is called by @ref ObjectHighwayManager on all vehicles immediately after a
    /// squash with any one car occurs, so that it can enforce the squish cooldown for the player.
    void reset() {
        m_squashed = false;
    }

    /// @endSetters

    /// @beginGetters

    /// @brief Checks if this vehicle has squashed the player this frame
    /// @return `true` if the vehicle has squashed the player, `false` otherwise
    [[nodiscard]] bool squashed() const {
        return m_squashed;
    }

    /// @endGetters

private:
    /// @brief Rate of speed increase when a vehicle enters and exits the highway
    static constexpr f32 TOLL_BOOTH_ACCEL = 200.0f;

    /// @addr{0x806D7D70}
    /// @brief Runs once per frame when cars are speeding up when getting on the highway
    /// @details Increases @ref m_currSpeed by @ref TOLL_BOOTH_ACCEL. Once @ref m_currSpeed reaches
    /// @ref m_highwaySpeed, it's clamped to @ref m_highwaySpeed and transitions to the cruising
    /// state. Regardless, sets the rail interpolator's speed to the updated @ref m_currSpeed and
    /// sets @ref m_vel by scaling the rail interpolator's current tangent direction by @ref
    /// m_currSpeed.
    void calcSpeedup() {
        m_currSpeed += TOLL_BOOTH_ACCEL;

        if (m_currSpeed > m_highwaySpeed) {
            m_currSpeed = m_highwaySpeed;
            m_nextStateId = 0;
        }

        m_railInterpolator->setSpeed(m_currSpeed);
        m_vel = m_railInterpolator->curTangentDir() * m_currSpeed;
    }

    /// @addr{0x806D7E0C}
    /// @brief Runs every frame when the car is slowing down after getting off the highway
    /// @details Decreases @ref m_currSpeed by @ref TOLL_BOOTH_ACCEL. Once @ref m_currSpeed reaches
    /// @ref m_localSpeed, it's clamped to @ref m_localSpeed and transitions to the cruising state.
    /// Regardless, sets the rail interpolator's speed to the updated @ref m_currSpeed and sets @ref
    /// m_vel by scaling the rail interpolator's current tangent direction by @ref m_currSpeed.
    void calcSlowdown() {
        m_currSpeed -= TOLL_BOOTH_ACCEL;

        if (m_currSpeed < m_localSpeed) {
            m_currSpeed = m_localSpeed;
            m_nextStateId = 0;
        }

        m_railInterpolator->setSpeed(m_currSpeed);
        m_vel = m_railInterpolator->curTangentDir() * m_currSpeed;
    }

    void calcPos();

    /// @addr{0x806D9504}
    /// @brief Checks if the car should speed up or slow down based off the rail point's settings
    /// @details For each point along the rail, setting 2 is used to specify whether it is a highway
    /// point (1) or a local road point (0). Based on the current and next rail point settings,
    /// transitions the car to the appropriate state (or does nothing if the road type has not
    /// changed).
    void calcState() {
        u16 curPointSpeedSetting = m_railInterpolator->curPoint().setting[1];
        u16 nextPointSpeedSetting = m_railInterpolator->nextPoint().setting[1];

        if (curPointSpeedSetting == 0 && nextPointSpeedSetting == 1) {
            m_nextStateId = 1;
        } else if (curPointSpeedSetting == 1 && nextPointSpeedSetting == 0) {
            m_nextStateId = 2;
        }
    }

    const ObjectHighwayManager *m_highwayMgr; ///< Manager that handles squish cooldowns
    ObjectCollisionBase *m_auxCollision; ///< Secondary collision cylinder for more accurate shape
    const f32 m_highwaySpeed;            ///< Speed while on the highway
    const f32 m_localSpeed;              ///< Speed while off the highway
    char m_carName[32];                  ///< Resource (.brres) name
    char m_mdlName[32];                  ///< Model/KCL name
    CarType m_carType;                   ///< Car, truck, or bomb car
    ObjectId m_dummyId;                  ///< Dummy id (CarBody or KartTruck) used for hit reaction
    EGG::Vector3f m_vel;                 ///< %Rail tangent scaled by current speed
    f32 m_currSpeed;                     ///< Current speed of the vehicle along the rail
    EGG::Vector3f m_up;                  ///< Smoothed up vector
    EGG::Vector3f m_tangent;             ///< Smoothed forward direction along the rail
    bool m_squashed;                     ///< Set if this vehicle squashed player in last 200 frames
    bool m_hasAuxCollision; ///< Set when a collision was the result of @ref m_auxCollision
    f32 m_hitAngle; ///< Angular threshold for determining if player should be launched or squished

    /// @brief The enter and calc functions for each @ref StateManager entry
    static constexpr std::array<StateManagerEntry, 3> STATE_ENTRIES = {{
            {StateEntry<ObjectCarTGE, nullptr, nullptr>(0)},
            {StateEntry<ObjectCarTGE, nullptr, &ObjectCarTGE::calcSpeedup>(1)},
            {StateEntry<ObjectCarTGE, nullptr, &ObjectCarTGE::calcSlowdown>(2)},
    }};
};

} // namespace Kinoko::Field
