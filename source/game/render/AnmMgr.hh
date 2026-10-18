#pragma once

#include <Types.hh>

#include <abstract/g3d/ResAnmChr.hh>

namespace Kinoko {

namespace Abstract::g3d {

class ResFile;

} // namespace Abstract::g3d

namespace Render {

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
    AnmNodeChr(Abstract::g3d::AnmObjChrRes &&anmObjChrRes);

    [[nodiscard]] f32 frameCount() const;
    [[nodiscard]] f32 frame() const;

private:
    const Abstract::g3d::AnmObjChrRes m_anmObjChrRes; ///< The underlying resource object
};

/// @brief Manager class for handling different types of animations bound to a model
class AnmMgr {
public:
    AnmMgr(DrawMdl *drawMdl);
    ~AnmMgr();

    void linkAnims(size_t idx, const Abstract::g3d::ResFile *resFile, const char *name,
            AnmType anmType);

    void playAnim(f32 frame, f32 rate, size_t idx);

    /// @beginGetters

    [[nodiscard]] const AnmNodeChr *activeAnim(AnmType anmType) const;

    /// @endGetters

private:
    [[maybe_unused]] const DrawMdl *const m_parent; ///< The model this animation manager is tied to
    alloc_list<AnmNodeChr> m_anmList; ///< A list of all animations registered with the manager

    /// @brief The currently active animations for each animation type
    /// @details In the base game, the array is of length 4, which implies that it does not track
    /// active @ref AnmType::Shp animations.
    std::array<AnmNodeChr *, static_cast<size_t>(AnmType::Max) - 1> m_activeAnims;
};

} // namespace Render

} // namespace Kinoko
