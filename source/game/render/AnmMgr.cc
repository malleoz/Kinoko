#include "AnmMgr.hh"

#include <abstract/g3d/ResFile.hh>

namespace Kinoko::Render {

/// @brief Constructor
/// @param anmObjChrRes The CHR0 animation resource to be wrapped by this node
/// @details For Kinoko, we can safely move the resource into the node, transferring ownership.
AnmNodeChr::AnmNodeChr(Abstract::g3d::AnmObjChrRes &&anmObjChrRes)
    : m_anmObjChrRes(std::move(anmObjChrRes)) {}

/// @addr{0x8055AE90}
/// @brief Gets the total number of frames in the animation
/// @return The total number of frames in the animation
f32 AnmNodeChr::frameCount() const {
    return static_cast<f32>(m_anmObjChrRes.frameCount());
}

/// @addr{0x8055ADFC}
/// @brief Gets the current frame of the animation
/// @return The current frame of the animation
f32 AnmNodeChr::frame() const {
    return m_anmObjChrRes.frame();
}

/// @addr{0x80555750}
/// @brief Constructs an animation manager tied to the provided @ref DrawMdl
/// @param drawMdl The model to which this animation manager is tied
AnmMgr::AnmMgr(DrawMdl *drawMdl) : m_parent(drawMdl) {}

/// @brief Default destructor
AnmMgr::~AnmMgr() = default;

/// @addr{0x8055597C}
/// @brief Registers the provided animation to the manager's list of animations
/// @details For the purposes of time trial synchronization, we only need to implement CHR0
/// animations.
/// @param resFile The resource file containing the animation
/// @param name The name of the animation within the resource file
/// @param anmType The type of the animation
void AnmMgr::linkAnims(size_t /*idx*/, const Abstract::g3d::ResFile *resFile, const char *name,
        AnmType anmType) {
    // For now, we only care about Chr
    switch (anmType) {
    case AnmType::Chr:
        m_anmList.emplace_back(resFile->resAnmChr(name));
        break;
    default:
        break;
    }
}

/// @addr{0x805573CC}
/// @brief Plays the animation at the specified index in the list of registered animations
/// @param idx The index of the animation to play
void AnmMgr::playAnim(f32 /*frame*/, f32 /*rate*/, size_t idx) {
    ASSERT(idx < m_anmList.size());
    AnmNodeChr *&activeChrAnim = m_activeAnims[static_cast<size_t>(AnmType::Chr)];
    activeChrAnim = &(*std::next(m_anmList.begin(), idx));
}

/// @addr{0x80557340}
/// @brief Retrieves the actively playing animation of the specified type
/// @param anmType The type of the animation to retrieve
/// @return A pointer to the actively playing animation of the specified type, or `nullptr` if
/// none is active
const AnmNodeChr *AnmMgr::activeAnim(AnmType anmType) const {
    return m_activeAnims[static_cast<size_t>(anmType)];
}

} // namespace Kinoko::Render
