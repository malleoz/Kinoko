#pragma once

#include "game/field/obj/ObjectCollidable.hh"

namespace Kinoko::Field {

/// @brief The traffic cones on Daisy Circuit
/// @desync Pylons are restricted to a 1200 unit radius around their initial position. If the
/// player hits a pylon at 70% of the kart's max speed, then the pylon will fly off. Otherwise, the
/// pylon will apply a slowing effect on the player if the player is colliding at a direct enough
/// angle. Performs collision checks against floors, walls, and other pylons to prevent clipping.
/// When a pylon starts flying off, it will bounce 4 times before shrinking and becoming intangible.
/// After a cooldown, it will spawn mid-air and fall down to the ground.
class ObjectPylon final : public ObjectCollidable {
public:
    ObjectPylon(const System::MapdataGeoObj &params);
    ~ObjectPylon() override;

    void init() override;
    void calc() override;

    /// @addr{0x8082E4F8}
    [[nodiscard]] u32 loadFlags() const override {
        return 1;
    }

    Kart::Reaction onCollision(Kart::KartObject *kartObj, Kart::Reaction reactionOnKart,
            Kart::Reaction reactionOnObj, EGG::Vector3f &hitDepth) override;

private:
    /// @brief Describes the current state of the pylon's motion and tangibility
    enum State {
        Idle = 0,     ///< Not moving
        Moving = 1,   ///< Cone is being pushed by the player
        Hide = 2,     ///< Cone is intangible
        ComeBack = 3, ///< Cone is respawning, falling from above
        Hiding = 4,   ///< Cone is shrinking before becoming intangible
        Hit = 5,      ///< Cone is flying and bouncing after being hit
    };

    void checkIntraCollision(const EGG::Vector3f &hitDepth);

    void startHit(f32 velFactor, EGG::Vector3f &hitDepth);

    void calcHit();
    void calcHiding();
    void calcHide();
    void calcComeBack();

    State m_state;                            ///< Current motion and tangibility state
    std::array<ObjectPylon *, 2> m_neighbors; ///< Pointers to the two closest neighboring pylons
    const EGG::Vector3f m_initPos;            ///< Initial position of the cone
    const EGG::Vector3f m_initScale;          ///< Initial scale of the cone
    const EGG::Vector3f m_initRot;            ///< Initial rotation of the cone
    u32 m_stateStartFrame;                    ///< Frame when pylon entered the current m_state
    u32 m_numBounces;       ///< Number of floor and wall collisions while in Hit state
    EGG::Vector3f m_vel;    ///< Velocity used in Hit and ComeBack states
    EGG::Vector3f m_angVel; ///< Added to rotation while in Hit state

    static constexpr f32 RADIUS = 120.0f;  ///< Radius of the pylon's collision sphere
    static constexpr f32 FALL_VEL = 20.0f; ///< Speed at which the pylon falls from mid-air

    /// @brief Minimum frames before a state change can occur
    static constexpr u32 STATE_COOLDOWN_FRAMES = 5;
};

} // namespace Kinoko::Field
