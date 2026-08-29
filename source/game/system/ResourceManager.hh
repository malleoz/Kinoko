#pragma once

#include "game/system/MultiDvdArchive.hh"

namespace Kinoko {

namespace Host {

class Context;

} // namespace Host

namespace System {

enum class ArchiveId {
    Core = 0,
    Course = 1,
};

/// @addr{0x809BD738}
/// @brief Highest level abstraction for archive management and subsequent file retrieval.
/// @details ResourceManager is responsible for loading and unloading archives. For example, it is
/// used by Field::CourseColMgr to load the KCL collision file from a particular course archive.
class ResourceManager : EGG::Disposer {
    friend class Host::Context;

public:
    /// @addr{0x805411FC}
    void *getFile(const char *filename, size_t *size, ArchiveId id) {
        s32 idx = static_cast<s32>(id);
        return m_archives[idx]->isLoaded() ? m_archives[idx]->getFile(filename, size) : nullptr;
    }

    /// @addr{0x805414A8}
    [[nodiscard]] void *getBsp(Vehicle vehicle, size_t *size) {
        char buffer[32];

        const char *name = GetVehicleName(vehicle);
        snprintf(buffer, sizeof(buffer), "/bsp/%s.bsp", name);

        return m_archives[0]->isLoaded() ? m_archives[0]->getFile(buffer, size) : nullptr;
    }

    /// @addr{0x80540760}
    [[nodiscard]] MultiDvdArchive *load(Course courseId) {
        char buffer[256];
        snprintf(buffer, sizeof(buffer), "Race/Course/%s",
                COURSE_NAMES[static_cast<s32>(courseId)]);
        m_archives[1]->load(buffer);
        return m_archives[1];
    }

    [[nodiscard]] MultiDvdArchive *load(s32 idx, const char *filename);

    /// @addr{0x805411E4}
    void unmount(MultiDvdArchive *archive) {
        archive->unmount();
    }

    /// @addr{0x805419EC}
    [[nodiscard]] static const char *GetVehicleName(Vehicle vehicle) {
        return vehicle < Vehicle::Max ? VEHICLE_NAMES[static_cast<u8>(vehicle)] : nullptr;
    }

    /// @addr{0x8053FC4C}
    static ResourceManager *CreateInstance() {
        ASSERT(!s_instance);
        s_instance = EGG::egg_new<ResourceManager>();
        return s_instance;
    }

    /// @addr{0x8053FC9C}
    static void DestroyInstance() {
        ASSERT(s_instance);
        auto *instance = s_instance;
        s_instance = nullptr;
        EGG::egg_delete(instance);
    }

    [[nodiscard]] static ResourceManager *Instance() {
        return s_instance;
    }

private:
    EGG_NEW_DELETE_FRIEND

    ResourceManager();
    ~ResourceManager() override;

    // 0: Core archive
    // 1: Course archive
    MultiDvdArchive **m_archives;

    /// @addr{Inlined in 0x8053FCEC}
    [[nodiscard]] MultiDvdArchive *Create(u8 i) {
        switch (i) {
        default:
            return EGG::egg_new<MultiDvdArchive>();
        }
    }

    static ResourceManager *s_instance; ///< @addr{0x809BD738}
};

} // namespace System

} // namespace Kinoko
