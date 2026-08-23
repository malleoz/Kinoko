#pragma once

#include "game/kart/KartObjectProxy.hh"

/// @brief Pertains to rendering the kart model.
namespace Kinoko::Render {

/// @brief Included in Kinoko because it mysteriously sets an angle member variable in KartBody.
class KartModel : Kart::KartObjectProxy {
public:
    KartModel();
    virtual ~KartModel();

    virtual void vf_1c();

    /// @addr{0x807C8758}
    void init() {
        FUN_807C7828(param()->playerIdx(), isBike());
        _2e8 = 0.0f;
    }

    /// @addr{0x807CB360}
    void calc() {
        FUN_807CB530();
    }

    void FUN_807CB198();

    /// @addr{0x807CB530}
    /// @rename
    void FUN_807CB530() {
        FUN_807CB198();
        vf_1c();
    }

    /// @addr{0x807C7828}
    /// @rename
    void FUN_807C7828(u8 /*playerIdx*/, bool /*isBike*/) {
        m_isInsideDrift = vehicleType() == Kart::KartParam::Stats::DriftType::Inside_Drift_Bike;
    }

private:
    bool m_somethingLeft;
    bool m_somethingRight;
    f32 _54;
    f32 _58;
    f32 _5c;
    f32 _64;
    bool m_isInsideDrift;
    f32 _2e8;
};

class KartModelKart : public KartModel {
public:
    /// @addr{0x807C7364}
    KartModelKart() = default;

    /// @addr{0x807CDD08}
    ~KartModelKart() = default;
};

class KartModelBike : public KartModel {
public:
    /// @addr{0x807CDCCC}
    KartModelBike() = default;

    /// @addr{0x807D3F58}
    ~KartModelBike() = default;
};

} // namespace Kinoko::Render
