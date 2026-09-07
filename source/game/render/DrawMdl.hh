#pragma once

#include "game/render/AnmMgr.hh"

namespace Kinoko::Render {

/// @brief Represesnts a renderable 3D model objects
/// @details For the purposes of Kinoko, we simply implement this as a wrapper around a @ref AnmMgr
/// so that we can support objects whose physics rely on animations.
class DrawMdl {
public:
    /// @brief Constructor
    DrawMdl() : m_anmMgr(nullptr) {}

    /// @brief Destructor that destroys the underlying @ref AnmMgr
    ~DrawMdl() {
        EGG::egg_delete(m_anmMgr);
    }

    /// @addr{0x8055DDEC}
    /// @brief Links an animation to the model's animation manager
    /// @param resFile The resource file containing the animation
    /// @param name The name of the animation within the resource file
    /// @param anmType The type of the animation
    void linkAnims(size_t idx, const Abstract::g3d::ResFile *resFile, const char *name,
            AnmType anmType) {
        if (!m_anmMgr) {
            m_anmMgr = EGG::egg_new<AnmMgr>(this);
        }

        m_anmMgr->linkAnims(idx, resFile, name, anmType);
    }

    [[nodiscard]] AnmMgr *anmMgr() {
        return m_anmMgr;
    }

private:
    AnmMgr *m_anmMgr; ///< The animation manager associated with this model
};

} // namespace Kinoko::Render
