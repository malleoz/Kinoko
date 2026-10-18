#include "ResourceManager.hh"

#include "game/system/MultiDvdArchive.hh"

namespace Kinoko::System {

static const char *const RESOURCE_PATHS[] = {
        "/Race/Common",
        nullptr,
};

/// @addr{0x805411FC}
/// @brief Retrieves a file from the loaded archive
/// @param filename The name of the file to retrieve
/// @param id The @ref ArchiveId of the archive to retrieve the file from
/// @return A span containing the file's contents, or an empty span if the file is not found
std::span<const u8> ResourceManager::getFile(const char *filename, ArchiveId id) {
    s32 idx = static_cast<s32>(id);
    return m_archives[idx]->isLoaded() ? m_archives[idx]->getFile(filename) : std::span<const u8>{};
}

/// @addr{0x805414A8}
/// @brief Retrieves the `BSP` file for the specified vehicle
/// @param vehicle The @ref Vehicle whose `BSP` file is to be retrieved
/// @return A span containing the `BSP` file's contents, or an empty span if the file is not
/// found
std::span<const u8> ResourceManager::getBsp(Vehicle vehicle) {
    char buffer[32];

    const char *name = GetVehicleName(vehicle);
    snprintf(buffer, sizeof(buffer), "/bsp/%s.bsp", name);

    return m_archives[0]->isLoaded() ? m_archives[0]->getFile(buffer) : std::span<const u8>{};
}

/// @addr{0x80540760}
/// @brief Loads the archive for the specified course
/// @param courseId The @ref Course identifying the course archive to load
/// @return A pointer to the loaded @ref MultiDvdArchive
MultiDvdArchive *ResourceManager::load(Course courseId) {
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "Race/Course/%s", COURSE_NAMES[static_cast<s32>(courseId)]);
    m_archives[1]->load(buffer);
    return m_archives[1];
}

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

/// @addr{0x805411E4}
/// @brief Unmounts the specified @ref MultiDvdArchive
/// @param archive The archive to unmount
/// @details Simply calls @ref MultiDvdArchive::unmount() on the provided archive.
void ResourceManager::unmount(MultiDvdArchive *archive) {
    archive->unmount();
}

/// @addr{0x805419EC}
/// @brief Retrieves the name of the specified vehicle
/// @param vehicle The vehicle whose name is to be retrieved
/// @return The name of the vehicle, or `nullptr` if the vehicle is invalid
const char *ResourceManager::GetVehicleName(Vehicle vehicle) {
    return vehicle < Vehicle::Max ? VEHICLE_NAMES[static_cast<u8>(vehicle)] : nullptr;
}

/// @addr{0x8053FC4C}
/// @brief Creates the singleton instance of the @ref ResourceManager
/// @return A pointer to the newly created @ref ResourceManager instance
/// @pre The singleton instance of the @ref ResourceManager must not already exist.
ResourceManager *ResourceManager::CreateInstance() {
    ASSERT(!s_instance);
    s_instance = EGG::egg_new<ResourceManager>();
    return s_instance;
}

/// @addr{0x8053FC9C}
/// @brief Destroys the singleton instance of the @ref ResourceManager
/// @pre The singleton instance of the @ref ResourceManager must exist.
void ResourceManager::DestroyInstance() {
    ASSERT(s_instance);
    auto *instance = s_instance;
    s_instance = nullptr;
    EGG::egg_delete(instance);
}

/// @brief Returns the singleton instance of the @ref ResourceManager
/// @return A pointer to the singleton instance of the @ref ResourceManager
ResourceManager *ResourceManager::Instance() {
    return s_instance;
}

/// @addr{0x8053FCEC}
/// @brief Private constructor which allocates and creates two @ref MultiDvdArchive objects
ResourceManager::ResourceManager() : m_archives(ARCHIVE_COUNT) {
    for (u8 i = 0; i < ARCHIVE_COUNT; i++) {
        m_archives.push_back(Create(i));
    }
}

/// @addr{0x8053FF1C}
/// @brief Private virtual destructor
ResourceManager::~ResourceManager() {
    if (s_instance) {
        s_instance = nullptr;
        WARN("ResourceManager instance not explicitly handled!");
    }
}

/// @addr{0x8052A098}
/// @brief Creates a new @ref MultiDvdArchive instance based on the specified index
/// @param i The index of the archive to create
/// @return A pointer to the newly created @ref MultiDvdArchive instance
/// @details In the base game, this switch statement is used to create derived classes based on
/// whether it is a course, UI, or Common archive. For Kinoko, we don't need to make this
/// distinction.
MultiDvdArchive *ResourceManager::Create(u8 i) {
    switch (i) {
    default:
        return EGG::egg_new<MultiDvdArchive>();
    }
}

ResourceManager *ResourceManager::s_instance = nullptr;

} // namespace Kinoko::System
