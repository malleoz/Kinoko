#include "DrawMdl.hh"

namespace Render {

/// @addr{0x8055BAA0}
void DrawMdl::init(Type type, u32 flags, void *param4, void *param5, int param6, int param7) {
    if (param4 && type == Type::ScnMdl) {
        flags |= 0x20;
        m_flags |= 0x10; // TODO: INIT
    }

    TODO;
}

/// @addr{0x8055C0E8}
void DrawMdl::initAnims(const char *name, const Abstract::g3d::ResFile *resFile, void *param4) {
    u32 flags = 0;

    if (resFile->hasResAnmShp()) {
        flags |= 0x7000;
    }

    init(Type::ScnMdl, flags, param4, nullptr, 0x80, -1);
}

/// @addr{0x8055DDEC}
void DrawMdl::linkAnims(size_t idx, const Abstract::g3d::ResFile *resFile, const char *name,
        AnmType anmType) {
    if (!m_anmMgr) {
        m_anmMgr = new AnmMgr(this);
    }

    m_anmMgr->linkAnims(idx, resFile, name, anmType);
}

} // namespace Render
