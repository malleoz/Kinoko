#include "ObjectFlowTable.hh"

#include "game/system/ResourceManager.hh"

#include <cstring>

namespace Kinoko::Field {

/// @addr{0x8082C10C}
/// @brief Constructor that obtains a pointer to the provided filename (`ObjFlow.bin`), parses
/// the count, and obtains pointers to the two data sections
/// @param filename The name of the file to parse (`ObjFlow.bin`)
/// @details Loads the file and reads the @ref m_count from the header. Sets @ref m_sets and
/// @ref m_slots to point to the respective data sections.
ObjectFlowTable::ObjectFlowTable(const char *filename) {
    const SFile *file = reinterpret_cast<const SFile *>(
            System::ResourceManager::Instance()->getFile(filename, System::ArchiveId::Core).data());

    m_count = parse<s16>(file->count);
    m_sets = file->sets;
    m_slots = reinterpret_cast<const s16 *>(m_sets + m_count);
}

/// @addr{0x8082C1F4}
/// @brief Default destructor
ObjectFlowTable::~ObjectFlowTable() = default;

/// @brief Returns a pointer to the @ref SObjectCollisionSet at the provided index
/// @param slot Index of the @ref SObjectCollisionSet to retrieve
/// @return Pointer to the @ref SObjectCollisionSet, or nullptr if the index is invalid
const SObjectCollisionSet *ObjectFlowTable::set(s16 slot) const {
    return slot == -1 ? nullptr : slot < m_count ? &m_sets[slot] : nullptr;
}

/// @brief Gets the index of the @ref SObjectCollisionSet for the provided @ref ObjectId
/// @param id The @ref ObjectId to get the index for
/// @return The index of the @ref SObjectCollisionSet, or -1 if the @ref ObjectId is invalid
s16 ObjectFlowTable::slot(ObjectId id) const {
    constexpr size_t SLOT_COUNT = 0x2f4;

    size_t i = static_cast<size_t>(id);
    return i < SLOT_COUNT ? parse<s16>(m_slots[i]) : -1;
}

/// @addr{0x8082C178}
/// @brief Iterates the @ref SObjectCollisionSet entries and returns the @ref ObjectId of the
/// entry with a matching name
/// @param name The name of the object to look up
/// @return The @ref ObjectId corresponding to the provided name, or @ref ObjectId::None if not
/// found
ObjectId ObjectFlowTable::getIdFromName(const char *name) const {
    for (s16 i = 0; i < m_count; ++i) {
        const auto *curSet = set(i);
        ASSERT(curSet);

        if (strncmp(name, curSet->name, sizeof(curSet->name)) == 0) {
            return static_cast<ObjectId>(parse<u16>(curSet->id));
        }
    }

    return ObjectId::None;
}

} // namespace Kinoko::Field
