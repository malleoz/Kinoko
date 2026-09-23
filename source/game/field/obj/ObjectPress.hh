#pragma once

#include "game/field/obj/ObjectCollidable.hh"

namespace Kinoko::Field {

/// @brief Represents the Toad's Factory stompers
/// @details The stompers are initially idle for m_initDelay frames. After that, the
/// stompers cycle through a wind up, lowering, hitting the floor, and raising back up to idle. When
/// the stompers hit the floor, the duration of the lowered state is determined based on the CHR
/// animation's framecount. To figure out where the floor is, the init function performs a floor
/// collision check.
/// @note If the press is positioned such that there is no floor beneath it, then @ref init() will
/// be stuck in an infinite loop, causing the race to never start.
class ObjectPress : public ObjectCollidable {
public:
    /// @addr{0x80777564}
    /// @copybrief ObjectCollidable::ObjectCollidable(const System::MapdataGeoObj &)
    /// @param params The parameters used to initialize the object
    /// @details Initializes @ref m_loweringSpeed to zero. Sets @ref m_initDelay based on param
    /// setting 2 and @ref m_raisedDuration based on param setting 3.
    ObjectPress(const System::MapdataGeoObj &params)
        : ObjectCollidable(params),
          m_loweringSpeed(0.0f),
          m_initDelay(static_cast<u32>(params.setting(1))),
          m_raisedDuration(static_cast<u32>(params.setting(2))) {}

    /// @addr{0x807775FC}
    /// @brief Default virtual destructor
    ~ObjectPress() override = default;

    void init() override;
    void calc() override;

    /// @addr{0x807787E8}
    /// @copybrief ObjectBase::loadFlags()
    /// @return Returns @ref eLoadFlags::Calc, so that object is calculated every frame.
    [[nodiscard]] LoadFlags loadFlags() const override {
        return LoadFlags(eLoadFlags::Calc);
    }

    /// @addr{0x807786E4}
    /// @copybrief ObjectBase::loadAnims()
    /// @details Loads the `Press` animation.
    void loadAnims() override {
        std::array<const char *, 1> names = {{
                "Press",
        }};

        std::array<Render::AnmType, 1> types = {{
                Render::AnmType::Chr,
        }};

        linkAnims(names, types);
    }

    void createCollision() override;

    /// @copybrief ObjectBase::getCollisionRadius()
    /// @return The collision radius of the press, `700.0f`.
    [[nodiscard]] f32 getCollisionRadius() const override {
        return 700.0f;
    }

    Kart::Reaction onCollision(Kart::KartObject *kartObj, Kart::Reaction reactionOnKart,
            Kart::Reaction reactionOnObj, EGG::Vector3f &hitDepth) override;

    /// @addr{0x80777A90}
    /// @brief Runs every frame that the press is in the raised state
    /// @details When the raised timer expires, the press will transition to the windup state where
    /// it winds up for 10 frames before stomping down.
    virtual void calcRaised() {
        constexpr u32 WINDUP_FRAMES = 10;

        if (--m_raisedTimer == 0) {
            m_state = State::WindUp;
            m_windUpTimer = WINDUP_FRAMES;
        }
    }

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
    /// @details Sets @ref m_anmTimer to 0, @ref m_raisedTimer to @ref m_initDelay, and @ref
    /// m_windUpTimer to 0.
    void initTimers() {
        m_anmTimer = 0;
        m_raisedTimer = m_initDelay;
        m_windUpTimer = 0;
    }

    /// @addr{0x80777B54}
    /// @brief Runs every frame that the press is winding up before stomping down
    /// @details Raises the stomper's position by `10.0f` every frame it is winding up. When the
    /// windup timer expires, the press will transition to the lowering state.
    void calcWindUp() {
        constexpr f32 SPEED = 10.0f;

        addPos(EGG::Vector3f(0.0f, SPEED, 0.0f));

        if (--m_windUpTimer == 0) {
            m_state = State::Lowering;
        }
    }

    /// @addr{0x80777B90}
    /// @brief Runs every frame that the press is stomping down
    /// @details The press accelerates downwards with constant acceleration of `3.0f` and calls @ref
    /// checkCollisionLowering() to check if it has hit the floor.
    void calcLowering() {
        constexpr f32 ACCEL = 3.0f;

        m_loweringSpeed -= ACCEL;
        addPos(EGG::Vector3f(0.0f, m_loweringSpeed, 0.0f));
        checkCollisionLowering();
    }

    void checkCollisionLowering();
    void calcLowered();
    void calcRaising();

    /// @addr{0x80778218}
    /// @brief Runs when the stomper begins raising upwards
    /// @details Plays the stomping animation is reverse, caching the animation duration to @ref
    /// m_anmTimer. Also sets the @ref m_startingRise flag.
    void enterRaising() {
        auto *anmMgr = m_drawMdl->anmMgr();
        f32 frameCount = anmMgr->activeAnim(Render::AnmType::Chr)->frameCount();
        anmMgr->playAnim(frameCount, -ANM_RATE, 0);
        m_startingRise = true;
        m_anmTimer = frameCount / ANM_RATE;
    }

    bool m_startingRise;   ///< Used to delay state change by 1 frame
    u32 m_raisedTimer;     ///< Number of frames remaining in raised state
    u32 m_anmTimer;        ///< Remaining frames in the current press animation
    f32 m_loweringSpeed;   ///< Speed at which the press is lowering
    f32 m_raisedHeight;    ///< Height of the press when it is in the raised state
    bool m_startedLowered; ///< Used to induce crush effect even if it hit floor this frame

    const u32 m_initDelay;      ///< Initial delay before the first stomp of the race
    const u32 m_raisedDuration; ///< How long the press stays raised before stomping down again

    static constexpr f32 ANM_RATE = 2.0f; ///< Rate at which the press animation plays
};

/// @brief The stompers on the left and right side of the first TF factory room
/// @details These stompers don't stomp down until a @ref ObjectItemboxBlock tells them to.
class ObjectPressSenko final : public ObjectPress {
public:
    /// @addr{0x8076E7AC}
    /// @copydoc ObjectPress::ObjectPress(const System::MapdataGeoObj &)
    /// @details Initializes the @ref m_shouldStartWindup flag to `false`.
    ObjectPressSenko(const System::MapdataGeoObj &params)
        : ObjectPress(params),
          m_shouldStartWindup(false) {}

    /// @addr{0x8076E818}
    /// @brief Default virtual destructor
    ~ObjectPressSenko() override = default;

    /// @addr{0x8076EA28}
    /// @copybrief ObjectBase::id()
    /// @return The ID of the press object, `ObjectId::Press`.
    [[nodiscard]] ObjectId id() const override {
        return ObjectId::Press;
    }

    /// @addr{0x8076EA20}
    /// @copybrief ObjectBase::loadFlags()
    /// @return Returns @ref eLoadFlags::Calc, so that object is calculated every frame.
    [[nodiscard]] LoadFlags loadFlags() const override {
        return LoadFlags(eLoadFlags::Calc);
    }

    /// @addr{0x8076EA30}
    /// @copybrief ObjectBase::getResources()
    /// @details Returns the resource name for the press.
    /// @return The resource name for the press (`Press`).
    [[nodiscard]] const char *getResources() const override {
        return "Press";
    }

    /// @addr{0x8076EA3C}
    /// @copybrief ObjectBase::getKclName()
    /// @return The model name of the press (`Press`).
    [[nodiscard]] const char *getKclName() const override {
        return "Press";
    }

    /// @addr{0x8076E870}
    /// @copybrief ObjectPress::calcRaised()
    /// @details If the press has been told to begin stomping, it will start winding up for the
    /// stomp by calling @ref startWindup() and resetting @ref m_shouldStartWindup to false.
    void calcRaised() override {
        if (m_shouldStartWindup) {
            startWindup();
            m_shouldStartWindup = false;
        }
    }

    /// @brief Interface for @ref ObjectItemboxBlock to tell the stomper to begin stomping
    /// @details Sets @ref m_shouldStartWindup to true.
    void beginStomp() {
        m_shouldStartWindup = true;
    }

private:
    /// @addr{0x8077808C}
    /// @brief Runs once when the press is in the raised state and has been told to begin stomping
    /// down
    /// @details Sets @ref m_state to @ref State::WindUp and initializes @ref m_windUpTimer to 10 so
    /// the press will wind up for 10 frames before stomping down.
    void startWindup() {
        constexpr u32 WINDUP_DURATION = 10;

        m_state = State::WindUp;
        m_windUpTimer = WINDUP_DURATION;
    }

    bool m_shouldStartWindup; ///< Signals that the press should begin stomping down next frame
};

} // namespace Kinoko::Field
