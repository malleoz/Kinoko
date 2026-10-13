#pragma once

#include "game/kart/CollisionGroup.hh"
#include "game/kart/KartObjectProxy.hh"

namespace Kinoko::Kart {

/// @brief Manages wheel physics and collision checks
/// @details Each wheel corresponds to a @ref CollisionGroup with a single hitbox for the tire.
class WheelPhysics : private KartObjectProxy {
public:
    /// @addr{0x8059940C}
    /// @brief Constructor
    /// @param wheelIdx The index of the wheel
    /// @param bspWheelIdx The index of the wheel in the @ref BSP::wheels array
    /// @details Sets @ref m_wheelIdx and @ref m_bspWheelIdx to the provided `wheelIdx` and
    /// `bspWheelIdx`. Initializes @ref m_bspWheel to `nullptr`.
    WheelPhysics(u16 wheelIdx, u16 bspWheelIdx)
        : m_wheelIdx(wheelIdx),
          m_bspWheelIdx(bspWheelIdx),
          m_bspWheel(nullptr) {}

    /// @addr{0x8059A9C4}
    /// @brief Virtual destructor that destroys the underlying @ref CollisionGroup subsystem for
    /// this wheel
    ~WheelPhysics() override {
        EGG::egg_delete(m_hitboxGroup);
    }

    /// @addr{0x80599470}
    /// @brief Creates and initializes the wheel's @ref m_hitboxGroup and creates a single hitbox
    /// for the tire
    /// @details Calls @ref CollisionGroup::createSingleHitbox() to create a hitbox with radius
    /// `10.0f` for the tire.
    void init() {
        constexpr f32 RADIUS = 10.0f;

        m_hitboxGroup = EGG::egg_new<CollisionGroup>();
        m_hitboxGroup->createSingleHitbox(RADIUS, EGG::Vector3f::zero);
    }

    /// @addr{0x805994D4}
    /// @brief Fetches the @ref BSP::Wheel pointer corresponding to this wheel
    /// @details Sets @ref m_bspWheel to the wheel in the kart's @ref BSP::wheels array at index
    /// @ref m_bspWheelIdx.
    void initBsp() {
        m_bspWheel = &bsp().wheels[m_bspWheelIdx];
    }

    void reset();
    void realign(const EGG::Vector3f &bottom, const EGG::Vector3f &vehicleMovement);
    void calcCollision(const EGG::Vector3f &bottom, const EGG::Vector3f &topmostPos);
    void calcSuspension(const EGG::Vector3f &forward);

    /// @beginSetters

    /// @brief Sets the current suspension travel distance
    /// @param suspTravel The new suspension travel distance to set
    void setSuspTravel(f32 suspTravel) {
        m_suspTravel = suspTravel;
    }

    /// @brief Sets the current position of the wheel
    /// @param pos The new position to set
    void setPos(const EGG::Vector3f &pos) {
        m_pos = pos;
    }

    /// @brief Sets the last position of the wheel
    /// @param pos The new last position to set
    void setLastPos(const EGG::Vector3f &pos) {
        m_lastPos = pos;
    }

    /// @brief Sets the distance between the tire's position and the topmost point of the suspension
    /// @param pos The new distance to set
    void setLastTopDiff(const EGG::Vector3f &pos) {
        m_lastTopDiff = pos;
    }

    /// @brief Sets the position at the bottom outer edge of the wheel
    /// @param pos The new wheel edge position to set
    void setWheelEdgePos(const EGG::Vector3f &pos) {
        m_wheelEdgePos = pos;
    }

    /// @brief Sets the velocity of the wheel
    /// @param vec The new velocity to set
    void setColVel(const EGG::Vector3f &vec) {
        m_colVel = vec;
    }

    /// @endSetters

    /// @beginGetters

    /// @brief Gets the current position of the tire
    /// @return The current world position of the tire, @ref m_pos
    [[nodiscard]] const EGG::Vector3f &pos() const {
        return m_pos;
    }

    /// @brief Gets the distance between the tire and its topmost point of the suspension
    /// @return The distance between the tire and the topmost point of the suspension, @ref
    /// m_lastTopDiff
    [[nodiscard]] const EGG::Vector3f &lastTopDiff() const {
        return m_lastTopDiff;
    }

    /// @brief Gets the current suspension travel distance
    /// @return The current suspension travel distance, @ref m_suspTravel
    [[nodiscard]] f32 suspTravel() {
        return m_suspTravel;
    }

    /// @brief Gets the topmost point of the suspension
    /// @return The topmost point of the suspension, @ref m_topmostPos
    [[nodiscard]] const EGG::Vector3f &topmostPos() const {
        return m_topmostPos;
    }

    /// @brief Gets the collision group associated with this wheel
    /// @return Ref to the collision group, @ref m_hitboxGroup
    [[nodiscard]] CollisionGroup *hitboxGroup() {
        return m_hitboxGroup;
    }

    /// @brief Gets the collision group associated with this wheel
    /// @return Const ref to the collision group, @ref m_hitboxGroup
    [[nodiscard]] const CollisionGroup *hitboxGroup() const {
        return m_hitboxGroup;
    }

    /// @brief Gets the wheel's velocity relative to the vehicle body
    /// @return The wheel's relative velocity, @ref m_relVel
    [[nodiscard]] const EGG::Vector3f &relVel() const {
        return m_relVel;
    }

    /// @brief Gets the position at the bottom outer edge of the wheel
    /// @return The position at the bottom outer edge of the wheel, @ref m_wheelEdgePos
    [[nodiscard]] const EGG::Vector3f &wheelEdgePos() const {
        return m_wheelEdgePos;
    }

    /// @brief Gets the current smoothed wheel radius
    /// @return The current smoothed wheel radius, @ref m_effectiveRadius
    [[nodiscard]] f32 effectiveRadius() const {
        return m_effectiveRadius;
    }

    /// @brief Checks if the suspension has travel
    /// @return The value of @ref m_hasSuspTravel
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
class KartSuspensionPhysics : private KartObjectProxy {
public:
    /// @brief Describes the type of tire for the purpose of computing relative position
    /// @details Every other kart tire is a mirror of the first. Bikes do not leverage this.
    enum class TireType {
        Kart,          ///< Standard kart tire
        KartReflected, ///< Mirrored kart tire
        Bike,          ///< Standard bike tire
    };

    /// @addr{0x80599ED4}
    /// @brief Constructor
    /// @param wheelIdx The index of the wheel
    /// @param tireType The type of tire
    /// @param bspWheelIdx The index of the wheel in the @ref BSP::wheels array
    /// @details Sets @ref m_tireType, @ref m_bspWheelIdx, and @ref m_wheelIdx to the provided
    /// `tireType`, `bspWheelIdx`, and `wheelIdx` respectively. Initializes @ref m_bspWheel to
    /// `nullptr`.
    KartSuspensionPhysics(u16 wheelIdx, TireType tireType, u16 bspWheelIdx)
        : m_tirePhysics(nullptr),
          m_tireType(tireType),
          m_bspWheelIdx(bspWheelIdx),
          m_wheelIdx(wheelIdx) {}

    /// @addr{0x8059AA04}
    /// @brief Default virtual destructor
    ~KartSuspensionPhysics() override = default;

    void init();

    /// @addr{0x80599F54}
    /// @brief Resets the suspension physics to its initial state
    /// @details Resets @ref m_topmostPos, @ref m_maxTravelScaled, and @ref m_bottomDir to zero.
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
