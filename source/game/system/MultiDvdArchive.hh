#pragma once

#include "game/system/DvdArchive.hh"

const char *const SZS_EXTENSION = ".szs";

namespace Kinoko::System {

class MultiDvdArchive {
public:
    enum class Format {
        Double,
        Single,
        None,
    };

    MultiDvdArchive(u16 archiveCount = 1);
    ~MultiDvdArchive();

    void *getFile(const char *filename, size_t *size) const;
    void load(const char *filename);

    /// @addr{0x8052AAE8}
    void load(const MultiDvdArchive *other) {
        for (u16 i = 0; i < m_archiveCount; i++) {
            m_archives[i].load(&other->m_archives[i]);
        }
    }

    void rip(const char *filename);

    /// @addr{0x8052AC40}
    void clear() {
        for (u16 i = 0; i < m_archiveCount; i++) {
            m_archives[i].clear();
        }
    }

    /// @addr{0x8052AA88}
    void unmount() {
        for (u16 i = 0; i < m_archiveCount; i++) {
            m_archives[i].unmount();
        }
    }

    /// @addr{0x8052A800}
    [[nodiscard]] bool isLoaded() const {
        for (u16 i = 0; i < m_archiveCount; i++) {
            if (m_archives[i].isLoaded()) {
                return true;
            }
        }

        return false;
    }

    /// @addr{0x8052AE08}
    [[nodiscard]] u16 rippedArchiveCount() const {
        u16 count = 0;
        for (u16 i = 0; i < m_archiveCount; i++) {
            if (m_archives[i].isRipped()) {
                count++;
            }
        }

        return count;
    }

private:
    DvdArchive *m_archives;
    void **m_fileStarts;
    size_t *m_fileSizes;
    char **m_suffixes;
    Format *m_formats;
    u16 m_archiveCount;
};

} // namespace Kinoko::System
