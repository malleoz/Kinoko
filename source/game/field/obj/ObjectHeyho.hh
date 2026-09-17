#pragma once

#include "game/field/StateManager.hh"
#include "game/field/obj/ObjectCollidable.hh"

namespace Kinoko::Field {

/// @brief Snowboarding Shy Guys on DK Summit
/// @details The Shy Guys move along a rail and jump over the zipper. When they go off a zipper, all
/// color variants perform a 180, except for red Shy Guys which perform a 720. The gravitational
/// acceleration is calculated based on the apex, the midpoint (lowest point), and the maximum
/// velocity param setting.
class ObjectHeyho final : public ObjectCollidable, private StateManager {
public:
    ObjectHeyho(const System::MapdataGeoObj &params);

    /// @addr{0x806CEB24}
    /// @brief Default virtual destructor
    ~ObjectHeyho() override = default;

    void init() override;

    /// @addr{0x806CEDF8}
    /// @copybrief ObjectBase::calc()
    /// @details Checks if the Shy Guy should transition to the next state. Updates the Shy Guy's
    /// position along the rail. Evaluates the Shy Guy's state machine. Finally, interpolates the
    /// Shy Guy's transform based off the current floor normal.
    void calc() override {
        calcStateTransition();
        calcMotion();
        StateManager::calc();
        calcInterp();
    }

    void loadAnims() override;
    void calcCollisionTransform() override;

    /// @addr{0x806D02B4}
    /// @copybrief ObjectBase::loadFlags()
    /// @return Returns @ref eLoadFlags::Calc and @ref eLoadFlags::Draw so that the object is
    /// calculated every frame
    /// @copybrief ObjectBase::loadFlags()
    /// @return Returns @ref eLoadFlags::Calc and @ref eLoadFlags::Draw so that the object is
    /// calculated every frame
    [[nodiscard]] LoadFlags loadFlags() const override {
        return LoadFlags().setBit(eLoadFlags::Calc, eLoadFlags::Draw);
    }

private:
    /// @brief Current animation of the Shy Guy
    enum class Animation {
        Move = 1,   ///< On the ground
        Jump = 2,   ///< In the air
        Jumped = 3, ///< Landed after a jump
    };

    /// @brief The color of the Shy Guy
    enum class Color {
        Red = 0,    ///< Red Shy Guy that performs a 720
        Yellow = 1, ///< Yellow Shy Guy that performs a 180
        Green = 2,  ///< Green Shy Guy that performs a 180
    };

    /// @brief Sets the specified animation (move or jump)
    /// @param anim The animation to set
    void changeAnimation(Animation anim) {
        m_drawMdl->anmMgr()->playAnim(0.0f, 1.0f, static_cast<size_t>(anim));
        m_currentAnim = anim;
    }

    /// @brief Runs when the Shy Guy enters the jump state
    /// @details Resets the spin frame counter for mid-air spinning
    void enterJump() {
        m_spinFrame = 0;
    }

    void calcMove();
    void calcJump();

    /// @addr{0x806CFD48}
    /// @brief Updates the @ref StateManager based off the current and next rail point settings
    /// @details If the current or next rail point's second setting is 0, then the Shy Guy
    /// transitions to the move state. Otherwise, it transitions to the jump state.
    /// @note Contrary to the other classes that inherit from @ref StateManager, this class directly
    /// sets @ref m_currentStateId rather than queuing the transition via @ref m_nextStateId.
    void calcStateTransition() {
        if (m_railInterpolator->curPoint().setting[1] == 0 ||
                m_railInterpolator->nextPoint().setting[1] == 0) {
            if (m_currentStateId != 0) {
                m_currentStateId = 0;
            }
        } else {
            if (m_currentStateId != 1) {
                m_currentStateId = 1;
            }
        }
    }

    void calcMotion();

    /// @addr{0x806CFFB0}
    /// @brief Interpolates the Shy Guy's orientation to reflect the current floor normal
    /// @details Interpolates the smoothed up vector, normalises the forward direction vector, and
    /// updates the transform matrix accordingly.
    void calcInterp() {
        m_up = Interpolate(0.2f, m_up, m_floorNrm);
        m_up.normalise2();
        m_forward.normalise2();
        setMatrixTangentTo(m_up, m_forward);
    }

    const Color m_color;      ///< Color of the Shy Guy, used to determine if it spins
    f32 m_apex;               ///< Highest Y position between the rail endpoints
    EGG::Vector3f m_midpoint; ///< Middle point (and lowest Y position) of the rai
    EGG::Vector3f m_initVel;  ///< Initial velocity vector
    f32 m_currentSpeed;         ///< Current speed along the rail
    f32 m_accel;              ///< Gravity based off apex, midpoint, and max velocity
    f32 m_maxVelSq;           ///< Square of the maximum velocity specified by param setting 1
    EGG::Vector3f m_up;       ///< Smoothed up vector
    EGG::Vector3f m_forward;  ///< Facing direction
    EGG::Vector3f m_floorNrm; ///< Floor normal from the most recent collision
    bool m_floorCollision;    ///< Whether the Shy Guy is currently colliding with the floor
    Animation m_currentAnim;  ///< Currently playing animation
    bool m_freeFall;          ///< Shy Guy has left the rail due to asymmetric endpoint heights
    f32 m_launchVel; ///< Speed at moment of rail direction change, used to snap back on landing
    s16 m_spinFrame; ///< Frame counter that ticks up while the Shy Guy is spinning mid-air

    /// @brief The enter and calc functions for each @ref StateManager entry
    static constexpr std::array<StateManagerEntry, 2> STATE_ENTRIES = {{
            {StateEntry<ObjectHeyho, nullptr, &ObjectHeyho::calcMove>(0)},
            {StateEntry<ObjectHeyho, &ObjectHeyho::enterJump, &ObjectHeyho::calcJump>(1)},
    }};

    /// @brief Height offset applied to the position passed into collision checks
    static constexpr EGG::Vector3f COLLISION_OFFSET = EGG::Vector3f(0.0f, 10.0f, 0.0f);

    static constexpr f32 COLLISION_RADIUS = 100.0f; ///< Radius of the collision sphere
};

} // namespace Kinoko::Field
