#pragma once

#include "game/kart/KartObjectProxy.hh"

namespace Kinoko::Kart {

/// @brief State management for the kart's scale and associated calculations
/// @details Mainly responsible for calculating scaling for the squish/unsquish and shrink/unshrink
/// animations.
class KartScale : private KartObjectProxy {
public:
    KartScale(const KartParam::Stats &stats);
    ~KartScale() override;

    void reset();
    void calc();

    void startCrush();
    void endCrush();
    void startShrink(bool inMega);
    void endShrink(bool inMega);

    /// @beginGetters

    [[nodiscard]] const EGG::Vector3f &shrinkScale() const;
    [[nodiscard]] const EGG::Vector3f &pressScale() const;

    /// @endGetters

private:
    /// @brief Distinguishes whether the kart is currently being crushed or uncrushed
    enum class CrushState {
        None = -1,   ///< The kart is currently in a normal state
        Crush = 0,   ///< The kart is currently crushed or is being crushed
        Uncrush = 1, ///< The kart is currently growing back to full scale
    };

    /// @brief Distinguishes between different thunder scale types
    /// @details In Kinoko, we omit types 2 and 3 which are only used for Mega Mushrooms
    enum class ThunderScaleType {
        None = -1,         ///< No thunder scale is active
        ScaleUp = 0,       ///< Thunder scale up is active
        ScaleDown = 1,     ///< Thunder scale down is active
        ScaleMegaUp = 2,   ///< Mega mushroom scale up is active
        ScaleMegaDown = 3, ///< Mega mushroom scale down is active
    };

    void calcShrink();
    void calcCrush();

    ThunderScaleType m_shrinkType;    ///< Which of the four shrink scale animations is active
    EGG::Vector3f m_shrinkOffset;     ///< Additive coefficient when calculating the shrink scale
    EGG::Vector3f m_shrinkRate;       ///< Linear slope coefficient to change the shrink scale
    EGG::Vector3f m_shrinkScale;      ///< The current scale of the kart due to shrink animations
    bool m_calcShrink;                ///< True if a shrink animation is currently active
    f32 m_shrinkAnmFrame;             ///< Current frame of the shrink/unshrink animation
    std::array<f32, 4> m_scaleTarget; ///< Target scale values for the four thunder scale types
    CrushState m_crushState;          ///< Specifies the current crush/uncrush state
    bool m_calcCrush;                 ///< Set while crush scaling is occurring
    f32 m_uncrushAnmFrame;            ///< Current frame of the unsquish animation
    EGG::Vector3f m_pressScale;       ///< The current scale of the kart due to crush animation

    [[nodiscard]] static EGG::Vector3f GetPressAnmScale(f32 frame);

    static constexpr f32 CRUSH_SCALE = 0.3f; ///< The Y-scale of the kart when it is fully squished

    /// @brief Scales at the start of the shrink animation for the four thunder scale types
    static constexpr std::array<f32, 4> s_baseScaleStart = {{
            0.5f,
            1.0f,
            1.0f,
            2.0f,
    }};

    /// @brief Scales at the end of the shrink animation for the four thunder scale types
    static constexpr std::array<f32, 4> s_baseScaleTarget = {{
            1.0f,
            0.5f,
            2.0f,
            1.0f,
    }};
};

} // namespace Kinoko::Kart
