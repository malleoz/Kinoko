#pragma once

#include "game/field/StateManager.hh"
#include "game/field/obj/ObjectCollidable.hh"

namespace Kinoko::Field {

/// @brief Represents a single bat. It's owned and managed by @ref ObjectBasabasa.
class ObjectBasabasaDummy final : public ObjectCollidable, private StateManager {
public:
    /// @addr{0x806B5C84}
    /// @brief Constructor
    /// @param params The parameters used to initialize the object
    ObjectBasabasaDummy(const System::MapdataGeoObj &params)
        : ObjectCollidable(params), StateManager(this, STATE_ENTRIES),
          m_bigBump(params.setting(7) == 1) {
        m_active = true;
    }

    /// @addr{0x806B7630}
    /// @brief Default virtual destructor
    ~ObjectBasabasaDummy() override = default;

    void init() override;

    /// @addr{0x806B602C}
    /// @copybrief ObjectBase::calc()
    /// @details Per-frame calculating are managed entirely by the @ref StateManager
    void calc() override {
        StateManager::calc();
    }

    /// @addr{0x806B7700}
    /// @copybrief ObjectBase::loadFlags()
    /// @return Returns @ref eLoadFlags::Calc and @ref eLoadFlags::Draw so that the object is
    /// calculated every frame
    [[nodiscard]] LoadFlags loadFlags() const override {
        return LoadFlags().setBit(eLoadFlags::Calc, eLoadFlags::Draw);
    }

    /// @addr{0x806B76F4}
    /// @copybrief ObjectBase::getKclName()
    /// @return Retuns "basabasa" so that the individual bat's collision is used
    [[nodiscard]] virtual const char *getKclName() const override {
        return "basabasa";
    }

    Kart::Reaction onCollision(Kart::KartObject *kartObj, Kart::Reaction reactionOnKart,
            Kart::Reaction reactionOnObj, EGG::Vector3f &hitDepth) override;

    /// @brief Sets whether the bat is spawned
    /// @param isSet Whether the bat should be active or not
    void setActive(bool isSet) {
        m_active = isSet;
    }

    /// @brief Checks whether the bat is currently spawned
    /// @return `true` if the bat is active, `false` otherwise
    [[nodiscard]] bool active() const {
        return m_active;
    }

private:
    /// @brief No-op when the bat transitions between states
    void enterStateStub() {}

    void calcStateActive();

    /// @addr{0x806B652C}
    /// @brief No-op when the bat is inactive (despawned)
    void calcStateStub() {}

    const bool m_bigBump;       ///< Affects the severity of the "push" when colliding with bat
    EGG::Vector3f m_initialPos; ///< RNG-based starting position for the bat
    bool m_active;              ///< Whether or not this bat is currently spawned

    /// @brief The enter and calc functions for each @ref StateManager entry
    static constexpr std::array<StateManagerEntry, 2> STATE_ENTRIES = {{
            {StateEntry<ObjectBasabasaDummy, &ObjectBasabasaDummy::enterStateStub,
                    &ObjectBasabasaDummy::calcStateActive>(0)},
            {StateEntry<ObjectBasabasaDummy, &ObjectBasabasaDummy::enterStateStub,
                    &ObjectBasabasaDummy::calcStateStub>(1)},
    }};
};

/// @brief Can be thought of as the bat "spawner". It's the class that manages an array of bats.
/// @details Spawns bats in small groups, whose size is determined by @ref m_batsPerGroup. After
/// that many bats have been spawned, it resets the @ref m_cycleTimer.
class ObjectBasabasa final : public ObjectCollidable {
    /// @brief Grants access to the singleton so a @ref Host::Context can restore the instance's
    /// state on context switch
    friend class Host::Context;

public:
    ObjectBasabasa(const System::MapdataGeoObj &params);
    ~ObjectBasabasa() override;

    void init() override;
    void calc() override;

    /// @addr{0x806B7628}
    /// @copybrief ObjectBase::loadFlags()
    /// @return Returns @ref eLoadFlags::Calc and @ref eLoadFlags::Draw so that the object is
    /// calculated every frame
    [[nodiscard]] LoadFlags loadFlags() const override {
        return LoadFlags().setBit(eLoadFlags::Calc, eLoadFlags::Draw);
    }

    /// @addr{0x806B761C}
    /// @copybrief ObjectBase::createCollision()
    /// @details No-op since the spawner itself does not have any collision
    void createCollision() override {}

    /// @addr{0x806B7620}
    /// @copybrief ObjectBase::loadRail()
    /// @details No-op since the spawner itself does not have any rail to load
    void loadRail() override {}

    /// @brief Exposes the range of initial X positions so the dummy can access it
    /// @return The range of initial X positions for the bats
    [[nodiscard]] static f32 initialXRange() {
        return s_initialXRange;
    }

    /// @brief Exposes the range of initial Y positions so the dummy can access it
    /// @return The range of initial Y positions for the bats
    [[nodiscard]] static f32 initialYRange() {
        return s_initialYRange;
    }

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

} // namespace Kinoko::Field
