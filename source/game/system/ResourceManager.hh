#pragma once

#include "game/system/MultiDvdArchive.hh"

namespace Kinoko {

namespace Host {

class Context;

} // namespace Host

namespace System {

/// @brief Unique identifiers for the archives loaded by @ref ResourceManager
enum class ArchiveId {
    Core = 0,   ///< Represents `Common.szs`
    Course = 1, ///< Represents course-specific archives
};

/// @addr{0x809BD738}
/// @brief Highest level abstraction for archive management and subsequent file retrieval
/// @details ResourceManager is responsible for loading and unloading archives. For example, it is
/// used by @ref Field::CourseColMgr to load the KCL collision file from a particular course
/// archive.
class ResourceManager : EGG::Disposer {
    /// @brief Grants access to the singleton so a @ref Host::Context can restore the instance's
    /// state on context switch
    friend class Host::Context;

public:
    /// @addr{0x805411FC}
    /// @brief Retrieves a file from the loaded archive
    /// @param filename The name of the file to retrieve
    /// @param id The @ref ArchiveId of the archive to retrieve the file from
    /// @return A span containing the file's contents, or an empty span if the file is not found
    [[nodiscard]] std::span<const u8> getFile(const char *filename, ArchiveId id) {
        s32 idx = static_cast<s32>(id);
        return m_archives[idx]->isLoaded() ? m_archives[idx]->getFile(filename) :
                                             std::span<const u8>{};
    }

    /// @addr{0x805414A8}
    /// @brief Retrieves the BSP file for the specified vehicle
    /// @param vehicle The @ref Vehicle whose BSP file is to be retrieved
    /// @return A span containing the BSP file's contents, or an empty span if the file is not found
    [[nodiscard]] std::span<const u8> getBsp(Vehicle vehicle) {
        char buffer[32];

        const char *name = GetVehicleName(vehicle);
        snprintf(buffer, sizeof(buffer), "/bsp/%s.bsp", name);

        return m_archives[0]->isLoaded() ? m_archives[0]->getFile(buffer) : std::span<const u8>{};
    }

    /// @addr{0x80540760}
    /// @brief Loads the archive for the specified course
    /// @param courseId The @ref Course identifying the course archive to load
    /// @return A pointer to the loaded @ref MultiDvdArchive
    [[nodiscard]] MultiDvdArchive *load(Course courseId) {
        char buffer[256];
        snprintf(buffer, sizeof(buffer), "Race/Course/%s",
                COURSE_NAMES[static_cast<s32>(courseId)]);
        m_archives[1]->load(buffer);
        return m_archives[1];
    }

    [[nodiscard]] MultiDvdArchive *load(s32 idx, const char *filename);

    /// @addr{0x805411E4}
    /// @brief Unmounts the specified @ref MultiDvdArchive
    /// @param archive The archive to unmount
    void unmount(MultiDvdArchive *archive) {
        archive->unmount();
    }

    /// @addr{0x805419EC}
    /// @brief Retrieves the name of the specified vehicle
    /// @param vehicle The vehicle whose name is to be retrieved
    /// @return The name of the vehicle, or nullptr if the vehicle is invalid
    [[nodiscard]] static const char *GetVehicleName(Vehicle vehicle) {
        return vehicle < Vehicle::Max ? VEHICLE_NAMES[static_cast<u8>(vehicle)] : nullptr;
    }

    /// @addr{0x8053FC4C}
    /// @brief Creates the singleton instance of the @ref ResourceManager
    /// @return A pointer to the newly created @ref ResourceManager instance
    static ResourceManager *CreateInstance() {
        ASSERT(!s_instance);
        s_instance = EGG::egg_new<ResourceManager>();
        return s_instance;
    }

    /// @addr{0x8053FC9C}
    /// @brief Destroys the singleton instance of the @ref ResourceManager
    static void DestroyInstance() {
        ASSERT(s_instance);
        auto *instance = s_instance;
        s_instance = nullptr;
        EGG::egg_delete(instance);
    }

    /// @brief Returns the singleton instance of the @ref ResourceManager
    /// @return A pointer to the singleton instance of the @ref ResourceManager
    [[nodiscard]] static ResourceManager *Instance() {
        return s_instance;
    }

private:
    EGG_NEW_DELETE_FRIEND

    ResourceManager();
    ~ResourceManager() override;

    fixed_vector<MultiDvdArchive *> m_archives; ///< One archive for each @ref ArchiveId

    /// @addr{0x8052A098}
    /// @brief Creates a new @ref MultiDvdArchive instance based on the specified index
    /// @param i The index of the archive to create
    /// @return A pointer to the newly created @ref MultiDvdArchive instance
    /// @details In the base game, this switch statement is used to create derived classes based on
    /// whether it is a course, UI, or Common archive. For Kinoko, we don't need to make this
    /// distinction.
    [[nodiscard]] static MultiDvdArchive *Create(u8 i) {
        switch (i) {
        default:
            return EGG::egg_new<MultiDvdArchive>();
        }
    }

    /// @brief The total number of archives managed by the ResourceManager
    static constexpr size_t ARCHIVE_COUNT = 2;

    static ResourceManager *s_instance; ///< @addr{0x809BD738}
};

} // namespace System

} // namespace Kinoko
