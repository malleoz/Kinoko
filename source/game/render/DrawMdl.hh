#pragma once

#include "game/render/AnmMgr.hh"

namespace Render {

class DrawMdl {
public:
    DrawMdl() : m_anmMgr(nullptr) {}

    void init(Type type, u32 flags, void *param4, void *param5, int param6, int param7);
    void initAnims(const char *name, const Abstract::g3d::ResFile *resFile, void *param4);
    void linkAnims(size_t idx, const Abstract::g3d::ResFile *resFile, const char *name,
            AnmType anmType);

    [[nodiscard]] AnmMgr *anmMgr() {
        return m_anmMgr;
    }

private:
    enum class Type {
        ScnMdlSimple = 0,
        ScnMdl = 1,
        ScnMdl1Mat1Shp = 2,
        ScnRfl = 3,
    };

    u32 m_flags;
    AnmMgr *m_anmMgr;
};

} // namespace Render
