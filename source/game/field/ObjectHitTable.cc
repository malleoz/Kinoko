#include "ObjectHitTable.hh"

#include "game/kart/Reaction.hh"

#include "game/system/ResourceManager.hh"

#include <egg/util/Stream.hh>

namespace Kinoko::Field {

/// @addr{0x807F9278}
/// @brief Constructor that obtains a pointer to the provided filename (either
/// `GeoHitTableKart.bin` or `GeoHitTableKartObj.bin`), parses the count, parses the reactions,
/// and obtains a pointer to the second data section
/// @param filename The name of the file to parse (either `GeoHitTableKart.bin` or
/// `GeoHitTableKartObj.bin`)
/// @details Reads the number of entries in the first data section to @ref m_count and the size
/// of each entry to @ref m_fieldCount. Initializes @ref m_reactions to a size of @ref m_count
/// and reads in all the reactions for interactions without any power-up. Finally, sets up the
/// pointer to the second data section.
ObjectHitTable::ObjectHitTable(const char *filename) {
    std::span<const u8> file =
            System::ResourceManager::Instance()->getFile(filename, System::ArchiveId::Core);

    EGG::RamStream stream = EGG::RamStream(file.data(), static_cast<u32>(file.size()));

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
/// @brief Default destructor
ObjectHitTable::~ObjectHitTable() = default;

/// @brief Gets the reaction for the specified entry in the first data section
/// @param i The index of the entry to get the reaction for
/// @return The @ref Kart::Reaction for the specified entry in @ref m_reactions
Kart::Reaction ObjectHitTable::reaction(s16 i) const {
    ASSERT(i != -1);
    ASSERT(i < m_count);
    return static_cast<Kart::Reaction>(m_reactions[i]);
}

/// @brief Uses the lookup table in the second data section to find the index in the first data
/// section corresponding to the provided @ref ObjectId
/// @param id The @ref ObjectId to get the index for
/// @return The index in the first data section corresponding to the provided @ref ObjectId, or
/// -1 if not found
s16 ObjectHitTable::slot(ObjectId id) const {
    constexpr size_t SLOT_COUNT = 0x2f4;

    size_t i = static_cast<std::underlying_type_t<ObjectId>>(id);
    return i < SLOT_COUNT ? parse<s16>(m_slots[i]) : -1;
}

} // namespace Kinoko::Field
