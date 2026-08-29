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
    void init() override {
        m_railInterpolator->init(0.0f, 0);
        m_curTangentDir = m_railInterpolator->curTangentDir();
        m_railInterpolator->setPerPointVelocities(true);
        setScale(EGG::Vector3f::unit);
    }

    /// @addr{0x8075DCA0}
    void calc() override {
        m_railInterpolator->calc();
        setPos(m_railInterpolator->curPos());
        calcTangent();
    }

    /// @addr{0x8075E744}
    [[nodiscard]] u32 loadFlags() const override {
        return 1;
    }

    /// @addr{0x8075E1F4}
    Kart::Reaction onCollision(Kart::KartObject * /*kartObj*/, Kart::Reaction reactionOnKart,
            Kart::Reaction /*reactionOnObj*/, EGG::Vector3f & /*hitDepth*/) override {
        return reactionOnKart;
    }

private:
    /// @addr{0x8075E070}
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

    EGG::Vector3f m_curTangentDir; ///< Direction of the tangent to the rail at the current position
};

} // namespace Kinoko::Field
