#pragma once

#include "game/kart/KartBody.hh"

namespace Kinoko::Kart {

/// @brief The highest level abstraction for a kart.
class KartObject : public KartObjectProxy {
public:
    /// @addr{0x8058DDBC}
    /// @brief Constructor
    /// @param param Pointer to the kart's stats
    /// @details Initializes KartAccessor::param with the provided `param` pointer.
    KartObject(KartParam *param) {
        m_pointers.param = param;
    }

    virtual ~KartObject();

    void init();
    void initCollision();
    void initPhysics();
    void createModel();
    void calcSub();
    void calc();

    /// @brief Gets a pointer to the collection of associated kart subsystems
    /// @return Const pointer to the collection of associated kart subsystems, @ref m_pointers
    [[nodiscard]] const KartAccessor *accessor() const {
        return &m_pointers;
    }

    [[nodiscard]] static KartObject *Create(Character character, Vehicle vehicle, u8 playerIdx);

protected:
    KartAccessor m_pointers; ///< The collection of pointers to the various subsystems of the kart

private:
    /// @addr{0x8058E5F8}
    /// @brief Creates a KartBody object based on the type of vehicle (kart or bike)
    /// @param physics Pointer to the kart's physics state manager
    /// @return Pointer to the created KartBody instance
    /// @details For the base class, this creates a @ref KartBodyKart.
    [[nodiscard]] virtual KartBody *createBody(KartPhysics *physics) {
        return EGG::egg_new<KartBodyKart>(physics);
    }

    virtual void createTires();

    void prepareTiresAndSuspensions();
    void createSub();
};

/// @brief The highest level abstraction for a bike.
class KartObjectBike final : public KartObject {
public:
    /// @addr{0x8058F20C}
    /// @brief Constructor
    /// @param param Pointer to the kart's stats
    KartObjectBike(KartParam *param) : KartObject(param) {}

    /// @addr{0x8058F8B0}
    /// @brief Default virtual destructor
    ~KartObjectBike() override = default;

private:
    /// @addr{0x8058F260}
    /// @copybrief @ref KartObject::createBody()
    /// @param physics Pointer to the kart's physics state manager
    /// @return Pointer to the created KartBody instance
    /// @details Creates a @ref KartBodyBike, except for the Quacker bike which creates a @ref
    /// KartBodyQuacker.
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
