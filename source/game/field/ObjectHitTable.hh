#pragma once

#include "game/field/obj/ObjectId.hh"

#include "game/kart/KartCollide.hh"

#include "game/system/ResourceManager.hh"

namespace Kinoko::Field {

/// @brief Parses the `GeoHitTableKart.bin` and `GeoHitTableKartObj.bin` tables which map an @ref
/// ObjectId to the associated reaction when hit by a kart and vice versa
/// @details These files contain a header and two data sections. The header contains the number of
/// objects stored in the table and the size of each entry in the first data section (excluding
/// the 2 byte @ref ObjectId field). The first data section contains an @ref ObjectId, followed by
/// four fields:
/// * Interaction type without any power-up status
/// * Interaction type with a Star
/// * Interaction type with a Mega Mushroom
/// * Interaction type with a Bullet Bill
/// For the purposes of Kinoko, we only care about the first field. Following this section is a
/// lookup table which maps an ObjectId to the index of the corresponding interaction entry in the
/// first data section.
class ObjectHitTable {
public:
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
    ObjectHitTable(const char *filename) {
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
    ~ObjectHitTable() = default;

    /// @beginGetters

    /// @brief Gets the reaction for the specified entry in the first data section
    /// @param i The index of the entry to get the reaction for
    /// @return The @ref Kart::Reaction for the specified entry
    [[nodiscard]] Kart::Reaction reaction(s16 i) const {
        ASSERT(i != -1);
        ASSERT(i < m_count);
        return static_cast<Kart::Reaction>(m_reactions[i]);
    }

    /// @brief Uses the lookup table in the second data section to find the index in the first data
    /// section corresponding to the provided @ref ObjectId
    /// @param id The @ref ObjectId to get the index for
    /// @return The index in the first data section corresponding to the provided @ref ObjectId, or
    /// -1 if not found
    [[nodiscard]] s16 slot(ObjectId id) const {
        constexpr size_t SLOT_COUNT = 0x2f4;

        size_t i = static_cast<std::underlying_type_t<ObjectId>>(id);
        return i < SLOT_COUNT ? parse<s16>(m_slots[i]) : -1;
    }

    /// @endGetters

private:
    s16 m_count;      ///< Endian-corrected number of entries in the first data section
    s16 m_fieldCount; ///< Number of fields in the first data section (excluding the @ref ObjectId)

    /// @brief Array of interaction types for each entry in the first data section, indexed by the
    /// entry's position in the first data section
    /// @details Since we do not need to implement interactions for power-ups, we only store the
    /// first field of each entry in the first data section.
    owning_span<s16> m_reactions;

    /// @brief Pointer to the start of the second data section of ObjHitTableKart.bin, which is a
    /// lookup table mapping @ref ObjectId to the index of the corresponding entry in the first data
    /// section
    const s16 *m_slots;
};

} // namespace Kinoko::Field
