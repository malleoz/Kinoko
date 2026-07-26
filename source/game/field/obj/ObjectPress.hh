#pragma once

#include "game/field/obj/ObjectCollidable.hh"

namespace Kinoko::Field {

/// @brief Represents the Toad's Factory stompers
/// @details The stompers are initially idle for m_initialRaisedDuration frames. After that, the
/// stompers cycle through a wind up, lowering, hitting the floor, and raising back up to idle. When
/// the stompers hit the floor, the duration of the lowered state is determined based on the CHR
/// animation's framecount. To figure out where the floor is, the init function performs a floor
/// collision check.
/// @note if the press is positioned such that there is no floor beneath it, then the init function
/// will be stuck in an infinite loop, causing the race to never start.
class ObjectPress : public ObjectCollidable {
public:
    ObjectPress(const System::MapdataGeoObj &params);
    ~ObjectPress() override;

    void init() override;
    void calc() override;

    /// @addr{0x807787E8}
    [[nodiscard]] u32 loadFlags() const override {
        return 1;
    }

    void loadAnims() override;
    void createCollision() override;

    [[nodiscard]] f32 getCollisionRadius() const override {
        return 700.0f;
    }

    Kart::Reaction onCollision(Kart::KartObject *kartObj, Kart::Reaction reactionOnKart,
            Kart::Reaction reactionOnObj, EGG::Vector3f &hitDepth) override;

    virtual void calcRaised();

protected:
    /// @brief Descirbes the current phase of the press's animation cycle
    enum class State {
        Raised,   ///< Idle, waiting to stomp down
        WindUp,   ///< Rising up a bit before stomping down
        Lowering, ///< Stomping down
        Lowered,  ///< Contacting the floor
        Raising,  ///< Rising back up to the raised position
    };

    State m_state;     ///< The current phase of the press's animation cycle
    u32 m_windUpTimer; ///< Number of frames remaining in windup state

private:
    /// @addr{0x8077786C}
    /// @brief Initializes timers for the press's lifecycle
    void initTimers() {
        m_anmTimer = 0;
        m_raisedTimer = m_initialRaisedDuration;
        m_windUpTimer = 0;
    }

    void calcWindUp();
    void calcLowering();
    void checkCollisionLowering();
    void calcLowered();
    void calcRaising();
    void enterRaising();

    bool m_startingRise;    ///< Used to delay state change by 1 frame
    u32 m_raisedTimer;      ///< Number of frames remaining in raised state
    u32 m_anmTimer;         ///< Remaining frames in the current press animation
    f32 m_loweringVelocity; ///< Speed at which the press is lowering
    f32 m_raisedHeight;     ///< Height of the press when it is in the raised state
    bool m_startedLowered;  ///< Used to induce crush effect even if it hit floor this frame

    const u32 m_initialRaisedDuration; ///< Initial delay before the first stomp of the race
    const u32 m_raisedDuration; ///< How long the press stays raised before stomping down again

    static constexpr f32 ANM_RATE = 2.0f; ///< Rate at which the press animation plays
};

/// @brief The stompers on the left and right side of the first TF factory room
/// @details These stompers don't stomp down until @ref ObjectItemboxPress tells them to.
class ObjectPressSenko final : public ObjectPress {
public:
    ObjectPressSenko(const System::MapdataGeoObj &params);
    ~ObjectPressSenko() override;

    /// @addr{0x8076EA28}
    [[nodiscard]] ObjectId id() const override {
        return ObjectId::Press;
    }

    /// @addr{0x8076EA20}
    [[nodiscard]] u32 loadFlags() const override {
        return 1;
    }

    /// @addr{0x8076EA30}
    [[nodiscard]] const char *getResources() const override {
        return "Press";
    }

    /// @addr{0x8076EA3C}
    [[nodiscard]] const char *getKclName() const override {
        return "Press";
    }

    void calcRaised() override;

    /// @brief Interface for @ref ObjectItemboxPress to tell the stomper to begin stomping
    void beginStomp() {
        m_startingWindup = true;
    }

private:
    void startWindup();

    bool m_startingWindup; ///< Signals that the press should begin stomping down on the next frame
};

} // namespace Kinoko::Field
