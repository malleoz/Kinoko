#pragma once

#include "game/system/GhostFile.hh"

#include <egg/math/Vector.hh>

#include <algorithm>

namespace Kinoko::System {

/// @brief Converts a raw stick input into an input usable by the state.
/// @param rawStick The raw stick input to convert `[0, 14]`
/// @return The converted input `[-1.0f, 1.0f]`
[[nodiscard]] static constexpr f32 RawStickToState(u8 rawStick) {
    return (static_cast<f32>(rawStick) - 7.0f) / 7.0f;
}

/// @brief Generates an array of the 15 discrete stick values in the range `[-1.0f, 1.0f]`
/// @return
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
    /// @brief Constructor that initializes the input state by resetting all values
    RaceInputState() {
        reset();
    }

    /// @brief Default virtual destructor
    virtual ~RaceInputState() = default;

    /// @addr{0x8051E85C}
    /// @brief Resets the input state to its default values
    void reset() {
        buttons = buttonsRaw = 0;
        stick = EGG::Vector2f::zero;
        stickXRaw = stickYRaw = 7;
        trick = Trick::None;
        trickRaw = 0;
    }

    /// @brief Checks if the input state is valid.
    /// @return True if the input state is valid, false otherwise.
    /// @details The input state is valid if no invalid buttons are being pressed, the stick values
    /// are one of 15 discrete values, and the trick bit is one of the 5 valid values.
    [[nodiscard]] bool isValid() const {
        bool isValid = isButtonsValid();
        isValid = isValid && isStickValid(stick.x);
        isValid = isValid && isStickValid(stick.y);
        isValid = isValid && isTrickValid();

        return isValid;
    }

    /// @brief Checks if there are any invalid buttons
    /// @return True if no invalid buttons are present, false otherwise.
    /// @details Validation with the previous input state doesn't happen because it doesn't exist.
    /// Therefore, we cannot check here if e.g. the drift button is pressed when it shouldn't be.
    [[nodiscard]] bool isButtonsValid() const {
        return !(buttons & ~0xf);
    }

    /// @brief Checks if the stick values are within the domain of the physics engine.
    /// @details The set of valid stick values is \f$\{\frac{x-7}{7}|0\leq x\leq
    /// 14,\in\mathbb{Z}\}\f$. It's possible for the stick input to be 8/7 with x = 15, but only
    /// with ghost controllers.
    /// @return If the stick values are valid.
    [[nodiscard]] bool isStickValid(f32 stick) const {
        return std::ranges::any_of(STICK_STATES, [stick](f32 val) { return stick == val; });
    }

    /// @brief Checks if the trick input is valid.
    /// @return If the trick input is valid.
    [[nodiscard]] bool isTrickValid() const {
        switch (trick) {
        case Trick::None:
        case Trick::Up:
        case Trick::Down:
        case Trick::Left:
        case Trick::Right:
            return true;
        default:
            return false;
        }
    }

    /// @brief Checks if the acceleration button is being pressed
    /// @return True if the acceleration button is being pressed, false otherwise.
    /// @details The accelerate button is the first button in the bitmask (0x01).
    [[nodiscard]] bool accelerate() const {
        return !!(buttons & 0x1);
    }

    /// @brief Checks if the brake button is being pressed
    /// @return True if the brake button is being pressed, false otherwise.
    /// @details The brake button is the second button in the bitmask (0x02).
    [[nodiscard]] bool brake() const {
        return !!(buttons & 0x2);
    }

    /// @brief Checks if the item button is being pressed
    /// @return True if the item button is being pressed, false otherwise.
    /// @details The item button is the third button in the bitmask (0x04).
    [[nodiscard]] bool item() const {
        return !!(buttons & 0x4);
    }

    /// @brief Checks if the drift button is being pressed
    /// @return True if the drift button is being pressed, false otherwise.
    /// @details The drift button is the fourth button in the bitmask (0x08).
    /// @warning When set, the game will register a hop regardless of whether or not the
    /// acceleration button is pressed. This can lead to "successful" synchronization of ghosts
    /// which could not have been created legitimately in the first place.
    [[nodiscard]] bool drift() const {
        return !!(buttons & 0x8);
    }

    /// @brief Checks if the up trick button is being pressed
    /// @return True if the up trick button is being pressed, false otherwise.
    [[nodiscard]] bool trickUp() const {
        return trick == Trick::Up;
    }

    /// @brief Checks if the down trick button is being pressed
    /// @return True if the down trick button is being pressed, false otherwise.
    [[nodiscard]] bool trickDown() const {
        return trick == Trick::Down;
    }

    u16 buttons;    ///< A bitfield of the buttons being pressed
    u16 buttonsRaw; ///< Raw representation of button presses (0x1=accel, 0x3=accel+brake, etc.)
    EGG::Vector2f stick; ///< The X and Y components of the analog stick `[-1.0f, 1.0f]`
    u8 stickXRaw;        ///< The integer X component of the analog stick `[0, 14]`
    u8 stickYRaw;        ///< The integer Y component of the analog stick `[0, 14]`
    Trick trick;         ///< The current @ref Trick being performed
    u8 trickRaw;         ///< Raw representation of the trick being performed

    /// @brief The discrete analog stick inputs in the range `[-1.0f, 1.0f]`
    static constexpr std::array<f32, 15> STICK_STATES = StickStates();
};

/// @brief Represents a stream of button inputs from a ghost file.
/// @details Inputs are stored in a 2 byte tuple of the form `(input state, duration)`, where the
/// input state is the first byte and the duration (in frames) is the second byte. When fetching a
/// frame's input, this struct is responsible for tracking how much time has elapsed in the current
/// tuple and detecting when the next tuple should be read.
struct KPadGhostButtonsStream {
    KPadGhostButtonsStream();
    virtual ~KPadGhostButtonsStream();

    [[nodiscard]] u8 readFrame();

    /// @addr{0x8052502C} @addr{0x80524FC4}
    /// @brief Checks if the current sequence has reached a new input tuple
    /// @return True if the current sequence has reached a new input tuple, false otherwise.
    [[nodiscard]] virtual bool readIsNewSequence() const {
        return readSequenceFrames >= (currentSequence & 0xFF);
    }

    /// @addr{0x80525024} @addr{0x80524FBC}
    /// @brief Reads the current input value from the sequence
    /// @return The current input value.
    [[nodiscard]] virtual u8 readVal() const {
        return currentSequence >> 8;
    }

    EGG::RamStream buffer;  ///< The underlying buffer storing the button input tuples
    u16 currentSequence;    ///< The current input sequence being read
    u16 readSequenceFrames; ///< The number of frames elapsed in the current input tuple
    u32 state;              ///< The state of the stream (1 = end-of-input, 2 = active)
};

/// @brief A specialized stream for button presses (not tricks).
/// @details Reads in the status for acceleration, braking, item usage, and drifting. The button
/// tuples take the following form:
/// Bitmask  | Description
///------------- | -------------
/// 0x01  | **Accelerating**
/// 0x02  | **Braking/Drifting**
/// 0x04  | **Item usage**
/// 0x08  | Set if braking/drifting pressed after pressing accelerating
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
/// Bitmask  | Description
///------------- | -------------
/// 0x0F  | Up/Down (0xE = Up, 0x0 = Down, 0x7 = Neutral)
/// 0xF0  | Left/Right (0xE0 = Right, 0x00 = Left, 0x70 = Neutral)
struct KPadGhostDirectionButtonsStream final : public KPadGhostButtonsStream {
    /// @brief Default constructor
    KPadGhostDirectionButtonsStream() = default;

    /// @brief Default virtual destructor
    ~KPadGhostDirectionButtonsStream() override = default;
};

/// @brief A specialized stream for D-Pad inputs for tricking and wheeling.
/// @details Trick tuples take the following form:
/// Bitmask  | Description
///------------- | -------------
/// 0x0F  | The upper four bits of the tuple's duration, forming a 12-bit integer.
/// 0x70  | 0x00 = No trick, 0x10 = Up/Wheelie, 0x20 = Down, 0x30 = Left, 0x40 = Right
struct KPadGhostTrickButtonsStream final : public KPadGhostButtonsStream {
    /// @brief Default constructor
    KPadGhostTrickButtonsStream() = default;

    /// @brief Default virtual destructor
    ~KPadGhostTrickButtonsStream() override = default;

    /// @addr{0x805250A8}
    /// @copydoc KPadGhostButtonsStream::readIsNewSequence
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
    /// @addr{0x8051EBA8}
    /// @brief Default constructor
    KPadController() : m_connected(false) {}

    /// @brief Default virtual destructor
    virtual ~KPadController() {}

    /// @brief Resets the state of the controller
    virtual void reset(bool /*driftIsAuto*/) {}

    /// @brief Virtual function responsible for handling the current frame's input state calculation
    virtual void calcImpl() {}

    /// @addr{0x8051ED14}
    void calc() {
        calcImpl();
    }

    /// @beginSetters
    /// @addr{0x8051F37C}
    void setDriftIsAuto(bool driftIsAuto) {
        m_driftIsAuto = driftIsAuto;
    }
    /// @endSetters

    /// @beginGetters
    /// @addr{0x8051CE7C}
    /// @brief Returns the source of the control input.
    /// @return The source of the control input.
    [[nodiscard]] virtual ControlSource controlSource() const {
        return ControlSource::Unknown;
    }

    [[nodiscard]] const RaceInputState &raceInputState() const {
        return m_raceInputState;
    }

    [[nodiscard]] bool driftIsAuto() const {
        return m_driftIsAuto;
    }
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

    /// @addr{0x8052282C}
    /// @copydoc KPadController::controlSource()
    [[nodiscard]] ControlSource controlSource() const override {
        return ControlSource::Ghost;
    }

    void reset(bool driftIsAuto) override;

    void readGhostBuffer(const u8 *buffer, bool driftIsAuto);

    void calcImpl() override;

    /// @brief Sets whether the controller should accept inputs.
    /// @param set True to accept inputs, false to ignore them.
    /// @details Effectively, this signals when the countdown has started so that inputs should
    /// start being read from the ghost buffer.
    void setAcceptingInputs(bool set) {
        m_acceptingInputs = set;
    }

private:
    const u8 *m_ghostBuffer; ///< Pointer to the uncompressed ghost input data buffer
    std::array<KPadGhostButtonsStream *, 3> m_buttonsStreams; ///< Array of ghost input data streams
    bool m_acceptingInputs; ///< Whether the controller is currently accepting inputs
};

/// @brief The abstraction of a controller object but for external usage.
/// @details The input state is managed externally by programs interfacing with Kinoko.
class KPadHostController final : public KPadController {
public:
    /// @brief Default constructor
    KPadHostController() = default;

    /// @brief Default virtual destructor
    ~KPadHostController() override = default;

    /// @copydoc KPadController::controlSource()
    [[nodiscard]] ControlSource controlSource() const override {
        return ControlSource::Host;
    }

    /// @copydoc KPadController::reset()
    /// @param driftIsAuto Indicates whether the controller should be set to auto drift mode.
    void reset(bool driftIsAuto) override {
        m_driftIsAuto = driftIsAuto;
        m_raceInputState.reset();
        m_connected = true;
    }

    /// @brief Sets the inputs of the controller.
    /// @param state The specified inputs packaged in the state. Only buttons, stick, and trick
    /// matter.
    /// @return Input state validity.
    bool setInputs(const RaceInputState &state) {
        return setInputs(state.buttons, state.stick, state.trick);
    }

    /// @brief Sets the inputs of the controller.
    /// @param buttons The button inputs.
    /// @param stick The stick inputs, as a 2D vector.
    /// @param trick The trick input.
    /// @return Input state validity.
    bool setInputs(u16 buttons, const EGG::Vector2f &stick, Trick trick) {
        m_raceInputState.buttons = buttons;
        m_raceInputState.stick = stick;
        m_raceInputState.trick = trick;

        return m_raceInputState.isValid();
    }

    /// @brief Sets the inputs of the controller.
    /// @param buttons The button inputs.
    /// @param stickX The stick input on the X axis.
    /// @param stickY The stick input on the Y axis.
    /// @param trick The trick input.
    /// @return Input state validity.
    bool setInputs(u16 buttons, f32 stickX, f32 stickY, Trick trick) {
        m_raceInputState.buttons = buttons;
        m_raceInputState.stick.x = stickX;
        m_raceInputState.stick.y = stickY;
        m_raceInputState.trick = trick;

        return m_raceInputState.isValid();
    }

    /// @brief Sets the inputs of the controller.
    /// @details A different name is specified to avoid any ambiguity with the parameters.
    /// @param buttons The button inputs.
    /// @param stickXRaw The 7-centered raw stick input on the X axis.
    /// @param stickYRaw The 7-centered raw stick input on the Y axis.
    /// @param trick The trick input.
    /// @return Input state validity.
    bool setInputsRawStick(u16 buttons, u8 stickXRaw, u8 stickYRaw, Trick trick) {
        return setInputs(buttons, RawStickToState(stickXRaw), RawStickToState(stickYRaw), trick);
    }

    /// @brief Sets the inputs of the controller.
    /// @details A different name is specified to avoid any ambiguity with the parameters.
    /// @param buttons The button inputs.
    /// @param stickXRaw The 0-centered raw stick input on the X axis.
    /// @param stickYRaw The 0-centered raw stick input on the Y axis.
    /// @param trick The trick input.
    /// @return Input state validity.
    bool setInputsRawStickZeroCenter(u16 buttons, s8 stickXRaw, s8 stickYRaw, Trick trick) {
        return setInputsRawStick(buttons, stickXRaw + 7, stickYRaw + 7, trick);
    }
};

/// @brief A wrapper class which holds a @ref KPadController and manages a player's input state
class KPad {
public:
    /// @addr{0x80520F64}
    /// @brief Default constructor
    KPad() : m_controller(nullptr) {
        reset();
    }

    /// @addr{0x805222B4}
    /// @brief Default destructor
    ~KPad() = default;

    /// @addr{0x80521198}
    /// @brief Updates the last and current input states to reflect the state of the controller
    void calc() {
        m_lastInputState = m_currentInputState;
        m_currentInputState = m_controller->raceInputState();
    }

    /// @addr{0x80521110}
    /// @brief Resets the controller and its input state
    void reset() {
        if (m_controller) {
            m_controller->reset(m_controller->driftIsAuto());
        }
    }

    /// @beginGetters
    [[nodiscard]] const RaceInputState &currentState() const {
        return m_currentInputState;
    }

    [[nodiscard]] const RaceInputState &lastState() const {
        return m_lastInputState;
    }

    [[nodiscard]] bool driftIsAuto() const {
        return m_controller->driftIsAuto();
    }
    /// @endGetters

protected:
    KPadController *m_controller;       ///< Pointer to the associated controller
    RaceInputState m_currentInputState; ///< The current input state of the pad
    RaceInputState m_lastInputState;    ///< Last frame's input state, to determine changes in state
};

/// @brief A specialized KPad for player input, as opposed to CPU players for example.
class KPadPlayer final : public KPad {
public:
    /// @addr{0x805220BC}
    /// @brief Default constructor
    KPadPlayer() = default;

    /// @addr{0x805222F4}
    /// @brief Default destructor
    ~KPadPlayer() = default;

    /// @addr{0x80521844}
    /// @brief Sets the ghost controller and copies input data into the ghost buffer
    /// @param controller Pointer to the ghost controller to drive this pad
    /// @param inputs Pointer to the input data to copy into the ghost buffer
    /// @param driftIsAuto Whether the drift is set to automatic
    void setGhostController(KPadGhostController *controller, const u8 *inputs, bool driftIsAuto) {
        m_controller = controller;

        if (inputs) {
            memcpy(m_ghostBuffer, inputs, RKG_UNCOMPRESSED_INPUT_DATA_SECTION_SIZE);
        }

        controller->readGhostBuffer(m_ghostBuffer, driftIsAuto);
    }

    /// @brief Sets the host controller to drive this pad
    /// @param controller Pointer to the host controller to drive this pad
    /// @param driftIsAuto Whether the drift is set to automatic
    void setHostController(KPadHostController *controller, bool driftIsAuto) {
        m_controller = controller;
        m_controller->setDriftIsAuto(driftIsAuto);
    }

    /// @addr{0x805215D4}
    /// @brief Starts the ghost proxy, allowing the ghost controller to accept inputs
    /// @details Effectively, this signals when the countdown has started so that inputs should
    /// start being read from the ghost buffer.
    void startGhostProxy() {
        if (!m_controller || m_controller->controlSource() != ControlSource::Ghost) {
            return;
        }

        KPadGhostController *ghostController =
                reinterpret_cast<KPadGhostController *>(m_controller);
        ghostController->setAcceptingInputs(true);
    }

    /// @addr{0x80521688}
    /// @brief Ends the ghost proxy, preventing the ghost controller from accepting inputs
    /// @details Effectively, this signals when the race has ended.
    void endGhostProxy() {
        if (!m_controller || m_controller->controlSource() != ControlSource::Ghost) {
            return;
        }

        KPadGhostController *ghostController =
                reinterpret_cast<KPadGhostController *>(m_controller);
        ghostController->setAcceptingInputs(false);
    }

private:
    u8 m_ghostBuffer[RKG_UNCOMPRESSED_INPUT_DATA_SECTION_SIZE]; ///< Buffer storing ghost input data
};

} // namespace Kinoko::System
