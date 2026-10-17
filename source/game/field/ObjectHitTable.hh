#pragma once

#include "game/field/obj/ObjectId.hh"

namespace Kinoko {

namespace Kart {

enum class Reaction;

}

namespace Field {

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
    ObjectHitTable(const char *filename);
    ~ObjectHitTable();

    /// @beginGetters

    [[nodiscard]] Kart::Reaction reaction(s16 i) const;
    [[nodiscard]] s16 slot(ObjectId id) const;

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

} // namespace Field

} // namespace Kinoko
