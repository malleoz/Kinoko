#pragma once

#include "game/kart/KartObjectManager.hh"

namespace Kinoko::Kart {

/// @brief State management for the kart's scale and associated calculations
/// @details Mainly responsible for calculating scaling for the squish/unsquish and shrink/unshrink
/// animations.
class KartScale : private KartObjectProxy {
public:
    KartScale(const KartParam::Stats &stats);
    ~KartScale();

    void reset();
    void calc();

    /// @addr{0x8056B060}
    /// @brief Called when the kart starts being crushed
    /// @details Sets the crush state to @ref CrushState::Crush, resets the press scale to the
    /// default value, resets the uncrush animation framecounter, and marks that crush updates
    /// should be performed.
    void startCrush() {
        m_crushState = CrushState::Crush;
        m_pressScale = EGG::Vector3f(1.0f, 1.0f, 1.0f);
        m_uncrushAnmFrame = 0.0f;
        m_calcCrush = true;
    }

    /// @addr{0x8056B094}
    /// @brief Called when the kart finishes being crushed
    /// @details Sets the crush state to @ref CrushState::Uncrush, resets the uncrush animation
    /// framecounter, and marks that crush updates should be performed. It also sets the press
    /// scale, which will get overwritten in @ref calcCrush on the following frame. It's set here
    /// so that @ref KartMove::calcScale() can snap the kart's scale to the correct value on the
    /// current frame.
    void endCrush() {
        m_crushState = CrushState::Uncrush;
        m_pressScale = EGG::Vector3f(1.0f, CRUSH_SCALE, 1.0f);
        m_uncrushAnmFrame = 0.0f;
        m_calcCrush = true;
    }

    void startShrink(bool inMega);
    void endShrink(bool inMega);

    /// @beginGetters
    [[nodiscard]] const EGG::Vector3f &shrinkScale() const {
        return m_shrinkScale;
    }

    [[nodiscard]] const EGG::Vector3f &pressScale() const {
        return m_pressScale;
    }
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

    void calcCrush();

    /// @addr{0x8056ACF4}
    /// @brief Fetches the scale of the uncrush animation at the given frame
    /// @param frame The frame of the press animation to fetch the scale for
    /// @return The scale of the press animation at the given frame
    [[nodiscard]] static EGG::Vector3f GetPressAnmScale(f32 frame) {
        const auto *scaleAnm = KartObjectManager::PressScaleUpAnmChr();
        ASSERT(scaleAnm);
        return scaleAnm->getAnmResult(frame, 0).scale();
    }

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
