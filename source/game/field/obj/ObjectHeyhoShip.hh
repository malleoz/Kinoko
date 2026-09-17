#pragma once

#include "game/field/obj/ObjectProjectileLauncher.hh"

namespace Kinoko::Field {

/// @brief The ship on GBA Shy Guy Beach that shoots cannonballs.
/// @details The ship moves along a rail and bobs up and down, occassionally shooting cannonballs.
/// The synchronization between the ship and the cannonballs is managed by @ref
/// ObjectHeyhoShipManager.
class ObjectHeyhoShip final : public ObjectProjectileLauncher {
public:
    /// @addr{0x806D18FC}
    /// @copydoc ObjectProjectileLauncher::ObjectProjectileLauncher(const System::MapdataGeoObj &)
    /// @details Sets the amplitude of the ship's vertical motion (@ref m_yAmplitude) based on param
    /// setting 2 and initializes @ref m_frame to zero. Finally, registers this object to the vector
    /// of managed objects in @ref ObjectDirector.
    ObjectHeyhoShip(const System::MapdataGeoObj &params)
        : ObjectProjectileLauncher(params),
          m_yAmplitude(static_cast<f32>(static_cast<s16>(params.setting(1)))),
          m_frame(0) {
        registerManagedObject();
    }

    /// @addr{0x806D2320}
    /// @brief Default virtual destructor
    ~ObjectHeyhoShip() override = default;

    /// @addr{0x806D19D8}
    /// @copybrief ObjectBase::init()
    void init() override {
        m_railInterpolator->init(0.0f, 0);
        m_railInterpolator->setPerPointVelocities(true);
        setPos(m_railInterpolator->curPos());

        const EGG::Vector3f &railTan = m_railInterpolator->curTangentDir();
        EGG::Vector3f tangent = EGG::Vector3f(railTan.x, 0.0f, railTan.z);
        tangent.normalise2();
        tangent = RotateXZByYaw(F_PI / 2.0f, tangent);

        if (EGG::Mathf::abs(tangent.y) > 0.1f) {
            tangent.y = 0.01f;
        }

        if (tangent.squaredLength() > std::numeric_limits<f32>::epsilon()) {
            tangent.normalise2();
        } else {
            tangent = EGG::Vector3f::ey;
        }

        m_framesSinceLastLaunch = 1000;
        setMatrixTangentTo(EGG::Vector3f::ey, tangent);
    }

    /// @addr{0x806D1B9C}
    /// @copybrief ObjectBase::calc()
    void calc() override {
        if (m_railInterpolator->calc() == RailInterpolator::Status::SegmentEnd) {
            m_framesSinceLastLaunch = 0;
        } else {
            ++m_framesSinceLastLaunch;
        }

        calcPos();
    }

    /// @addr{0x806D2360}
    /// @copybrief ObjectBase::loadFlags()
    /// @return Returns @ref eLoadFlags::Calc, so that object is calculated every frame.
    [[nodiscard]] LoadFlags loadFlags() const override {
        return LoadFlags(eLoadFlags::Calc);
    }

    /// @addr{0x806D1CA4}
    /// @brief Used by @ref ObjectHeyhoShipManager to check which cannonball index (if any) should
    /// be launched
    /// @return The index of the rail point from which to launch the cannonball, or -1 if no
    /// cannonball should be launched this frame.
    [[nodiscard]] s16 launchPointIdx() override {
        return m_framesSinceLastLaunch == 0 ? m_railInterpolator->curPointIdx() : -1;
    }

    /// @addr{0x806D1CC4}
    /// @brief Initializes the rail interpolator to the beginning of the given segment index and
    /// returns the tangent
    /// @param idx The segment index to initialize the rail interpreter at
    /// @return The current tangent direction along the rail
    const EGG::Vector3f &initRailDir(u16 idx) {
        m_railInterpolator->init(0.0f, static_cast<u32>(idx));
        return m_railInterpolator->curTangentDir();
    }

    /// @addr{0x806D1D10}
    /// @brief Updates the ship's position along the rail and applies a bobbing effect
    void calcPos() {
        constexpr f32 PERIOD = 100.0f;

        setPos(m_railInterpolator->curPos());

        f32 fidx = DEG2FIDX * (360.0f * static_cast<f32>(++m_frame) / PERIOD);
        f32 posY = m_yAmplitude * EGG::Mathf::SinFIdx(fidx) + pos().y;
        setPos(EGG::Vector3f(pos().x, posY, pos().z));
    }

private:
    const f32 m_yAmplitude;      ///< How much the ship bobs up and down
    u32 m_frame;                 ///< Number of frames since the ship was initialized
    u32 m_framesSinceLastLaunch; ///< Number of frames since the last cannonball was launched
};

} // namespace Kinoko::Field
