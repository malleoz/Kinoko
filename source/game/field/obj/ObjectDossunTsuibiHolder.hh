#pragma once

#include "game/field/obj/ObjectCollidable.hh"

#include <array>

namespace Field {

class ObjectDossunTsuibi;

class ObjectDossunTsuibiHolder final : public ObjectCollidable {
public:
    ObjectDossunTsuibiHolder(const System::MapdataGeoObj &params);
    ~ObjectDossunTsuibiHolder() override;

    void init() override;
    void calc() override;

    /// @addr{0x80764A1C}
    void calcModel() override {}

    /// @addr{0x80764A30}
    [[nodiscard]] u32 loadFlags() const override {
        return 1;
    }

    /// @addr{0x80764A2C}
    void loadGraphics() override {}

    /// @addr{0x80764A24}
    void createCollision() override {}

    void startStill() {
        m_state = State::Backward;
        m_railInterpolator->setCurrVel(m_vel);
    }

private:
    void FUN_807624F0();
    void FUN_807625F0();
    void FUN_8076321C();
    void FUN_807634C0();

    enum class State {
        Still = 0,
        Forward = 1,    ///< Moving forward down the hallway
        StartStomp = 2, ///< One frame in which the holder activates the Thwomp stomp
        Stomping = 3,
        Backward = 4,     ///< Moving backwards up the hallway
        SillRotating = 5, ///< Returned back to home, but rotating to face hallway
    };

    std::array<ObjectDossunTsuibi *, 2> m_dossuns;
    State m_state;
    EGG::Vector3f m_initPos;
    u32 m_stillTimer;
    bool m_movingForward;
    u32 m_forwardTimer;
    bool m_movingSideways; ///< Whether the Thwomps should move along the z-axis
    u32 m_flipSideways;    ///< Inverts which direction the thwomps move along the z-axis
    u32 m_sidewaysPhase;
    bool m_facingBackwards;
    f32 m_lastStompZ; ///< Z-position of the last stomp
    f32 m_resetZVel;  ///< Speed required to move from m_lastStompZ to the home's Z in 36 frames
    f32 m_vel;
    f32 m_initRotY;
    f32 m_resetAngVel;      ///< Rotational speed of Thwomps after returning home
    u32 m_backwardsCounter; ///< Ticks up every frame the Thwomps are facing backwards, or are
                            ///< resetting after returning home.

    /// How long Thwomps take to reset after returning to home
    static constexpr u32 HOME_RESET_FRAMES = 36;
    static constexpr f32 DOSSUN_POS_OFFSET = 500.0f;
};

} // namespace Field
