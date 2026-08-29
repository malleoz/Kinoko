#include "DvdArchive.hh"

namespace Kinoko::System {

/// @addr{0x80518CC0}
DvdArchive::DvdArchive()
    : m_archive(nullptr), m_archiveStart(nullptr), m_archiveSize(0), m_fileStart(nullptr),
      m_fileSize(0), m_state(State::Cleared) {}

/// @addr{0x80518CF4}
DvdArchive::~DvdArchive() {
    unmount();
}

/// @addr{0x80519420}
void *DvdArchive::getFile(const char *filename, size_t *size) const {
    if (m_state != State::Mounted) {
        return nullptr;
    }

    char buffer[256];
    if (filename[0] == '/') {
        snprintf(buffer, sizeof(buffer), "%s", filename);
    } else {
        snprintf(buffer, sizeof(buffer), "/%s", filename);
    }
    buffer[sizeof(buffer) - 1] = '\0';

    Abstract::ArchiveHandle::FileInfo fileInfo{0, 0};
    s32 entryId = m_archive->convertPathToEntryId(buffer);
    if (entryId == -1) {
        return nullptr;
    }

    void *file = m_archive->getFileFast(entryId, fileInfo);
    if (file && size) {
        *size = fileInfo.length;
    }

    return file;
}

/// @addr{0x80518E10}
void DvdArchive::load(const char *path, bool decompress_) {
    if (m_state == State::Cleared) {
        rip(path);
    }

    if (m_state == State::Ripped) {
        if (decompress_) {
            decompress();
            clearFile();
        } else {
            move();
        }
        mount();
    }
}

/// @addr{0x805195D8}
void DvdArchive::load(const DvdArchive *other) {
    if (other->m_state == State::Ripped) {
        m_fileStart = other->m_fileStart;
        m_state = State::Ripped;
        decompress();
        mount();
        m_fileStart = nullptr;
        m_fileSize = 0;
    } else {
        m_state = State::Cleared;
    }
}

/// @addr{0x80518FBC}
void DvdArchive::load(void *fileStart, size_t fileSize, bool decompress_) {
    m_fileStart = fileStart;
    m_fileSize = fileSize;
    if (decompress_) {
        decompress();
        m_fileStart = nullptr;
        m_fileSize = 0;
    }
    mount();
}

} // namespace Kinoko::System
