#include "ResourceManager.hh"

namespace Kinoko::System {

static const char *const RESOURCE_PATHS[] = {
        "/Race/Common",
        nullptr,
};

/// @addr{0x80540450}
/// @brief Loads the archive at the specified index from the provided filename
/// @param idx The index of the archive to load
/// @param filename The filename of the archive to load
/// @return A pointer to the loaded @ref MultiDvdArchive
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

ResourceManager *ResourceManager::s_instance = nullptr;

} // namespace Kinoko::System
