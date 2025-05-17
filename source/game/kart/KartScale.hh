#pragma once

#include "game/kart/KartObjectProxy.hh"

namespace Kart {

class KartScale : protected KartObjectProxy {
public:
    KartScale();
    ~KartScale();

    void reset();
    void calc();

    void FUN_8056B060();
    void calcCrushAnimation();

    [[nodiscard]] const EGG::Vector3f &targetScale() const {
        return m_targetScale;
    }

private:
    s32 m_60;
    bool m_64;
    f32 m_scaleAnmFrame;
    EGG::Vector3f m_targetScale;
};

} // namespace Kart
