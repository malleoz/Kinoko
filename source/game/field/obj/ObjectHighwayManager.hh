#pragma once

#include <Types.hh>

#include "game/field/obj/ObjectCollidable.hh"

namespace Kinoko::Field {

class ObjectCarTGE;

/// @brief Watcher class that checks if any car has squashed the player
/// @details This squash cooldown is used by @ref ObjectCarTGE to determine if the player can be
/// squashed again.
class ObjectHighwayManager final : public ObjectCollidable {
public:
    ObjectHighwayManager();
    ~ObjectHighwayManager() override;

    void init() override;
    void calc() override;
    void loadGraphics() override;
    void createCollision() override;
    void loadRail() override;
    [[nodiscard]] LoadFlags loadFlags() const override;

    /// @beginGetters

    [[nodiscard]] u32 squashTimer() const;

    /// @endGetters

private:
    void calcSquash();

    owning_span<ObjectCarTGE *> m_cars; ///< Pointers to all car objects
    u32 m_squashTimer;                  ///< Frames since last squash

    static constexpr u32 SQUASH_MAX = 600; ///< Squash timer cap
};

} // namespace Kinoko::Field
