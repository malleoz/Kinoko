#pragma once

#include "game/field/StateManager.hh"
#include "game/field/obj/ObjectCollidable.hh"

namespace Kinoko {

namespace Host {

class Context;

} // namespace Host

namespace Field {

/// @brief Represents a single bat. It's owned and managed by @ref ObjectBasabasa.
class ObjectBasabasaDummy final : public ObjectCollidable, private StateManager {
public:
    ObjectBasabasaDummy(const System::MapdataGeoObj &params);
    ~ObjectBasabasaDummy() override;

    void init() override;
    void calc() override;
    [[nodiscard]] LoadFlags loadFlags() const override;
    [[nodiscard]] virtual const char *getKclName() const override;
    Kart::Reaction onCollision(Kart::KartObject *kartObj, Kart::Reaction reactionOnKart,
            Kart::Reaction reactionOnObj, EGG::Vector3f &hitDepth) override;

    /// @beginSetters

    void setActive(bool isSet);

    /// @endSetters

    /// @beginGetters

    [[nodiscard]] bool active() const;

    /// @endGetters

private:
    void calcStateActive();

    const bool m_bigBump;       ///< Affects the severity of the "push" when colliding with bat
    EGG::Vector3f m_initialPos; ///< RNG-based starting position for the bat
    bool m_active;              ///< Whether or not this bat is currently spawned

    /// @brief The enter and calc functions for each @ref StateManager entry
    static constexpr std::array<StateManagerEntry, 2> STATE_ENTRIES = {{
            {StateEntry<ObjectBasabasaDummy, nullptr, &ObjectBasabasaDummy::calcStateActive>(0)},
            {StateEntry<ObjectBasabasaDummy, nullptr, nullptr>(1)},
    }};
};

/// @brief Can be thought of as the bat "spawner". It's the class that manages an array of bats.
/// @details Spawns bats in small groups, whose size is determined by @ref m_batsPerGroup. After
/// that many bats have been spawned, it resets the @ref m_cycleTimer.
class ObjectBasabasa final : public ObjectCollidable {
    /// @brief Grants access to static member variables so a @ref Host::Context can restore their
    /// values on context switch
    friend Host::Context;

public:
    ObjectBasabasa(const System::MapdataGeoObj &params);
    ~ObjectBasabasa() override;

    void init() override;
    void calc() override;

    [[nodiscard]] LoadFlags loadFlags() const override;
    void createCollision() override;
    void loadRail() override;

    [[nodiscard]] static f32 InitialXRange();
    [[nodiscard]] static f32 InitialYRange();

private:
    owning_span<ObjectBasabasaDummy *> m_bats; ///< The array of individual bats
    const u32 m_initialTimer;                  ///< The m_cycleTimer starts and resets to this value
    const u32 m_batsPerGroup; ///< Number of bats that will spawn before resetting the m_cycleTimer
    const u32 m_startFrame;   ///< Initial delay before the spawner will start calculating
    const u32 m_batSpacing;   ///< How many frames in between bat spawns
    u32 m_cycleTimer;         ///< Used to determine when to spawn next bat
    u32 m_batsActive;         ///< The number of bats currently spawned

    /// @addr{0x809C2200}
    /// @brief Range of random X offsets for the bats' initial positions
    static f32 s_initialXRange;

    /// @addr{0x809C2204}
    /// @brief Range of random Y offsets for the bats' initial positions
    static f32 s_initialYRange;
};

} // namespace Field

} // namespace Kinoko
