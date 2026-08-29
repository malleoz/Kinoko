#pragma once

#include <abstract/File.hh>

#include <egg/core/Archive.hh>
#include <egg/core/Decomp.hh>

namespace Kinoko::System {

class DvdArchive {
public:
    enum class State {
        Cleared = 0,
        Ripped = 1,
        Decompressed = 2,
        Mounted = 3,
    };

    DvdArchive();
    ~DvdArchive();

    /// @addr{0x80519508}
    void decompress() {
        m_archiveSize = EGG::Decomp::GetExpandSize(reinterpret_cast<u8 *>(m_fileStart));
        m_archiveStart = static_cast<u8 *>(EGG::egg_alloc(m_archiveSize));
        EGG::Decomp::DecodeSZS(reinterpret_cast<u8 *>(m_fileStart),
                reinterpret_cast<u8 *>(m_archiveStart));
        m_state = State::Decompressed;
    }

    void *getFile(const char *filename, size_t *size) const;
    void load(const char *path, bool decompress_);
    void load(const DvdArchive *other);
    void load(void *fileStart, size_t fileSize, bool decompress_);

    /// @addr{0x80518DCC}
    void mount() {
        m_archive = EGG::Archive::Mount(m_archiveStart);
        m_state = State::Mounted;
    }

    /// @addr{0x805195A4}
    void move() {
        m_archiveStart = m_fileStart;
        m_archiveSize = m_fileSize;
        m_fileStart = nullptr;
        m_fileSize = 0;
        m_state = State::Decompressed;
    }

    /// @addr{0x805190F0}
    void rip(const char *path) {
        m_fileStart = Abstract::File::Load(path, m_fileSize);
        if (m_fileSize != 0 && m_fileStart) {
            m_state = State::Ripped;
        }
    }

    /// @addr{0x80519240}
    void clear() {
        clearArchive();
        clearFile();
    }

    /// @addr{0x80519370}
    void clearArchive() {
        if (!m_archiveStart) {
            return;
        }

        EGG::egg_free(static_cast<u8 *>(m_archiveStart));
        m_archiveStart = nullptr;
        m_archiveSize = 0;
    }

    /// @addr{0x805193C8}
    void clearFile() {
        if (!m_fileStart) {
            return;
        }

        EGG::egg_free(static_cast<u8 *>(m_fileStart));
        m_fileStart = nullptr;
        m_fileSize = 0;
    }

    /// @addr{0x805192CC}
    void unmount() {
        if (m_state == State::Mounted) {
            m_archive->unmount();
        }
        clear();
        m_state = State::Cleared;
    }

    [[nodiscard]] bool isLoaded() const {
        return m_state == State::Mounted;
    }

    [[nodiscard]] bool isRipped() const {
        return m_state == State::Ripped;
    }

private:
    EGG::Archive *m_archive;
    void *m_archiveStart;
    size_t m_archiveSize;
    void *m_fileStart;
    size_t m_fileSize;
    State m_state;
};

} // namespace Kinoko::System
