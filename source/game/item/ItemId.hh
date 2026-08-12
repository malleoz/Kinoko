#pragma once

namespace Kinoko::Item {

/// @brief Unique identifiers for the different items in the game
/// @details We only care about triple mushrooms for the purposes of time trials.
enum class ItemId {
    TRIPLE_MUSHROOM = 0x5, ///< Three mushroom boosts
    NONE = 0x14,           ///< No item
};

} // namespace Kinoko::Item
