#pragma once

#include "Singleton.hh"

#include "game/kart/KartParam.hh"

#include "game/system/ResourceManager.hh"

namespace Kart {

/// @brief Abstraction for the process of retrieving kart parameters from files.
/// @details This has been modified from the base game in order to perform validation and make the
/// class accessible as a singleton.
class KartParamFileManager : EGG::Disposer {
public:
    void clear();
    void init();
    [[nodiscard]] EGG::RamStream getDriverStream(Character character) const;
    [[nodiscard]] EGG::RamStream getVehicleStream(Vehicle vehicle) const;
    [[nodiscard]] EGG::RamStream getHitboxStream(Vehicle vehicle) const;
    [[nodiscard]] EGG::RamStream getBikeDispParamsStream(Vehicle vehicle) const;

    static KartParamFileManager *CreateInstance();
    void DestroyInstance();

private:
    template <typename T>
    struct ParamFile {
        u32 count;
        T params[];
    };

    struct FileInfo {
        void clear() {
            file = nullptr;
            size = 0;
        }

        void load(const char *filename) {
            auto *resourceManager = Singleton<System::ResourceManager>::Instance();
            file = resourceManager->getFile(filename, &size, System::ArchiveId::Core);
        }

        void *file;
        size_t size;
    };

    KartParamFileManager();
    ~KartParamFileManager() override;

    [[nodiscard]] bool validate() const;

    FileInfo m_kartParam;     // kartParam.bin
    FileInfo m_driverParam;   // driverParam.bin
    FileInfo m_bikeDispParam; // bikePartsDispParam.bin
};

} // namespace Kart
