#pragma once

#include "game/kart/KartObjectProxy.hh"

namespace Kinoko::Kart {

class KartBody;

/// @brief The highest level abstraction for a kart.
class KartObject : public KartObjectProxy {
public:
    KartObject(KartParam *param);
    ~KartObject() override;

    void init();
    void initCollision();
    void initPhysics();
    void createModel();
    void calcSub();
    void calc();

    /// @beginGetters

    [[nodiscard]] const KartAccessor *accessor() const;

    /// @endGetters

    [[nodiscard]] static KartObject *Create(Character character, Vehicle vehicle, u8 playerIdx);

protected:
    KartAccessor m_pointers; ///< The collection of pointers to the various subsystems of the kart

private:
    [[nodiscard]] virtual KartBody *createBody(KartPhysics *physics);
    virtual void createTires();

    void prepareTiresAndSuspensions();
    void createSub();
};

/// @brief The highest level abstraction for a bike.
class KartObjectBike final : public KartObject {
public:
    KartObjectBike(KartParam *param);
    ~KartObjectBike() override;

private:
    [[nodiscard]] KartBody *createBody(KartPhysics *physics) override;
    void createTires() override;
};

} // namespace Kinoko::Kart
