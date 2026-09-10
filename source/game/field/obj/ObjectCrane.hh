#pragma once

#include "game/field/obj/ObjectKCL.hh"

namespace Kinoko::Field {

/// @brief The moving crane platforms after the second turn of Toad's Factory.
/// @details Moves in a simple sine wave. There is both an x-axis and y-axis wave, though the
/// platforms in the base game have a Y amplitude of 0.
class ObjectCrane final : public ObjectKCL {
public:
    ObjectCrane(const System::MapdataGeoObj &params);

    /// @addr{0x807FEB28}
    /// @brief Default virtual destructor
    ~ObjectCrane() override = default;

    void calc() override;

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
    const EGG::Vector3f m_startPos; ///< Initial starting position
    u16 m_xt;                       ///< Current time along the x-axis period
    u16 m_yt;                       ///< Current time along the y-axis period
    const u16 m_xPeriod;            ///< Framecount of a full oscillation on x-axis
    const u16 m_yPeriod;            ///< Framecount of a full oscillation on y-axis
    const u16 m_xAmplitude;         ///< Max x-position delta from starting position
    const u16 m_yAmplitude;         ///< Max y-position delta from starting position
    const f32 m_xFreq;              ///< 2pi / m_xPeriod
    const f32 m_yFreq;              ///< 2pi / m_yPeriod
};

} // namespace Kinoko::Field
