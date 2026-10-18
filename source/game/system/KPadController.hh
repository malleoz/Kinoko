#pragma once

#include "game/system/GhostFile.hh"

#include <egg/math/Vector.hh>

namespace Kinoko::System {

/// @brief Converts a raw stick input into the range used by @ref RaceInputState.
/// @param rawStick The raw stick input in the range `[0, 14]` to convert
/// @return The converted input in the range  `[-1.0f, 1.0f]`
[[nodiscard]] static constexpr f32 RawStickToState(u8 rawStick) {
    return (static_cast<f32>(rawStick) - 7.0f) / 7.0f;
}

/// @brief Generates an array of the 15 discrete stick values in the range `[-1.0f, 1.0f]`
/// @return An array of the 15 discrete stick values in the range `[-1.0f, 1.0f]`
[[nodiscard]] consteval std::array<f32, 15> StickStates() {
    std::array<f32, 15> states{};
    for (size_t i = 0; i < states.size(); ++i) {
        states[i] = RawStickToState(static_cast<u8>(i));
    }
    return states;
}

/// @brief Describes the type of controller being used
enum class ControlSource {
    Unknown = -1,  ///< No controller
    Core = 0,      ///< WiiMote
    Freestyle = 1, ///< WiiMote + Nunchuk
    Classic = 2,   ///< Classic Controller
    Gamecube = 3,  ///< GameCube Controller
    Ghost = 4,     ///< The inputs are driven by the ghost data
    AI = 5,        ///< CPU player controller
    Host = 6,      // Added in Kinoko, represents an external program
};

/// @brief Describes the type of trick input being pressed on the controller
enum class Trick {
    None = 0,  ///< No trick input
    Up = 1,    ///< An up trick
    Down = 2,  ///< A down trick
    Left = 3,  ///< A left side trick
    Right = 4, ///< A right side trick
};

/// @brief Represents the state of controller inputs
struct RaceInputState {
    u16 buttons;         ///< A bitfield of the buttons being pressed
    u16 buttonsRaw;      ///< Raw representation of button presses
    EGG::Vector2f stick; ///< The X and Y components of the analog stick `[-1.0f, 1.0f]`
    u8 stickXRaw;        ///< The integer X component of the analog stick `[0, 14]`
    u8 stickYRaw;        ///< The integer Y component of the analog stick `[0, 14]`
    Trick trick;         ///< The current @ref Trick being performed
    u8 trickRaw;         ///< Raw representation of the trick being performed

    RaceInputState();
    virtual ~RaceInputState();

    void reset();

    [[nodiscard]] bool isValid() const;
    [[nodiscard]] bool isButtonsValid() const;
    [[nodiscard]] bool isTrickValid() const;
    [[nodiscard]] bool accelerate() const;
    [[nodiscard]] bool brake() const;
    [[nodiscard]] bool item() const;
    [[nodiscard]] bool drift() const;
    [[nodiscard]] bool trickUp() const;
    [[nodiscard]] bool trickDown() const;

    [[nodiscard]] static bool IsStickValid(f32 stick);

    /// @brief The discrete analog stick inputs in the range `[-1.0f, 1.0f]`
    static constexpr std::array<f32, 15> STICK_STATES = StickStates();
};

/// @brief Represents a stream of button inputs from a ghost file.
/// @details Inputs are stored in a 2 byte tuple of the form `(input state, duration)`, where the
/// input state is the first byte and the duration (in frames) is the second byte. When fetching a
/// frame's input, this struct is responsible for tracking how much time has elapsed in the current
/// tuple and detecting when the next tuple should be read.
struct KPadGhostButtonsStream {
    EGG::RamStream buffer;  ///< The underlying buffer storing the button input tuples
    u16 currentSequence;    ///< The current input sequence being read
    u16 readSequenceFrames; ///< The number of frames elapsed in the current input tuple
    u32 state;              ///< The state of the stream (1 = active, 2 = end-of-stream)

    KPadGhostButtonsStream();
    virtual ~KPadGhostButtonsStream();

    [[nodiscard]] virtual bool readIsNewSequence() const;
    [[nodiscard]] virtual u8 readVal() const;

    [[nodiscard]] u8 readFrame();
};

/// @brief A specialized stream for button presses (not tricks).
/// @details Reads in the status for acceleration, braking, item usage, and drifting. The button
/// tuples take the following form:
/// Bitmask  | Description
///--------- | -------------
/// 0x0100   | **Accelerating**
/// 0x0200   | **Braking/Drifting**
/// 0x0400   | **Item usage**
/// 0x0800   | Set if braking/drifting pressed after pressing accelerating
/// 0x00FF   | Duration
/// @warning When bitmask 0x08 is set, the game will register a hop regardless of whether or
/// not the acceleration button is pressed. This can lead to "successful" synchronization of
/// ghosts which could not have been created legitimately in the first place.
struct KPadGhostFaceButtonsStream final : public KPadGhostButtonsStream {
    /// @brief Default constructor
    KPadGhostFaceButtonsStream() = default;

    /// @brief Default virtual destructor
    ~KPadGhostFaceButtonsStream() override = default;
};

/// @brief A specialized stream for the analog stick.
/// @details Direction tuples take the following form:
/// Bitmask    | Description
///----------- | -------------
/// 0x0F00     | Up/Down (0xE = Up, 0x0 = Down, 0x7 = Neutral)
/// 0xF000     | Left/Right (0xE0 = Right, 0x00 = Left, 0x70 = Neutral)
/// 0x00FF     | Duration
struct KPadGhostDirectionButtonsStream final : public KPadGhostButtonsStream {
    /// @brief Default constructor
    KPadGhostDirectionButtonsStream() = default;

    /// @brief Default virtual destructor
    ~KPadGhostDirectionButtonsStream() override = default;
};

/// @brief A specialized stream for D-Pad inputs for tricking and wheeling.
/// @details Trick tuples take the following form:
/// Bitmask | Description
///-------- | -------------
/// 0x7000  | 0x00 = No trick, 0x10 = Up/Wheelie, 0x20 = Down, 0x30 = Left, 0x40 = Right
/// 0x0FFF  | Duration
struct KPadGhostTrickButtonsStream final : public KPadGhostButtonsStream {
    /// @brief Default constructor
    KPadGhostTrickButtonsStream() = default;

    /// @brief Default virtual destructor
    ~KPadGhostTrickButtonsStream() override = default;

    /// @addr{0x805250A8}
    /// @copydoc KPadGhostButtonsStream::readIsNewSequence
    /// @details Handles the fact that the duration for trick tuples is 12 bits instead of the
    /// typical 8.
    [[nodiscard]] bool readIsNewSequence() const override {
        u16 duration = currentSequence & 0xFF;
        duration += 256 * (currentSequence >> 8 & 0xF);
        return duration <= readSequenceFrames;
    }

    /// @addr{0x8052509C}
    /// @copydoc KPadGhostButtonsStream::readVal
    [[nodiscard]] u8 readVal() const override {
        return currentSequence >> 0x8 & ~0x80;
    }
};

/// @brief An abstraction for a controller object. It is associated with an input state.
class KPadController {
public:
    KPadController();
    virtual ~KPadController();

    virtual void reset(bool driftIsAuto);
    virtual void calcImpl();

    void calc();

    /// @beginSetters

    void setDriftIsAuto(bool driftIsAuto);

    /// @endSetters

    /// @beginGetters

    [[nodiscard]] virtual ControlSource controlSource() const;
    [[nodiscard]] const RaceInputState &raceInputState() const;
    [[nodiscard]] bool driftIsAuto() const;

    /// @endGetters

protected:
    RaceInputState m_raceInputState; ///< The current inputs from this controller.
    bool m_connected;                ///< Whether the controller is active.
    bool m_driftIsAuto;              ///< True for auto transmission, false for manual.
};

/// @brief The abstraction of a controller object but for ghost playback.
/// @details The ghost data buffer is split into three sections: face buttons, analog stick, and the
/// D-Pad. Each section is an array of tuples, where each tuple contains the input state and the
/// duration of that input state. This is used to minimize data consumption given that the user is
/// not changing inputs every frame. The header of the RKG input data section is as follows:
/// Offset | Size    | Description                                         |
///------- | ------- | --------------------------------------------------- |
/// 0x00   | 2 bytes | Count of face button input tuples                   |
/// 0x02   | 2 bytes | Count of analog stick input tuples                  |
/// 0x04   | 2 bytes | Count of D-Pad input tuples                         |
/// 0x06   | 2 bytes | Unknown. Probably padding.                          |
/// 0x08   |         | End of header, beginning of face button input data. |
class KPadGhostController final : public KPadController {
public:
    KPadGhostController();
    ~KPadGhostController() override;

    [[nodiscard]] ControlSource controlSource() const override;
    void reset(bool driftIsAuto) override;
    void calcImpl() override;

    void readGhostBuffer(const u8 *buffer, bool driftIsAuto);

    /// @beginSetters

    void setAcceptingInputs(bool set);

    /// @endSetters

private:
    const u8 *m_ghostBuffer; ///< Pointer to the uncompressed ghost input data buffer
    std::array<KPadGhostButtonsStream *, 3> m_buttonsStreams; ///< Array of ghost input data streams
    bool m_acceptingInputs; ///< Whether the controller is currently accepting inputs
};

/// @brief The abstraction of a controller object but for external usage.
/// @details The input state is managed externally by programs interfacing with Kinoko.
class KPadHostController final : public KPadController {
public:
    KPadHostController();
    ~KPadHostController() override;

    [[nodiscard]] ControlSource controlSource() const override;
    void reset(bool driftIsAuto) override;

    bool setInputs(const RaceInputState &state);
    bool setInputs(u16 buttons, const EGG::Vector2f &stick, Trick trick);
    bool setInputs(u16 buttons, f32 stickX, f32 stickY, Trick trick);
    bool setInputsRawStick(u16 buttons, u8 stickXRaw, u8 stickYRaw, Trick trick);
    bool setInputsRawStickZeroCenter(u16 buttons, s8 stickXRaw, s8 stickYRaw, Trick trick);
};

/// @brief A wrapper class which holds a @ref KPadController and manages a player's input state
class KPad {
public:
    KPad();
    virtual ~KPad();

    void calc();
    void reset();

    /// @beginGetters

    [[nodiscard]] const RaceInputState &currentState() const;
    [[nodiscard]] const RaceInputState &lastState() const;
    [[nodiscard]] bool driftIsAuto() const;

    /// @endGetters

protected:
    KPadController *m_controller;       ///< Pointer to the associated controller
    RaceInputState m_currentInputState; ///< The current input state of the pad
    RaceInputState m_lastInputState;    ///< Last frame's input state, to determine changes in state
};

/// @brief A specialized KPad for player input, as opposed to CPU players for example.
class KPadPlayer final : public KPad {
public:
    KPadPlayer();
    ~KPadPlayer() override;

    void setGhostController(KPadGhostController *controller, const u8 *inputs, bool driftIsAuto);
    void setHostController(KPadHostController *controller, bool driftIsAuto);

    void startGhostProxy();
    void endGhostProxy();

private:
    u8 m_ghostBuffer[RKG_UNCOMPRESSED_INPUT_DATA_SECTION_SIZE]; ///< Buffer storing ghost input data
};

} // namespace Kinoko::System
