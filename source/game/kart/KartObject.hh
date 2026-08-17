#pragma once

#include "game/kart/KartBody.hh"

namespace Kinoko::Kart {

/// @brief The highest level abstraction for a kart.
class KartObject : public KartObjectProxy {
public:
    KartObject(KartParam *param);
    virtual ~KartObject();

    /// @brief Creates a KartBody object based on the type of vehicle (kart or bike)
    /// @addr{0x8058E5F8}
    /// @details For the base class, this creates a @ref KartBodyKart.
    [[nodiscard]] virtual KartBody *createBody(KartPhysics *physics) {
        return EGG::egg_new<KartBodyKart>(physics);
    }

    /// @brief Creates the appropriate @ref KartTire and @ref KartSuspension objects
    virtual void createTires();

    void init();
    void initCollision();
    void initPhysics();
    void prepareTiresAndSuspensions();

    void createSub();
    void createModel();

    void calcSub();
    void calc();

    [[nodiscard]] const KartAccessor *accessor() const {
        return &m_pointers;
    }

    [[nodiscard]] static KartObject *Create(Character character, Vehicle vehicle, u8 playerIdx);

protected:
    KartAccessor m_pointers; ///< The collection of pointers to the various subsystems of the kart
};

/// @brief The highest level abstraction for a bike.
class KartObjectBike : public KartObject {
public:
    KartObjectBike(KartParam *param);
    ~KartObjectBike() override;

    /// @addr{0x8058F260}
    /// @details For this derived class, this creates a @ref KartBodyBike or @ref KartBodyQuacker
    /// depending on the vehicle type.
    [[nodiscard]] KartBody *createBody(KartPhysics *physics) override {
        if (m_pointers.param->isVehicleRelativeBike()) {
            return EGG::egg_new<KartBodyQuacker>(physics);
        } else {
            return EGG::egg_new<KartBodyBike>(physics);
        }
    }

    void createTires() override;
};

} // namespace Kinoko::Kart
