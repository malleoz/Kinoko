#pragma once

#include "game/field/StateManager.hh"
#include "game/field/obj/ObjectCollidable.hh"

namespace Field {

class ObjectBasabasaDummy;

template <>
class StateManager<ObjectBasabasaDummy> : public StateManagerBase<ObjectBasabasaDummy> {
public:
    StateManager(ObjectBasabasaDummy *obj);
    ~StateManager() override;

private:
    static const std::array<StateManagerEntry<ObjectBasabasaDummy>, 2> STATE_ENTRIES;
};

/// @brief Represents a single bat. It's owned and managed by @ref ObjectBasabasa.
class ObjectBasabasaDummy : public ObjectCollidable, public StateManager<ObjectBasabasaDummy> {
    friend StateManager<ObjectBasabasaDummy>;

public:
    ObjectBasabasaDummy(const System::MapdataGeoObj &params);
    ~ObjectBasabasaDummy() override;

    void init() override;
    void calc() override;

    /// @addr{0x806B7700}
    [[nodiscard]] u32 loadFlags() const override {
        return 3;
    }

    /// @addr{0x806B76F4}
    [[nodiscard]] virtual const char *getKclName() const override {
        return "basabasa";
    }

    Kart::Reaction onCollision(Kart::KartObject *kartObj, Kart::Reaction reactionOnKart,
            Kart::Reaction reactionOnObj, EGG::Vector3f &hitDepth) override;

    void setActive(bool isSet) {
        m_active = isSet;
    }

    [[nodiscard]] bool active() const {
        return m_active;
    }

private:
    void enterState0();
    void enterState1();
    void calcState0();
    void calcState1();

    EGG::Vector3f m_initialPos; // 0xe4
    bool m_active;              // 0xf0
    EGG::Vector3f m_curPos;     // 0xf4
};

/// @brief Can be thought of as the bat "spawner". It's the class that manages an array of bats.
class ObjectBasabasa : public ObjectCollidable {
public:
    ObjectBasabasa(const System::MapdataGeoObj &params);
    ~ObjectBasabasa() override;

    void init() override;
    void calc() override;

    /// @addr{0x806B7628}
    [[nodiscard]] u32 loadFlags() const override {
        return 3;
    }

    /// @addr{0x806B761C}
    void createCollision() override {}

    /// @addr{0x806B7620}
    void loadRail() override {}

private:
    std::span<ObjectBasabasaDummy *> m_bats; ///< The array of individual bats.
    u32 m_initialTimer;                      ///< The m_cycleTimer starts and resets to this value.
    u32 m_batsPerGroup;
    u32 m_startFrame;
    u32 m_batCount;
    u32 m_batDuration;
    u32 m_cycleTimer;
    u32 m_batsActive;
};

} // namespace Field
