#pragma once

#include "game/field/obj/ObjectCollidable.hh"

namespace Kinoko::Field {

/// @brief Represents the piranhas on GCN Mario Circuit.
/// @details These piranhas often exist above a pipe, but these piranha objects are separate from
/// the pipe object, which is a primitive @ref ObjectCollidable represented with @ref
/// ObjectId::PakkunDokan. Piranhas cycle between a waiting state and an attacking state. When they
/// attack, their collision transformation matrix is updated to reflect the piranha chomping forward
/// and downward.
class ObjectPakkunF final : public ObjectCollidable {
public:
    /// @addr{0x807743A4}
    /// @copybrief ObjectCollidable::ObjectCollidable(const System::MapdataGeoObj &)
    /// @param params The parameters used to initialize the object
    /// @details Initializes @ref m_attackFrames and @ref m_currAttackFrames to zero. Sets @ref
    /// m_waitDuration based on param setting 1.
    /// @note @ref m_waitDuration is normally set in @ref init(), but since it is conditionally set
    /// based on whether we are in time trial mode, we can move it to the constructor for Kinoko.
    /// This also allows us to mark the member as `const`. As for @ref m_attackFrames and @ref
    /// m_currAttackFrame, these are not initialized in the constructor in the base game; rather,
    /// they are zero'd out as a result of the fact that heap memory is zero'd on console.
    ObjectPakkunF(const System::MapdataGeoObj &params)
        : ObjectCollidable(params),
          m_attackFrames(0),
          m_currAttackFrame(0),
          m_waitDuration(static_cast<s32>(params.setting(0))) {}

    /// @addr{0x807754FC}
    /// @brief Default virtual destructor
    ~ObjectPakkunF() override = default;

    /// @addr{0x807743E4}
    /// @copybrief ObjectBase::init()
    /// @details Initializes @ref m_state to @ref State::Wait and sets @ref m_waitFrames to @ref
    /// m_waitDuration. Finally, updates the piranha's transformation matrix.
    /// @note  @ref m_waitDuration is normally initialized in this function, but since it is
    /// conditionally set based on whether we are in time trial mode, we can move it to the
    /// constructor for Kinoko. This also allows us to mark the member as `const`.
    void init() override {
        m_state = State::Wait;
        m_waitFrames = m_waitDuration;

        calcTransform();
    }

    /// @addr{0x80774754}
    /// @copybrief ObjectBase::calc()
    /// @details If the piranha is idle, calls @ref calcWait(). If the piranha is attacking, calls
    /// @ref calcAttack(). Finally, updates the piranha's collision manager's transformation matrix
    /// to reflect any change in state.
    void calc() override {
        switch (m_state) {
        case State::Wait:
            calcWait();
            break;
        case State::Attack:
            calcAttack();
            break;
        default:
            break;
        }

        calcCollisionTransform();
    }

    /// @addr{0x807754F4}
    /// @copybrief ObjectBase::loadFlags()
    /// @return Returns @ref eLoadFlags::Calc, so that object is calculated every frame.
    [[nodiscard]] LoadFlags loadFlags() const override {
        return LoadFlags(eLoadFlags::Calc);
    }

    /// @addr{0x80775464}
    /// @copybrief ObjectBase::loadAnims()
    /// @details Loads the `attack`, `damage`, and `wait` animations for the piranha.
    void loadAnims() override {
        std::array<const char *, 3> names = {{
                "attack",
                "damage",
                "wait",
        }};

        std::array<Render::AnmType, 3> types = {{
                Render::AnmType::Chr,
                Render::AnmType::Chr,
                Render::AnmType::Chr,
        }};

        linkAnims(names, types);
    }

    void calcCollisionTransform() override;

    /// @addr{0x80775458}
    /// @copybrief ObjectBase::getCollisionRadius()
    /// @return The collision radius of the piranha, `1000.0f`.
    /// @details Extends the collision radius to ensure GJK collision checks occur from far enough
    /// away when the piranha chomps forward.
    [[nodiscard]] f32 getCollisionRadius() const override {
        return 1000.0f;
    }

private:
    /// @brief Distinguishes between whether the piranha is idle or chomping
    enum class State {
        Wait = 0,   ///< The piranha is idle
        Attack = 1, ///< The piranha is chomping
    };

    /// @addr{0x80774A00}
    /// @brief Runs every frame when the piranha is idle
    /// @details When the the wait timer reaches 0, the piranha will enter the attack state.
    void calcWait() {
        if (--m_waitFrames == 0) {
            enterAttack();
        }
    }

    /// @addr{0x80774A84}
    /// @brief Runs every frame while the piranha is attacking
    /// @details Increments @ref m_currAttackFrame and calls @ref calcAttackTimer() to see if the
    /// piranha should stop attacking.
    void calcAttack() {
        ++m_currAttackFrame;
        calcAttackTimer();
    }

    /// @addr{0x80774CB0}
    /// @brief Runs once when the piranha stops idling and is about to start attacking
    /// @details Sets the piranha's state to @ref State::Attack. Resets @ref m_currAttackFrame to
    /// zero, and sets @ref m_attackFrames to the animation duration plus a `60` frame lingering
    /// period.
    void enterAttack() {
        constexpr s32 LINGERING_FRAMES = 60; // Additional frames before switching back to idle

        m_state = State::Attack;
        auto *anmMgr = m_drawMdl->anmMgr();
        anmMgr->playAnim(0.0f, 1.0f, 0);
        m_currAttackFrame = 0;
        auto *attackAnm = anmMgr->activeAnim(Render::AnmType::Chr);
        m_attackFrames = static_cast<s32>(attackAnm->frameCount()) + LINGERING_FRAMES;
    }

    /// @addr{0x80774C1C}
    /// @brief Checks if the piranha should stop attacking and transition back to the wait state
    /// @details Decrements @ref m_attackFrames. If it reaches zero, calls @ref enterWait() to
    /// transition the piranha back to the wait state.
    void calcAttackTimer() {
        if (--m_attackFrames == 0) {
            enterWait();
        }
    }

    /// @addr{0x80774C80}
    /// @brief Transitions the piranha from the attack state back to the wait state
    /// @details Sets the piranha's state to @ref State::Wait and resets @ref m_waitFrames to
    /// @ref m_waitDuration.
    void enterWait() {
        m_state = State::Wait;
        m_waitFrames = m_waitDuration;
    }

    State m_state;         ///< Tracks whether the piranha is idle or chomping
    s32 m_waitFrames;      ///< How long until the piranha starts chomping
    s32 m_attackFrames;    ///< How long until the piranha stops chomping
    u32 m_currAttackFrame; /// How long the piranhas has been attacking for

    /// @brief Total time the piranha will be idle for
    /// @details Not in the base game, but caching this prevents having to dereference m_mapObj
    /// multiple times.
    const s32 m_waitDuration;
};

} // namespace Kinoko::Field
