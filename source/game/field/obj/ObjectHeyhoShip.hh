#pragma once

#include "game/field/StateManager.hh"
#include "game/field/obj/ObjectProjectileLauncher.hh"

namespace Kinoko::Field {

/// @brief The ship on GBA Shy Guy Beach that shoots cannonballs.
/// @details The ship moves along a rail and bobs up and down, occassionally shooting cannonballs.
/// The synchronization between the ship and the cannonballs is managed by @ref
/// ObjectHeyhoShipManager.
class ObjectHeyhoShip final : public ObjectProjectileLauncher {
public:
    ObjectHeyhoShip(const System::MapdataGeoObj &params);
    ~ObjectHeyhoShip() override;

    void init() override;
    void calc() override;

    /// @addr{0x806D2360}
    [[nodiscard]] u32 loadFlags() const override {
        return 1;
    }

    /// @addr{0x806D1CA4}
    [[nodiscard]] s16 launchPointIdx() override {
        return m_framesSinceLastLaunch == 0 ? m_railInterpolator->curPointIdx() : -1;
    }

    /// @addr{0x806D1CC4}
    /// @brief Initializes the rail interpolator to the given point index and returns the tangent
    const EGG::Vector3f &initRailDir(u16 idx) {
        m_railInterpolator->init(0.0f, static_cast<u32>(idx));
        return m_railInterpolator->curTangentDir();
    }

    void calcPos();

private:
    const f32 m_yAmplitude;      ///< How much the ship bobs up and down
    u32 m_frame;                 ///< Number of frames since the ship was initialized
    u32 m_framesSinceLastLaunch; ///< Number of frames since the last cannonball was launched
};

} // namespace Kinoko::Field
