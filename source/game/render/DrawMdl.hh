#pragma once

#include <Common.hh>

namespace Kinoko {

namespace Abstract::g3d {

class ResFile;

} // namespace Abstract::g3d

namespace Render {

class AnmMgr;
enum class AnmType;

/// @brief Represesnts a renderable 3D model objects
/// @details For the purposes of Kinoko, we simply implement this as a wrapper around a @ref AnmMgr
/// so that we can support objects whose physics rely on animations.
class DrawMdl {
public:
    DrawMdl();
    ~DrawMdl();

    void linkAnims(size_t idx, const Abstract::g3d::ResFile *resFile, const char *name,
            AnmType anmType);
    /// @beginGetters

    [[nodiscard]] AnmMgr *anmMgr();
    [[nodiscard]] const AnmMgr *anmMgr() const;

    /// @endGetters

private:
    AnmMgr *m_anmMgr; ///< The animation manager associated with this model
};

} // namespace Render

} // namespace Kinoko
