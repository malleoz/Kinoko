#pragma once

#include "game/field/obj/ObjectKCL.hh"

namespace Kinoko::Field {

/// @brief The moving crane platforms after the second turn of Toad's Factory.
/// @details Moves in a simple sine wave. There is both an x-axis and y-axis wave, though the
/// platforms in the base game have a Y amplitude of 0.
class ObjectCrane final : public ObjectKCL {
public:
    /// @addr{0x807FE658}
    /// @copybrief ObjectKCL::ObjectKCL(const System::MapdataGeoObj &)
    /// @details Caches the object's initial position to @ref m_initPos. Sets @ref m_xPeriod and
    /// @ref m_yPeriod as the maxiumum between `2` and param setting 2 and param setting 5
    /// respectively. Sets @ref m_xAmplitude and @ref m_yAmplitude based on param setting 3 and
    /// param setting 6 respectively. Computes @ref m_xFreq and @ref m_yFreq based on the periods.
    /// Sets the initial x-axis phase based off param setting 4 and sets the y-axis phase to zero.
    ObjectCrane(const System::MapdataGeoObj &params)
        : ObjectKCL(params),
          m_initPos(pos()),
          m_xPeriod(std::max<u16>(2, params.setting(1))),
          m_yPeriod(std::max<u16>(2, params.setting(4))),
          m_xAmplitude(params.setting(2)),
          m_yAmplitude(params.setting(5)),
          m_xFreq(2.0f * F_PI / static_cast<f32>(m_xPeriod)),
          m_yFreq(2.0f * F_PI / static_cast<f32>(m_yPeriod)) {
        m_xt = params.setting(3);
        m_yt = 0;
    }

    /// @addr{0x807FEB28}
    /// @brief Default virtual destructor
    ~ObjectCrane() override = default;

    /// @addr{0x807FE7EC}
    /// @copybrief ObjectBase::calc()
    /// @details Updates the crane's position based on its x and y oscillation periods and
    /// amplitudes. Increments the x and y phase counters accordingly. Finally, computes the moving
    /// object velocity based on the change in position this frame.
    void calc() override {
        const EGG::Vector3f prevPos = pos();

        f32 xDelta = EGG::Mathf::cos(m_xFreq * static_cast<f32>(m_xt));
        EGG::Vector3f scaledX = EGG::Vector3f::ex * xDelta * static_cast<f32>(m_xAmplitude);

        f32 yDelta = EGG::Mathf::cos(m_yFreq * static_cast<f32>(m_yt));
        EGG::Vector3f scaledY = EGG::Vector3f::ey * yDelta * static_cast<f32>(m_yAmplitude);

        calcTransform();
        setPos(m_initPos + transform().multVector33(scaledX + scaledY));

        if (m_yt++ > m_yPeriod) {
            m_yt = 0;
        }

        if (m_xt++ > m_xPeriod) {
            m_xt = 0;
        }

        setMovingObjVel(pos() - prevPos);
    }

    /// @addr{0x807FEB20}
    /// @copybrief ObjectBase::loadFlags()
    /// @return Returns @ref eLoadFlags::Calc, so that object is calculated every frame.
    [[nodiscard]] LoadFlags loadFlags() const override {
        return LoadFlags(eLoadFlags::Calc);
    }

    /// @addr{0x807FEAF0}
    /// @copybrief ObjectKCL::colRadiusAdditionalLength()
    /// @return The x-axis amplitude of the crane's oscillation
    [[nodiscard]] f32 colRadiusAdditionalLength() const override {
        return m_xAmplitude;
    }

private:
    const EGG::Vector3f m_initPos; ///< Initial starting position
    u16 m_xt;                      ///< Current time along the x-axis period
    u16 m_yt;                      ///< Current time along the y-axis period
    const u16 m_xPeriod;           ///< Framecount of a full oscillation on x-axis
    const u16 m_yPeriod;           ///< Framecount of a full oscillation on y-axis
    const u16 m_xAmplitude;        ///< Max x-position delta from starting position
    const u16 m_yAmplitude;        ///< Max y-position delta from starting position
    const f32 m_xFreq;             ///< 2pi / m_xPeriod
    const f32 m_yFreq;             ///< 2pi / m_yPeriod
};

} // namespace Kinoko::Field
