#pragma once

#include <abstract/g3d/ResFile.hh>

namespace Kinoko::Render {

class DrawMdl;

/// @brief Types of animations managed by @ref AnmMgr
enum class AnmType : s32 {
    Empty = -1, ///< No animation
    Chr = 0,    ///< Model movement animations
    Clr = 1,    ///< Color changing animations
    Srt = 2,    ///< Texture movement animations
    Pat = 3,    ///< Texture swapping animations
    Shp = 4,    ///< Polygon shape morphing animations

    Max = 5, ///< Maximum number of animation types
};

/// @brief Wrapper/interface that represents a CHR0 animation bound to a model
class AnmNodeChr {
public:
    AnmNodeChr(Abstract::g3d::AnmObjChrRes anmObjChrRes) : m_anmObjChrRes(anmObjChrRes) {}

    /// @addr{0x8055AE90}
    /// @brief Returns the total number of frames in the animation
    [[nodiscard]] f32 frameCount() const {
        return static_cast<f32>(m_anmObjChrRes.frameCount());
    }

    /// @addr{0x8055ADFC}
    /// @brief Returns the current frame of the animation
    [[nodiscard]] f32 frame() const {
        return m_anmObjChrRes.frame();
    }

private:
    Abstract::g3d::AnmObjChrRes m_anmObjChrRes; ///< The underlying resource object
};

/// @brief Manager class for handling different types of animations bound to a model
class AnmMgr {
public:
    /// @addr{0x80555750}
    /// @brief Constructs an animation manager tied to the provided @ref DrawMdl
    /// @param drawMdl The model to which this animation manager is tied
    AnmMgr(DrawMdl *drawMdl) : m_parent(drawMdl) {}

    /// @addr{0x8055597C}
    /// @brief Registers the provided animation to the manager's list of animations
    /// @details For the purposes of time trial synchronization, we only need to implement CHR0
    /// animations.
    /// @param resFile The resource file containing the animation
    /// @param name The name of the animation within the resource file
    /// @param anmType The type of the animation
    void linkAnims(size_t /*idx*/, const Abstract::g3d::ResFile *resFile, const char *name,
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
    void playAnim(f32 /*frame*/, f32 /*rate*/, size_t idx) {
        ASSERT(idx < m_anmList.size());
        AnmNodeChr *&activeChrAnim = m_activeAnims[static_cast<size_t>(AnmType::Chr)];
        activeChrAnim = &(*std::next(m_anmList.begin(), idx));
    }

    /// @addr{0x80557340}
    /// @brief Retrieves the actively playing animation of the specified type
    /// @param anmType The type of the animation to retrieve
    /// @return A pointer to the actively playing animation of the specified type, or nullptr if
    /// none is active
    [[nodiscard]] const AnmNodeChr *activeAnim(AnmType anmType) const {
        return m_activeAnims[static_cast<size_t>(anmType)];
    }

private:
    [[maybe_unused]] DrawMdl *m_parent; ///< The model to which this animation manager is tied
    alloc_list<AnmNodeChr> m_anmList;   ///< A list of all animations registered with the manager

    /// @brief The currently active animations for each animation type
    /// @details In the base game, the array is of length 4, which implies that it does not track
    /// active @ref AnmType::Shp animations.
    std::array<AnmNodeChr *, static_cast<size_t>(AnmType::Max) - 1> m_activeAnims;
};

} // namespace Kinoko::Render
