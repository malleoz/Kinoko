#pragma once

#include "game/field/StateManager.hh"
#include "game/field/obj/ObjectCollidable.hh"

#include "game/system/RaceManager.hh"

namespace Kinoko::Field {

/// @brief Lava eruptions without any hump (@ref ObjectFlamePoleFoot) like on N64 Bowser's Castle
class ObjectFlamePoleV final : public ObjectCollidable, private StateManager {
public:
    /// @addr{0x806C3AA4}
    /// @copybrief ObjectCollidable::ObjectCollidable(const System::MapdataGeoObj &)
    /// @param params The parameters used to initialize the object
    /// @details Computes @ref m_initDelay based on param setting 2, @ref m_cycleDuration based on
    /// param setting 1, and @ref m_dormantFrames as `200` plus param setting 4. Sets @ref
    /// m_scaleFactor based on param setting 3, defaulting to `5.0f` if the setting is `0`. Caches
    /// the initial Y-position of the object to @ref m_initPosY. Caches whether the object
    /// represents the larger flame poles at the end of N64 Bowser's Castle to @ref m_isBig. In
    /// Kinoko, we take creative liberty to define and initialize @ref m_eruptedDuration to avoid
    /// frequent recomputation in @ref calcErupted(). Finally, computes @ref m_maxOffsetY by
    /// multiplying the base height of `384.0f` with @ref m_scaleFactor and (if @ref m_isBig is
    /// `true`) an additional `14.0f` multiplier.
    ObjectFlamePoleV(const System::MapdataGeoObj &params)
        : ObjectCollidable(params),
          StateManager(this, STATE_ENTRIES),
          m_initDelay(static_cast<u32>(params.setting(1))),
          m_cycleDuration(static_cast<s32>(params.setting(0))),
          m_dormantFrames(static_cast<s32>(params.setting(3)) + 200),
          m_scaleFactor(params.setting(2) == 0 ? 5.0f : static_cast<f32>(params.setting(2))),
          m_initPosY(pos().y),
          m_isBig(strcmp(getName(), "FlamePole_v_big") == 0),
          m_eruptedDuration(
                  static_cast<u32>(static_cast<f32>(m_cycleDuration) - BEFORE_ERUPT_FRAMES -
                          ERUPT_FRAMES - FALL_FRAMES - static_cast<f32>(m_dormantFrames))) {
        constexpr f32 BASE_HEIGHT = 384.0f;
        constexpr f32 BIG_MULTIPLIER = 14.0f;

        if (m_isBig) {
            m_maxOffsetY = BIG_MULTIPLIER * BASE_HEIGHT * m_scaleFactor;
        } else {
            m_maxOffsetY = BASE_HEIGHT * m_scaleFactor;
        }
    }

    /// @addr{0x806C47B4}
    /// @brief Default virtual destructor
    ~ObjectFlamePoleV() = default;

    /// @addr{0x806C3E90}
    /// @copybrief ObjectBase::init()
    /// @details Initializes the object to the before erupting state, however this state won't be
    /// evaluated right away since @ref calc() early returns until @ref m_initDelay frames have
    /// elapsed in the race. Disables collision for the flame pole initially. Sets the object's
    /// scale based off whether it is a big flame pole. Also resizes the collision radius based on
    /// the scale. Finally, resets @ref m_currOffsetY to zero and computes the flame pole's @ref
    /// m_fallSpeed so that it takes the pole @ref FALL_FRAMES frames to fall. In Kinoko, we also
    /// take creative liberity to initialize @ref m_initEruptingVel and @ref m_eruptingAccel here
    /// rather than recompute it every time @ref enterErupting() is called.
    void init() override {
        constexpr EGG::Vector3f BIG_SCALE = EGG::Vector3f(13.0f, 14.0f, 13.0f);
        constexpr f32 RADIUS = 70.0f;

        m_nextStateId = 0;

        disableCollision();

        if (m_isBig) {
            setScale(BIG_SCALE);
        } else if (0.0f != m_scaleFactor) {
            setScale(m_scaleFactor);
        }

        resize(RADIUS * scale().y, 0.0f);

        m_currOffsetY = 0.0f;
        m_fallSpeed = m_maxOffsetY / FALL_FRAMES;

        InitEruptionKinematics();
    }

    /// @addr{0x806C3FCC}
    /// @copybrief ObjectBase::calc()
    /// @details Early returns if the race timer has not yet exceeded @ref m_initDelay. Otherwise,
    /// evaluates the flame pole's state machine. Finally, sets the flame pole's Y-position to
    /// reflect the current height offset.
    void calc() override {
        if (System::RaceManager::Instance()->timer() <= m_initDelay) {
            return;
        }

        StateManager::calc();

        f32 posY = m_currOffsetY + (m_initPosY - m_maxOffsetY);
        setPos(EGG::Vector3f(pos().x, posY, pos().z));
    }

    /// @addr{0x806C4898}
    /// @copybrief ObjectBase::loadFlags()
    /// @return Returns @ref eLoadFlags::Calc, so that object is calculated every frame.
    [[nodiscard]] LoadFlags loadFlags() const override {
        return LoadFlags(eLoadFlags::Calc);
    }

    /// @addr{0x806C488C}
    /// @copybrief ObjectBase::getResources()
    /// @details Returns the resource name for the vertical flame pole.
    /// @return The resource name for the vertical flame pole (`FlamePole_v`).
    [[nodiscard]] const char *getResources() const override {
        return "FlamePole_v";
    }

    /// @addr{0x806C4880}
    /// @copybrief ObjectBase::getKclName()
    /// @return The model name of the vertical flame pole (`FlamePoleEff`)
    [[nodiscard]] const char *getKclName() const override {
        return "FlamePoleEff";
    }

private:
    /// @addr{0x806C40E4}
    /// @brief Runs once when the geyser is leaving dormancy
    /// @details Re-enables collision for the flame pole.
    void enterBeforeErupting() {
        enableCollision();
    }

    /// @addr{0x806C4178}
    /// @brief Runs once when the geyser begins erupting upwards
    /// @details Resets the current height offset to zero.
    /// @note In the base game, this function recomputes @ref m_initEruptingVel and @ref
    /// m_eruptionDecel, but since these values can be computed at the moment of object
    /// initialization, we omit this recomputation here.
    void enterErupting() {
        m_currOffsetY = 0.0f;
    }

    /// @addr{0x806C43BC}
    /// @brief Runs once when the geyser begins to descend
    /// @details Sets @ref m_loweringStartOffsetY to the current height offset.
    /// @note In the base game, this function also recomputes @ref m_fallSpeed as `@ref m_maxOffsetY
    /// / @ref FALL_FRAMES`. Because this is already computed in @ref init() and @ref m_maxOffsetY
    /// and @ref m_fallSpeed do not change at any point, we omit this recomputation in Kinoko.
    void enterLowering() {
        m_loweringStartOffsetY = m_currOffsetY;
    }

    /// @addr{0x806C4478}
    /// @brief Runs once when the geyser enters dormancy
    /// @details Disables collision for the flame pole.
    void enterDormant() {
        disableCollision();
    }

    /// @addr{0x806C4130}
    /// @brief Runs every frame that the flame pole is in the pre-eruption state
    /// @details This is effectively a no-op that runs for @ref BEFORE_ERUPT_FRAMES frames.
    /// Afterwards, transitions the flame pole to the erupting state.
    void calcBeforeErupting() {
        if (static_cast<f32>(m_currentFrame) >= BEFORE_ERUPT_FRAMES) {
            m_nextStateId = 1;
        }
    }

    /// @addr{0x806C41F0}
    /// @brief Runs every frame that the geyser is erupting
    /// @details If @ref ERUPT_FRAMES frames have elapsed in the erupting state, transitions to the
    /// erupted state. Otherwise, updates the current height offset based on the kinematic
    /// parameters @ref m_initEruptingVel and @ref m_eruptionDecel.
    void calcErupting() {
        if (static_cast<f32>(m_currentFrame) >= ERUPT_FRAMES) {
            m_nextStateId = 2;
        }

        m_currOffsetY =
                CalcParabolicDisplacement(m_initEruptingVel, m_eruptionDecel, m_currentFrame);
    }

    /// @addr{0x806C42A0}
    /// @brief Runs every frame after the geyser has erupted and before ther geyser descends to
    /// dormancy
    /// @details The flame pole's height offset oscillates around @ref m_maxOffsetY with an
    /// amplitude of `50.0f` and a period of 30 frames. If @ref m_eruptedDuration frames have
    /// elapsed in this state, the flame pole transitions to the lowering state.
    /// @note It's intended that @ref m_dormantFrames is less than @ref m_cycleDuration. If @ref
    /// m_dormantFrames is greater, then this will result in the geyser skipping its bobbing
    /// animation after eruption and instead it will immediately descend.
    void calcErupted() {
        constexpr f32 AMPLITUDE = BEFORE_ERUPT_FRAMES;
        constexpr f32 PERIOD = 30.0f;

        if (m_currentFrame >= m_eruptedDuration) {
            m_nextStateId = 3;
        }

        f32 sin = EGG::Mathf::SinFIdx(
                DEG2FIDX * (360.0f * static_cast<f32>(m_currentFrame) / PERIOD));
        m_currOffsetY = m_maxOffsetY + AMPLITUDE * sin;
    }

    /// @addr{0x806C43E8}
    /// @brief Runs every frame the geyser is lowering to dormancy
    /// @details Linearly decreases the flame pole's height offset. If the flame pole reaches a
    /// height offset of `0.0f` (or `-300.0f` when @ref m_isBig is `true`), then the flame pole
    /// transitions to the dormant state.
    void calcLowering() {
        if (m_isBig) {
            if (m_currOffsetY <= -300.0f) {
                m_nextStateId = 4;
            }
        } else {
            if (m_currOffsetY <= 0.0f) {
                m_nextStateId = 4;
            }
        }
        m_currOffsetY = m_loweringStartOffsetY - m_fallSpeed * static_cast<f32>(m_currentFrame);
    }

    /// @addr{0x806C44C8}
    /// @brief Runs every frame that the geyser lays dormant
    /// @details This is effectively a no-op that runs for @ref m_dormantFrames frames. After this
    /// period, the flame pole transitions back to the before erupting state.
    void calcDormant() {
        if (m_currentFrame >= static_cast<u32>(m_dormantFrames)) {
            m_nextStateId = 0;
        }
    }

    /// @addr{0x806B5A0C}
    /// @brief Computes @ref m_initEruptingVel and @ref m_eruptionDecel such that it takes @ref
    /// ERUPT_FRAMES frames to reach an eruption of @ref m_maxOffsetY height
    /// @details Derived from the kinematic equations \f$y(t) = v_0 t - \frac12 a t^2\f$ and
    ///          \f$v(t) = v_0 - a t\f$. At the peak, \f$v(t) = 0\f$, so \f$a = v_0 / t\f$,
    ///          which reduces the displacement equation to \f$y(t) = \frac12 v_0 t\f$.
    ///          Solving for \f$v_0\f$ with \f$y(t) = \text{maxHeight}\f$ gives
    ///          \f$v_0 = 2 \cdot \text{maxHeight} / t\f$, and substituting back into
    ///          \f$a = v_0 / t\f$ gives \f$a = v_0^2 / (2 \cdot \text{maxHeight})\f$.
    void InitEruptionKinematics() {
        f32 doublePeak = 2.0f * m_maxOffsetY;
        m_initEruptingVel = doublePeak / ERUPT_FRAMES;
        m_eruptionDecel = m_initEruptingVel * m_initEruptingVel / doublePeak;
    }

    const u32 m_initDelay;     ///< Frames before state lifecycle begins
    const s32 m_cycleDuration; ///< Duration of a full loop of the geyser's state lifecycle
    const s32 m_dormantFrames; ///< Number of frames within a cycle the geyser remains dormant for
    const f32 m_scaleFactor;   ///< Scale multiplier
    const f32 m_initPosY;      ///< Initial y-axis position
    f32 m_maxOffsetY;          ///< Max height offset of the pole
    f32 m_eruptionDecel;       ///< Effectively the gravity coefficient when erupting
    f32 m_initEruptingVel;     ///< Velocity of the geyser when it begins erupting upwards
    f32 m_fallSpeed;           ///< Rate at which the geyser lowers
    const bool m_isBig; ///< Discerns between FlamePole_v and FlamePole_v_big to alter size handling
    f32 m_currOffsetY;  ///< Height offset based off eruption velocity
    f32 m_loweringStartOffsetY; ///< Height offset when the geyser begins lowering to dormancy

    /// @brief Duration of the erupted state, cached in Kinoko to avoid recomputation in @ref
    /// calcErupted()
    /// @details Rathern than populating this duration directly from a param setting, the base game
    /// instead derives the erupted duration by "filling in the gap" between all other state
    /// durations and the total @ref m_cycleDuration.
    const u32 m_eruptedDuration;

    static constexpr f32 ERUPT_FRAMES = 60.0f;        ///< Frames it takes to raise to max height
    static constexpr f32 BEFORE_ERUPT_FRAMES = 50.0f; ///< Delay before pole starts erupting
    static constexpr f32 FALL_FRAMES = 180.0f;        ///< Frames it takes to lower into the ground

    /// @brief The enter and calc functions for each @ref StateManager entry
    static constexpr std::array<StateManagerEntry, 5> STATE_ENTRIES = {{
            {StateEntry<ObjectFlamePoleV, &ObjectFlamePoleV::enterBeforeErupting,
                    &ObjectFlamePoleV::calcBeforeErupting>(0)},
            {StateEntry<ObjectFlamePoleV, &ObjectFlamePoleV::enterErupting,
                    &ObjectFlamePoleV::calcErupting>(1)},
            {StateEntry<ObjectFlamePoleV, nullptr, &ObjectFlamePoleV::calcErupted>(2)},
            {StateEntry<ObjectFlamePoleV, &ObjectFlamePoleV::enterLowering,
                    &ObjectFlamePoleV::calcLowering>(3)},
            {StateEntry<ObjectFlamePoleV, &ObjectFlamePoleV::enterDormant,
                    &ObjectFlamePoleV::calcDormant>(4)},
    }};
};

} // namespace Kinoko::Field
