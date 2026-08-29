#include "ResourceManager.hh"

namespace Kinoko::System {

#define ARCHIVE_COUNT 2

static const char *const RESOURCE_PATHS[] = {
        "/Race/Common",
        nullptr,
};

/// @addr{0x80540450}
MultiDvdArchive *ResourceManager::load(s32 idx, const char *filename) {
    // Course has a dedicated load function, so we do not want it here
    ASSERT(idx != 1);

    if (!filename) {
        filename = RESOURCE_PATHS[idx];
    }

    if (!m_archives[idx]->isLoaded() && filename) {
        m_archives[idx]->load(filename);
    }

    return m_archives[idx];
}

/// @addr{0x8053FCEC}
ResourceManager::ResourceManager() {
    m_archives = static_cast<MultiDvdArchive **>(
            EGG::egg_alloc(ARCHIVE_COUNT * sizeof(MultiDvdArchive *)));
    for (u8 i = 0; i < ARCHIVE_COUNT; i++) {
        m_archives[i] = Create(i);
    }
}

/// @addr{0x8053FF1C}
ResourceManager::~ResourceManager() {
    if (s_instance) {
        s_instance = nullptr;
        WARN("ResourceManager instance not explicitly handled!");
    }
}

ResourceManager *ResourceManager::s_instance = nullptr;

} // namespace Kinoko::System
