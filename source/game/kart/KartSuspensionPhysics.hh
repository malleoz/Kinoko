#pragma once

#include "game/kart/CollisionGroup.hh"
#include "game/kart/KartObjectProxy.hh"

namespace Kinoko::Kart {

/// @brief Manages wheel physics and collision checks
/// @details Each wheel corresponds to a @ref CollisionGroup with a single hitbox for the tire.
class WheelPhysics : KartObjectProxy {
public:
    WheelPhysics(u16 wheelIdx, u16 bspWheelIdx);
    ~WheelPhysics();

    /// @addr{0x80599470}
    /// @brief Initializes the wheel's @ref CollisionGroup and creates a single hitbox for the tire
    void init() {
        m_hitboxGroup = EGG::egg_new<CollisionGroup>();
        m_hitboxGroup->createSingleHitbox(10.0f, EGG::Vector3f::zero);
    }

    /// @addr{0x805994D4}
    /// @brief Fetches the @ref BSP::Wheel pointer corresponding to this wheel
    void initBsp() {
        m_bspWheel = &bsp().wheels[m_bspWheelIdx];
    }

    void reset();

    void realign(const EGG::Vector3f &bottom, const EGG::Vector3f &vehicleMovement);

    void calcCollision(const EGG::Vector3f &bottom, const EGG::Vector3f &topmostPos);
    void calcSuspension(const EGG::Vector3f &forward);

    /// @beginSetters
    void setSuspTravel(f32 suspTravel) {
        m_suspTravel = suspTravel;
    }

    void setPos(const EGG::Vector3f &pos) {
        m_pos = pos;
    }

    void setLastPos(const EGG::Vector3f &pos) {
        m_lastPos = pos;
    }

    void setLastTopDiff(const EGG::Vector3f &pos) {
        m_lastTopDiff = pos;
    }

    void setWheelEdgePos(const EGG::Vector3f &pos) {
        m_wheelEdgePos = pos;
    }

    void setColVel(const EGG::Vector3f &vec) {
        m_colVel = vec;
    }
    /// @endSetters

    /// @beginGetters
    [[nodiscard]] const EGG::Vector3f &pos() const {
        return m_pos;
    }

    [[nodiscard]] const EGG::Vector3f &lastTopDiff() const {
        return m_lastTopDiff;
    }

    [[nodiscard]] f32 suspTravel() {
        return m_suspTravel;
    }

    [[nodiscard]] const EGG::Vector3f &topmostPos() const {
        return m_topmostPos;
    }

    [[nodiscard]] CollisionGroup *hitboxGroup() {
        return m_hitboxGroup;
    }

    [[nodiscard]] const CollisionGroup *hitboxGroup() const {
        return m_hitboxGroup;
    }

    [[nodiscard]] const EGG::Vector3f &relVel() const {
        return m_relVel;
    }

    [[nodiscard]] const EGG::Vector3f &wheelEdgePos() const {
        return m_wheelEdgePos;
    }

    [[nodiscard]] f32 effectiveRadius() const {
        return m_effectiveRadius;
    }

    [[nodiscard]] f32 hasSuspTravel() const {
        return m_hasSuspTravel;
    }
    /// @endGetters

private:
    const u16 m_wheelIdx;          ///< Index of this tire in the kart's array of wheels
    const u16 m_bspWheelIdx;       ///< Index of this tire in the BSP's array of wheels
    const BSP::Wheel *m_bspWheel;  ///< Pointer to the corresponding wheel in the BSP data
    CollisionGroup *m_hitboxGroup; ///< Pointer to the @ref CollisionGroup for this wheel
    EGG::Vector3f m_pos;           ///< The current world position of the tire
    EGG::Vector3f m_lastPos;       ///< The previous frame's world position of the tire
    EGG::Vector3f m_lastTopDiff;   ///< Difference between @ref m_pos and the topmost position
    f32 m_suspTravel;              ///< Current suspension travel distance
    EGG::Vector3f m_colVel;        ///< The wheel's velocity for the purpose of collision checks
    EGG::Vector3f m_relVel;        ///< Wheel velocity relative to the vehicle body
    EGG::Vector3f m_wheelEdgePos;  ///< Position at the bottom outer edge of the wheel
    f32 m_effectiveRadius;         ///< Current smoothed wheel radius
    f32 m_targetEffectiveRadius;   ///< The target wheel radius
    f32 m_hasSuspTravel;           ///< Boolean-like gate indicating if the suspension has travel
    EGG::Vector3f m_topmostPos;    ///< World position of the top of the suspension
};

/// @brief Physics for a single wheel's suspension
/// @details Also owns the underlying @ref WheelPhysics subsystem.
class KartSuspensionPhysics : KartObjectProxy {
public:
    /// @brief Describes the type of tire for the purpose of computing relative position
    /// @details Every other kart tire is a mirror of the first. Bikes do not leverage this.
    enum class TireType {
        Kart,          ///< Standard kart tire
        KartReflected, ///< Mirrored kart tire
        Bike,          ///< Standard bike tire
    };

    KartSuspensionPhysics(u16 wheelIdx, TireType TireType, u16 bspWheelIdx);
    ~KartSuspensionPhysics();

    void init();

    /// @addr{0x80599F54}
    /// @brief Resets the suspension physics to its initial state
    void reset() {
        m_topmostPos.setZero();
        m_maxTravelScaled = 0.0f;
        m_bottomDir.setZero();
    }

    void setInitialState();

    void calcCollision(f32 dt, const EGG::Vector3f &gravity, const EGG::Matrix34f &mat);
    void calcSuspension(const EGG::Vector3f &forward, const EGG::Vector3f &vehicleMovement);

private:
    const BSP::Wheel *m_bspWheel; ///< Pointer to the corresponding wheel in the BSP data
    WheelPhysics *m_tirePhysics;  ///< Pointer to the underlying @ref WheelPhysics subsystem
    const TireType m_tireType;    ///< The type of tire corresponding to this suspension
    const u16 m_bspWheelIdx;      ///< Index of this tire in the BSP's array of wheels
    const u16 m_wheelIdx;         ///< Index of this tire in the kart's array of wheels
    EGG::Vector3f m_topmostPos;   ///< World position of the top of the suspension
    f32 m_maxTravelScaled;     ///< @ref BSP::Wheel::maxTravel scaled by @ref KartSub::m_suspScale
    EGG::Vector3f m_bottomDir; ///< The "down" direction along the suspension axis
};

} // namespace Kinoko::Kart
