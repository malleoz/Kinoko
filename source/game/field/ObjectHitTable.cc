#include "ObjectHitTable.hh"

#include "game/system/ResourceManager.hh"

#include <egg/util/Stream.hh>

namespace Kinoko::Field {

/// @addr{0x807F9278}
/// @brief Obtains a pointer to the provided filename (either GeoHitTableKart.bin or
/// GeoHitTableKartObj.bin), parses the count, parses the reactions, and obtains a pointer to the
/// second data section
ObjectHitTable::ObjectHitTable(const char *filename) {
    size_t size;
    void *file =
            System::ResourceManager::Instance()->getFile(filename, &size, System::ArchiveId::Core);

    EGG::RamStream stream = EGG::RamStream(file, size);

    m_count = stream.read_s16();
    m_fieldCount = stream.read_s16();
    m_reactions = owning_span<s16>(m_count);

    for (auto &reaction : m_reactions) {
        stream.skip(0x2);
        reaction = stream.read_s16();
        stream.skip(m_fieldCount * 2 - 2);
    }

    m_slots = reinterpret_cast<const s16 *>(stream.dataAtIndex());
}

/// @addr{0x807F9348}
ObjectHitTable::~ObjectHitTable() = default;

} // namespace Kinoko::Field
