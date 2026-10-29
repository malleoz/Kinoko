#pragma once

#include "game/kart/KartObjectProxy.hh"

#include <egg/math/Matrix.hh>

namespace Kinoko::Kart {

class KartPhysics;

/// @brief Represents the body of a general vehicle
/// @details This class owns the @ref KartPhysics object for this kart. It also tracks the current
/// and target sink depth of the kart, which represents how far the kart can sink into the ground in
/// response to the colliding @ref Field::CollisionInfo::intensity.
class KartBody : protected KartObjectProxy {
public:
    KartBody(KartPhysics *physics);
    virtual ~KartBody();

    [[nodiscard]] virtual EGG::Matrix34f wheelMatrix(u16 wheelIdx, const EGG::Matrix34f &bodyMat);
    virtual void setParams(const KartParam::KartDisp &kartDisp,
            const KartParam::BikeDisp &bikeDisp) = 0;

    void init(const KartParam::KartDisp &kartDisp, const KartParam::BikeDisp &bikeDisp);
    void reset();
    void calcSinkDepth();
    void trySetTargetSinkDepth(f32 val);
    void calcTargetSinkDepth();

    /// @beginSetters

    void setLeanAngle(f32 val);

    /// @endSetters

    /// @beginGetters

    [[nodiscard]] KartPhysics *physics() const;
    [[nodiscard]] f32 sinkDepth() const;

    /// @endGetters

protected:
    KartPhysics *const m_physics; ///< Pointer to the kart's physics state manager
    f32 m_leanAngle;       ///< Lean angle of the kart incurred from drifting, SSMT, or burnout
    f32 m_sinkDepth;       ///< Current smoothed vehicle offset applied downward into collision
    f32 m_targetSinkDepth; ///< Maximum vehicle offset applied downward into collision
};

/// @brief Represents the body of a kart
/// @details In the base game, this is a separate class from @ref KartBody, even though we don't
/// implement any overridden functions in this subclass. To maintain clarity, we define this as a
/// separate class so the @p addr{} Doxygen annotations are accurate.
class KartBodyKart final : public KartBody {
public:
    KartBodyKart(KartPhysics *physics);
    ~KartBodyKart() override;

    /// @addr{0x8056CDC4}
    void setParams(const KartParam::KartDisp &, const KartParam::BikeDisp &) override {}
};

/// @brief Represents the body of a bike
/// @details Overrides the wheelMatrix() function to factor in handlebar rotation for the front
/// wheel.
class KartBodyBike : public KartBody {
public:
    KartBodyBike(KartPhysics *physics);
    ~KartBodyBike() override;

    [[nodiscard]] EGG::Matrix34f wheelMatrix(u16 wheelIdx, const EGG::Matrix34f &bodyMat) override;
    void setParams(const KartParam::KartDisp &kartDisp,
            const KartParam::BikeDisp &bikeDisp) override;

private:
    /// @brief The rotation of the handlebar
    /// @details Introduced in Kinoko as a slight performance improvement to prevent unnecessarily
    /// recalculating the handlebar rotation matrix every frame in wheelMatrix().
    EGG::Matrix34f m_handleRotMat;
};

/// @brief Represents the body of @ref Vehicle::Quacker
/// @details For the purposes of Kinoko, this effectively behaves the same as @ref KartBody in
/// terms of @ref wheelMatrix(). We keep it separate to maintain clarity and accuracy of the @p
/// addr{} Doxygen annotations.
class KartBodyQuacker : public KartBodyBike {
public:
    KartBodyQuacker(KartPhysics *physics);
    ~KartBodyQuacker() override;

    [[nodiscard]] EGG::Matrix34f wheelMatrix(u16 wheelIdx, const EGG::Matrix34f &bodyMat) override;
};

} // namespace Kinoko::Kart
