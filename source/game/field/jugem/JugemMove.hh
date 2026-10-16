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
    void calcForwardFromKartObjPosDelta(bool setCurr);
    void calcForwardFromKartObjMainRot(bool setCurr);

    /// @beginSetters

    void setPos(const EGG::Vector3f &pos, bool transPos);
    void setAnchorPos(const EGG::Vector3f &v);
    void setRiseVel(const EGG::Vector3f &v);
    void setAwayOrDescending(bool isSet);
    void setDescending(bool isSet);
    void setRising(bool isSet);

    /// @endSetters

    /// @beginGetters

    const EGG::Vector3f &transPos() const;
    const EGG::Matrix34f &transform() const;

    /// @endGetters

private:
    EGG::Matrix34f calcOrthonormalBasis();
    EGG::Vector3f calcOscillation(const EGG::Matrix34f &mat);
    EGG::Matrix34f calcLeanBasis();

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
    EGG::Vector3f m_smoothedVel;    ///< Smoothed velocity
    EGG::Vector3f m_currForward;    ///< Lakitu's current facing direction
    EGG::Vector3f m_targetForward;  ///< Lakitu's target facing direction
    f32 m_forwardInterpRate;        ///< Interpolation rate for Lakitu's smoothed facing direction
    bool m_isAwayOrDescending;      ///< True if Lakitu is inactive or is descending from the sky
    f32 m_velInterpRate;            ///< Interpolation rate for Lakitu's smoothed velocity
    bool m_isDescending;            ///< True if Lakitu is descending from the sky (spawning)
    bool m_isRising;                ///< True if Lakitu is rising up into the sky (despawning)
};

} // namespace Kinoko::Field
