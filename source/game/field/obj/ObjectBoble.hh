#pragma once

#include "game/field/obj/ObjectCollidable.hh"

namespace Kinoko::Field {

/// @brief Rising and falling fireballs on GBA Bowser Castle 3
class ObjectBoble final : public ObjectCollidable {
public:
    /// @addr{0x8075DB3C}
    /// @brief Constructor
    /// @param params The parameters used to initialize the object
    ObjectBoble(const System::MapdataGeoObj &params) : ObjectCollidable(params) {}

    /// @addr{0x8075E74C}
    /// @brief Default virtual destructor
    ~ObjectBoble() override = default;

    /// @addr{0x8075DBA0}
    /// @copybrief ObjectBase::init()
    /// @details Initializes the rail interpolator and sets the initial tangent direction and scale.
    void init() override {
        m_railInterpolator->init(0.0f, 0);
        m_curTangentDir = m_railInterpolator->curTangentDir();
        m_railInterpolator->setPerPointVelocities(true);
        setScale(EGG::Vector3f::unit);
    }

    /// @addr{0x8075DCA0}
    /// @copybrief ObjectBase::calc()
    /// @details Updates the rail interpolator, sets the object's position based on the current rail
    /// position, and recalculates the tangent direction.
    void calc() override {
        m_railInterpolator->calc();
        setPos(m_railInterpolator->curPos());
        calcTangent();
    }

    /// @addr{0x8075E744}
    /// @copybrief ObjectBase::loadFlags()
    /// @return Returns @ref eLoadFlags::Calc, so that object is calculated every frame.
    [[nodiscard]] LoadFlags loadFlags() const override {
        return LoadFlags(eLoadFlags::Calc);
    }

private:
    /// @addr{0x8075E070}
    /// @brief Calculates the tangent direction of the object based on the rail interpolator
    /// @details Interpolates the rail's tangent direction and updates the object's transformation
    /// matrix accordingly.
    void calcTangent() {
        m_curTangentDir = Interpolate(0.2f, m_curTangentDir, m_railInterpolator->curTangentDir());
        m_curTangentDir.normalise();

        EGG::Vector3f axis = m_curTangentDir.cross(EGG::Vector3f::ex);
        if (axis.normalise() == 0.0f) {
            axis = m_curTangentDir.cross(EGG::Vector3f::ez);
            axis.normalise();
        }

        setMatrixTangentTo(axis.cross(m_curTangentDir), m_curTangentDir);
    }

    EGG::Vector3f m_curTangentDir; ///< Smooted rail tangent direction at the current position
};

} // namespace Kinoko::Field
