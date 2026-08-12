#pragma once

#include "game/field/jugem/JugemMove.hh"
#include "game/field/jugem/JugemSwitch.hh"

#include "game/field/StateManager.hh"

#include "game/kart/KartObject.hh"

namespace Kinoko::Field {

/// @brief Manages interpolation keyframes for the Lakitu's position when ascending and descending
class JugemInterp {
public:
    /// @addr{Inlined in 0x80721514}
    /// @brief Initializes an array of keyframes
    /// @param count The number of keyframes
    JugemInterp(u32 count) {
        m_keyframes = owning_span<InterpKeyframe>(count);
        m_time = 0.0f;
        m_rate = 0.0f;
    }

    /// @addr{0x80721E64}
    ~JugemInterp() = default;

    /// @addr{0x8072370C}
    /// @brief Initializes the keyframes for the interpolation sequence
    /// @param startRate The interpolation rate for the first keyframe
    /// @param endRate The interpolation rate for the last keyframe
    /// @param tDelta The time delta between the first and last keyframe
    void initKeyframes(f32 startRate, f32 endRate, f32 tDelta, u32 param4) {
        m_numKeyframes = 0;

        setStartKeyframe(startRate);
        addKeyframe(endRate, tDelta, param4);

        m_rate = startRate;
        m_time = 0.0f;
    }

    /// @addr{0x80723AF8}
    /// @brief Advances the interpolation sequenceand calculates the current interpolation rate
    /// @param tDelta The time delta to advance the interpolation sequence
    /// @return Whether the interpolation sequence has reached the end of the last keyframe
    bool calc(f32 tDelta) {
        m_rate = calcInterpRate(m_time);
        m_time += tDelta;

        bool endOfKeyframe = m_time > m_keyframes[1].m_time;
        if (endOfKeyframe) {
            m_time = m_keyframes[1].m_time;
        }

        return endOfKeyframe;
    }

    /// @addr{0x8074C048}
    /// @brief Adds a keyframe at time 0 with the specified interpolation rate
    /// @param rate The interpolation rate for the keyframe
    void setStartKeyframe(f32 rate) {
        m_keyframes[m_numKeyframes].m_interpRate = rate;
        m_keyframes[m_numKeyframes].m_time = 0.0f;
        ++m_numKeyframes;
    }

    /// @addr{0x8074C0B4}
    /// @brief Adds a keyframe with the specified interpolation rate and time delta
    /// @param rate The interpolation rate for the keyframe
    /// @param tDelta The time delta from the previous keyframe to this keyframe
    void addKeyframe(f32 rate, f32 tDelta, u32 /*param4*/) {
        m_keyframes[m_numKeyframes].m_interpRate = rate;
        m_keyframes[m_numKeyframes].m_time = tDelta + m_keyframes[m_numKeyframes - 1].m_time;
        ++m_numKeyframes;
    }

    /// @addr{0x8074C1E0}
    /// @brief Calculates the current interpolation rate based on the current time and keyframes
    /// @param time The current frame in the interpolation sequence
    [[nodiscard]] f32 calcInterpRate(f32 time) const {
        const InterpKeyframe *currKeyframe = nullptr;
        const InterpKeyframe *nextKeyframe = nullptr;

        for (u32 idx = 0; idx < m_numKeyframes - 1; ++idx) {
            currKeyframe = &m_keyframes[idx];
            nextKeyframe = &m_keyframes[idx + 1];
            if (currKeyframe->m_time <= time && time < nextKeyframe->m_time) {
                break;
            }
        }

        f32 timePastKeyframe = time - currKeyframe->m_time;
        f32 keyframeDuration = nextKeyframe->m_time - currKeyframe->m_time;

        timePastKeyframe = std::clamp(timePastKeyframe, 0.0f, keyframeDuration);

        return SmoothLerp(currKeyframe->m_interpRate, nextKeyframe->m_interpRate,
                timePastKeyframe / keyframeDuration);
    }

    [[nodiscard]] f32 rate() const {
        return m_rate;
    }

private:
    /// @brief Represents a single keyframe in the interpolation sequence
    struct InterpKeyframe {
        InterpKeyframe() : m_interpRate(0.0f), m_time(0.0f) {}

        f32 m_interpRate; ///< The interpolation rate at this keyframe
        f32 m_time;       ///< The time at which this keyframe occurs in the interpolation sequence
    };

    /// @addr{0x8074C3F0}
    /// @brief Performs a smooth interpolation between two values based on a parameter t
    /// @param a The starting value
    /// @param b The ending value
    /// @param t The interpolation parameter [0, 1]
    [[nodiscard]] static f32 SmoothLerp(f32 a, f32 b, f32 t) {
        f32 sin = EGG::Mathf::SinFIdx(RAD2FIDX * (-HALF_PI + t * (HALF_PI - -HALF_PI)));
        f32 interpFactor = std::clamp(0.5f * (1.0f + sin), 0.0f, 1.0f);

        return a + interpFactor * (b - a);
    }

    owning_span<InterpKeyframe> m_keyframes; ///< Array of keyframes for the interpolation sequence
    u32 m_numKeyframes;                      ///< Number of keyframes in the interpolation sequence
    f32 m_time;                              ///< Current time in the interpolation sequence
    f32 m_rate;                              ///< Current interpolation rate
};

/// @brief Represents a single Lakitu and houses some state management members
/// @details On each frame, evaluates switches to determine if Lakitu should be toggled on/off. When
/// Lakitu is idle and a switch is toggled, transitions to the appropriate state and evaluates
/// state-specific logic. For Kinoko, we only need to implement the reverse switch, which toggles
/// Lakitu on when the player is driving backwards for 60 frames.
class JugemUnit : public StateManager {
public:
    JugemUnit(const Kart::KartObject *kartObj);
    ~JugemUnit();

    /// @addr{0x80721EC0}
    /// @brief Creates the switches for the Lakitu
    void createSwitchRace() {
        m_switchReverse = EGG::egg_new<JugemSwitchReverse>();
    }

    /// @addr{0x80722100}
    /// @brief Initializes the internal state of the Lakitu unit
    void init() {
        m_pos.setZero();
        m_move->init();
    }

    void calc();

private:
    /// @brief Describes the current state of the Lakitu unit, which determines how it behaves and
    /// responds to switches
    enum class State {
        Away = 0,       ///< Lakitu is idle and not visible
        Descending = 1, ///< Lakitu is descending to the player
        Stay = 2,       ///< Lakitu is active and is following the player
        Ascending = 3,  ///< Lakitu is ascending and leaving the player
    };

    /// @addr{0x80723458}
    /// @brief Runs once when the Lakitu begins ascending and leaving the player
    void enterIdle() {
        m_state = State::Away;
        m_ascendTimer = 0;
    }

    void enterReverse();

    /// @addr{0x807234A4}
    /// @brief Runs every frame when the Lakitu is idle and not visible, checking for switch toggles
    void calcIdle() {
        if (m_switchReverse && m_switchReverse->isOn()) {
            m_nextStateId = 1;
        }
    }

    void calcReverse();

    /// @addr{0x80722ED8}
    /// @brief Evaluates the switches for the Lakitu unit, checking if it should be toggled on/off
    void calcSwitches() {
        if (m_switchReverse) {
            m_switchReverse->calc();
        }
    }

    /// @addr{0x80722F6C}
    /// @brief Updates the Lakitu's position based on the provided transformation matrix, while
    /// keeping the y-coordinate fixed to the player's position
    void setPosFromTransform(const EGG::Matrix34f &mat) {
        m_pos = mat.base(3);
        m_pos.y = m_kartObj->pos().y;
    }

    [[nodiscard]] EGG::Vector3f transformLocalToWorldUpright(const EGG::Vector3f &v) const;
    void calcCollision();

    const Kart::KartObject *m_kartObj; ///< Pointer to the player's kart object
    EGG::Vector3f m_pos;               ///< Lakitu's position in world space
    State m_state;                     ///< The current state of the Lakitu unit
    u32 m_ascendTimer;                 ///< How long Lakitu has been disappearing/ascending for
    JugemSwitch *m_switchReverse; ///< Pointer to the switch that toggles when driving backwards
    JugemMove *m_move;            ///< Pointer to the Lakitu movement controller
    JugemInterp *m_interp;        ///< Pointer to the keyframe interpolation controller

    /// @brief Linearly interpolates between two vectors
    [[nodiscard]] static EGG::Vector3f Interpolate(f32 t, const EGG::Vector3f &v0,
            const EGG::Vector3f &v1) {
        return v0 + (v1 - v0) * t;
    }

    static constexpr std::array<StateManagerEntry, 2> STATE_ENTRIES = {{
            StateEntry<JugemUnit, &JugemUnit::enterIdle, &JugemUnit::calcIdle>(0),
            StateEntry<JugemUnit, &JugemUnit::enterReverse, &JugemUnit::calcReverse>(1),
    }};
};

} // namespace Kinoko::Field
