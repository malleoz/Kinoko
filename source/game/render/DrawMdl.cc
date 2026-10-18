#include "DrawMdl.hh"

#include "game/render/AnmMgr.hh"

namespace Kinoko::Render {

/// @brief Constructor
/// @details Initializes @ref m_anmMgr to `nullptr`.
DrawMdl::DrawMdl() : m_anmMgr(nullptr) {}

/// @brief Destructor that destroys the underlying @ref AnmMgr
DrawMdl::~DrawMdl() {
    EGG::egg_delete(m_anmMgr);
}

/// @addr{0x8055DDEC}
/// @brief Links an animation to the model's animation manager
/// @param resFile The resource file containing the animation
/// @param name The name of the animation within the resource file
/// @param anmType The type of the animation
/// @details Creates the @ref AnmMgr if it doesn't exist already
void DrawMdl::linkAnims(size_t idx, const Abstract::g3d::ResFile *resFile, const char *name,
        AnmType anmType) {
    if (!m_anmMgr) {
        m_anmMgr = EGG::egg_new<AnmMgr>(this);
    }

    m_anmMgr->linkAnims(idx, resFile, name, anmType);
}

/// @brief Gets the animation manager associated with this model
/// @return Pointer to the animation manager associated with this model
AnmMgr *DrawMdl::anmMgr() {
    return m_anmMgr;
}

/// @brief Gets the animation manager associated with this model
/// @return Cont pointer to the animation manager associated with this model
const AnmMgr *DrawMdl::anmMgr() const {
    return m_anmMgr;
}

} // namespace Kinoko::Render
