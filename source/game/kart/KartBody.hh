#pragma once

#include "game/kart/KartObjectProxy.hh"
#include "game/kart/KartPhysics.hh"

namespace Kart {

/// @nosubgrouping
class KartBody : protected KartObjectProxy {
public:
    KartBody(KartPhysics *physics);
    virtual ~KartBody() {}

    virtual EGG::Matrix34f wheelMatrix(u16);

    /// @beginSetters
    void reset();
    void setAngle(f32 val);
    /// @endSetters

    /// @beginGetters
    KartPhysics *physics() const;
    /// @endGetters

protected:
    KartPhysics *m_physics;
    f32 m_anAngle; ///< @rename
};

class KartBodyKart : public KartBody {
public:
    KartBodyKart(KartPhysics *physics);
    ~KartBodyKart();
};

class KartBodyBike : public KartBody {
public:
    KartBodyBike(KartPhysics *physics);
    ~KartBodyBike();

    EGG::Matrix34f wheelMatrix(u16 wheelIdx) override;
};

} // namespace Kart
