#pragma once

#include "game/kart/KartObject.hh"

namespace Kinoko::Field {

/// @brief Manages state information related to Lakitu movement
class JugemMove {
public:
    JugemMove(const Kart::KartObject *kartObj);
    ~JugemMove();

    void init();
    void calc();

    /// @addr{0x8071EFA8}
    /// @brief Sets Lakitu's position and optionally sets the transformed position
    /// @param pos The new position to set
    /// @param transPos If true, also sets the transformed position to the same value
    void setPos(const EGG::Vector3f &pos, bool transPos) {
        m_pos = pos;

        if (transPos) {
            m_transPos = pos;
        }
    }

    void setForwardFromKartObjPosDelta(bool setCurr);
    void setForwardFromKartObjMainRot(bool setCurr);

    /// @beginSetters
    /// @addr{0x8071EFD8}
    void setAnchorPos(const EGG::Vector3f &v) {
        m_anchorPos = v;
    }

    void setRiseVel(const EGG::Vector3f &v) {
        m_riseVel = v;
    }

    void setAwayOrDescending(bool isSet) {
        m_isAwayOrDescending = isSet;
    }

    void setDescending(bool isSet) {
        m_isDescending = isSet;
    }

    void setRising(bool isSet) {
        m_isRising = isSet;
    }
    /// @endSetters

    /// @beginGetters
    const EGG::Vector3f &transPos() const {
        return m_transPos;
    }

    const EGG::Matrix34f &transform() const {
        return m_transform;
    }
    /// @endGetters

private:
    EGG::Matrix34f calcOrthonormalBasis();
    EGG::Vector3f calcOscillation(const EGG::Matrix34f &mat);
    [[nodiscard]] EGG::Matrix34f calcLeanBasis();

    /// @brief Linearly interpolates between two vectors
    /// @param t Interpolation factor
    /// @param v0 Starting vector
    /// @param v1 Ending vector
    /// @return The interpolated vector
    [[nodiscard]] static EGG::Vector3f Interpolate(f32 t, const EGG::Vector3f &v0,
            const EGG::Vector3f &v1) {
        return v0 + (v1 - v0) * t;
    }

    const Kart::KartObject *m_kartObj; ///< Pointer to the kart object that Lakitu is following
    EGG::Vector3f m_pos;               ///< Lakitu's position in world space
    EGG::Vector3f m_transPos;   ///< Lakitu's world position after applying the oscillation offset
    EGG::Vector3f m_anchorPos;  ///< The position that Lakitu is trying to move towards
    EGG::Matrix34f m_leanMat;   ///< Tilts the up vector based on movement direction
    EGG::Matrix34f m_rtMat;     ///< Describes the Lakitu's orientation
    EGG::Matrix34f m_transform; ///< Combines the lean and rotation matrices
    f32 m_phaseX;               ///< Left/right oscillation phase
    f32 m_phaseY;               ///< Up/down oscillation phase
    EGG::Vector3f m_lastKartObjPos; ///< Position of the kart on the previous frame
    EGG::Vector3f m_vel;            ///< Velocity of Lakitu's movement
    EGG::Vector3f m_riseVel;        ///< Velocity of Lakitu once he leaves by rising upwards
    EGG::Vector3f m_dir;            ///< Smoothed direction of Lakitu's movement
    EGG::Vector3f m_currForward;    ///< Lakitu's current facing direction
    EGG::Vector3f m_targetForward;  ///< Lakitu's target facing direction
    f32 m_forwardInterpRate;        ///< Interpolation rate for Lakitu's smoothed facing direction
    bool m_isAwayOrDescending;      ///< True if Lakitu is inactive or is descending from the sky
    f32 m_velInterpRate;            ///< Interpolation rate for Lakitu's smoothed velocity
    bool m_isDescending;            ///< True if Lakitu is descending from the sky (spawning)
    bool m_isRising;                ///< True if Lakitu is rising up into the sky (despawning)
};

} // namespace Kinoko::Field
