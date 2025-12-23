#pragma once

#include "game/kart/KartObjectProxy.hh"

namespace Kart {

/// @brief Mainly responsible for calculating scaling for the squish/unsquish animation.
class KartScale : protected KartObjectProxy {
public:
    KartScale();
    ~KartScale();

    void reset();
    void calc();

    void startCrush();
    void startUncrush();
    void FUN_8056AFB4(s32 unk);
    void FUN_8056B168(s32 unk);

    [[nodiscard]] const EGG::Vector3f &_2c() const {
        return m_2c;
    }

    [[nodiscard]] const EGG::Vector3f &currScale() const {
        return m_currScale;
    }

private:
    enum class CrushState {
        None = -1,
        Crush = 0,
        Uncrush = 1,
    };

    void calcCrush();

    [[nodiscard]] EGG::Vector3f getAnmScale(f32 frame) const;

    s32 m_type;
    EGG::Vector3f m_14;
    EGG::Vector3f m_20;
    EGG::Vector3f m_2c;
    bool m_38;
    f32 m_3c;
    CrushState m_crushState;   ///< Specifies the current crush/uncrush state
    bool m_calcCrush;          ///< Set while crush scaling is occurring
    f32 m_uncrushAnmFrame;     ///< Current frame of the unsquish animation
    EGG::Vector3f m_currScale; ///< The computed scale for the current frame

    static constexpr f32 CRUSH_SCALE = 0.3f;

    static constexpr std::array<f32, 4> _808B50B0 = {{
            0.5f,
            1.0f,
            1.0f,
            2.0f,
    }};

    static constexpr std::array<f32, 4> _808B50C0 = {{
            1.0f,
            0.5f,
            2.0f,
            1.0f,
    }};
};

} // namespace Kart
