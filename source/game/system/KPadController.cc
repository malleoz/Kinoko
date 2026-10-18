#include "KPadController.hh"

#include <egg/core/Heap.hh>

#include <algorithm>
#include <cstring>

namespace Kinoko::System {

/// @brief Constructor that initializes the input state by resetting all values
RaceInputState::RaceInputState() {
    reset();
}

/// @brief Default virtual destructor
RaceInputState::~RaceInputState() = default;

/// @addr{0x8051E85C}
/// @brief Resets the input state to its default values
/// @details Sets @ref buttons, @ref buttonsRaw, and @ref stick to zero. Sets @ref stickXRaw and
/// @ref stickYRaw to 7. Sets @ref trick to @ref Trick::None and @ref trickRaw to zero.
void RaceInputState::reset() {
    buttons = buttonsRaw = 0;
    stick = EGG::Vector2f::zero;
    stickXRaw = stickYRaw = 7;
    trick = Trick::None;
    trickRaw = 0;
}

/// @brief Checks if the input state is valid.
/// @return `true` if the input state is valid, `false` otherwise.
/// @details The input state is valid if no invalid buttons are being pressed, the stick values
/// are one of 15 discrete values, and the trick bit is one of the 5 valid values.
bool RaceInputState::isValid() const {
    bool isValid = isButtonsValid();
    isValid = isValid && IsStickValid(stick.x);
    isValid = isValid && IsStickValid(stick.y);
    isValid = isValid && isTrickValid();

    return isValid;
}

/// @brief Checks if there are any invalid buttons
/// @return `true` if no invalid buttons are present, `false` otherwise.
/// @details Validation with the previous input state doesn't happen because it doesn't exist.
/// Therefore, we cannot check here if e.g. the drift button is pressed when it shouldn't be.
bool RaceInputState::isButtonsValid() const {
    return !(buttons & ~0xf);
}

/// @brief Checks if the trick input is valid.
/// @return `true` if the trick input is valid, `false` otherwise.
bool RaceInputState::isTrickValid() const {
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
/// @return `true` if the acceleration button is being pressed, `false` otherwise.
/// @details The accelerate button is the first button in the bitmask (0x01).
bool RaceInputState::accelerate() const {
    return !!(buttons & 0x1);
}

/// @brief Checks if the brake button is being pressed
/// @return `true` if the brake button is being pressed, `false` otherwise.
/// @details The brake button is the second button in the bitmask (0x02).
bool RaceInputState::brake() const {
    return !!(buttons & 0x2);
}

/// @brief Checks if the item button is being pressed
/// @return `true` if the item button is being pressed, `false` otherwise.
/// @details The item button is the third button in the bitmask (0x04).
bool RaceInputState::item() const {
    return !!(buttons & 0x4);
}

/// @brief Checks if the drift button is being pressed
/// @return `true` if the drift button is being pressed, `false` otherwise.
/// @details The drift button is the fourth button in the bitmask (0x08).
/// @warning When set, the game will register a hop regardless of whether or not the
/// acceleration button is pressed. This can lead to "successful" synchronization of ghosts
/// which could not have been created legitimately in the first place.
bool RaceInputState::drift() const {
    return !!(buttons & 0x8);
}

/// @brief Checks if the up trick button is being pressed
/// @return `true` if the up @ref trick button is being pressed, `false` otherwise.
bool RaceInputState::trickUp() const {
    return trick == Trick::Up;
}

/// @brief Checks if the down trick button is being pressed
/// @return `true` if the down @ref trick button is being pressed, `false` otherwise.
bool RaceInputState::trickDown() const {
    return trick == Trick::Down;
}

/// @brief Checks if the provided stick value is within the domain of the physics engine.
/// @param stick The stick value to check
/// @return `true` if the stick value is valid, `false` otherwise.
/// @details The set of valid stick values is \f$\{\frac{x-7}{7}|0\leq x\leq
/// 14,\in\mathbb{Z}\}\f$. It's possible for the stick input to be 8/7 with x = 15, but only
/// with ghost controllers.
bool RaceInputState::IsStickValid(f32 stick) {
    return std::ranges::any_of(STICK_STATES, [stick](f32 val) { return stick == val; });
}

/// @brief Default constructor
KPadGhostButtonsStream::KPadGhostButtonsStream()
    : currentSequence(std::numeric_limits<u16>::max()),
      state(2) {}

/// @brief Default virtual destructor
KPadGhostButtonsStream::~KPadGhostButtonsStream() = default;

/// @addr{0x8052502C} @addr{0x80524FC4}
/// @brief Checks if the current sequence has reached a new input tuple
/// @return `true` if the current sequence has reached a new input tuple, `false` otherwise.
bool KPadGhostButtonsStream::readIsNewSequence() const {
    return readSequenceFrames >= (currentSequence & 0xFF);
}

/// @addr{0x80525024} @addr{0x80524FBC}
/// @brief Reads the current input value from the sequence
/// @return The current input value from the @ref currentSequence tuple
u8 KPadGhostButtonsStream::readVal() const {
    return currentSequence >> 8;
}

/// @addr{0x80520D4C} @addr{0x80522C5C} @addr{0x80522F40}
/// @brief Reads the data from the corresponding tuple in the buffer.
/// @return The input value for the current frame.
/// @details If the stream has already reached the end of the input buffer (denoted by `state ==
/// 1`), the function early returns zero. If @ref currentSequence is uninitialized, then reads the
/// first sequence from the buffer and resets @ref readSequenceFrames to zero. Otherwise, checks to
/// see if a new sequence should be read. If so, reads the next sequence from the buffer and resets
/// @ref readSequenceFrames to zero. Either way, increments @ref readSequenceFrames. Checks to see
/// if the stream has reached the end of the input buffer this frame and updates @ref state to `2`
/// if so. Finally, returns the current input value for the frame.
/// @note In the base game, this is a virtual function, but no derived class overrides this
/// function. For the purposes of Kinoko, we can devirtualize.
u8 KPadGhostButtonsStream::readFrame() {
    if (state != 1) {
        return 0;
    }

    if (currentSequence == std::numeric_limits<u16>::max()) {
        readSequenceFrames = 0;
        currentSequence = buffer.read_u16();
    } else {
        if (readIsNewSequence()) {
            readSequenceFrames = 0;
            currentSequence = buffer.read_u16();
        }
    }

    ++readSequenceFrames;

    // In the base game, this check normally occurs before a new sequence is read. As a result, the
    // base game does not know that it has run out of inputs until the frame that it tries to access
    // past the last valid input. We stray from this behavior so that we can know when we are on the
    // last frame of input.
    if (buffer.eof() && readIsNewSequence()) {
        state = 2;
    }

    return readVal();
}

/// @addr{0x8051EBA8}
/// @brief Constructor
KPadController::KPadController() : m_connected(false) {}

/// @brief Default virtual destructor
KPadController::~KPadController() = default;

/// @brief Resets the state of the controller
void KPadController::reset(bool /*driftIsAuto*/) {}

/// @brief Responsible for handling the current frame's input state calculation
void KPadController::calcImpl() {}

/// @addr{0x8051ED14}
/// @brief Runs every frame to update the controller state for the current frame
void KPadController::calc() {
    calcImpl();
}

/// @addr{0x8051F37C}
/// @brief Sets whether the drift is automatic or manual.
/// @param driftIsAuto `true` for automatic drift, `false` for manual.
void KPadController::setDriftIsAuto(bool driftIsAuto) {
    m_driftIsAuto = driftIsAuto;
}

/// @addr{0x8051CE7C}
/// @brief Gets the source of the control input.
/// @return The source of the control input, @ref ControlSource::Unknown for the base class.
ControlSource KPadController::controlSource() const {
    return ControlSource::Unknown;
}

/// @brief Gets the current race input state.
/// @return The current race input state, @ref m_raceInputState
const RaceInputState &KPadController::raceInputState() const {
    return m_raceInputState;
}

/// @brief Gets whether the drift is automatic or manual.
/// @return The value of @ref m_driftIsAuto, which is `true` if the drift is automatic, `false`
/// if manual.
bool KPadController::driftIsAuto() const {
    return m_driftIsAuto;
}

/// @addr{0x80520730}
/// @brief Default constructor that creates streams for the buttons, trick buttons, and analog
/// stick
/// @details Initializes @ref m_acceptingInputs to `false`.
KPadGhostController::KPadGhostController() : m_acceptingInputs(false) {
    m_buttonsStreams[0] = EGG::egg_new<KPadGhostFaceButtonsStream>();
    m_buttonsStreams[1] = EGG::egg_new<KPadGhostDirectionButtonsStream>();
    m_buttonsStreams[2] = EGG::egg_new<KPadGhostTrickButtonsStream>();
}

/// @addr{0x80520924}
/// @brief Default virtual destructor
KPadGhostController::~KPadGhostController() = default;

/// @addr{0x8052282C}
/// @copybrief KPadController::controlSource()
/// @return The source of the control input, @ref ControlSource::Ghost
ControlSource KPadGhostController::controlSource() const {
    return ControlSource::Ghost;
}

/// @addr{0x80520998}
/// @copybrief KPadController::reset()
/// @param driftIsAuto Indicates whether the controller should be set to auto drift mode.
/// @details Sets @ref m_driftIsAuto based on the provided boolean. Resets the current input state
/// and all button streams. Finally, resets @ref m_acceptingInputs to `false` and @ref m_connected
/// to `true`.
void KPadGhostController::reset(bool driftIsAuto) {
    m_driftIsAuto = driftIsAuto;
    m_raceInputState.reset();

    for (auto &stream : m_buttonsStreams) {
        stream->currentSequence = 0;
        stream->readSequenceFrames = 0;
        stream->state = 1;
    }

    m_acceptingInputs = false;
    m_connected = true;
}

/// @addr{0x80520B9C}
/// @brief Fetches all input data from the ghost buffer streams and updates the race input state
/// @details This function early returns if @ref m_ghostBuffer has not been set or if @ref
/// m_acceptingInputs is `false`. Reads the current frame's input from the face, direction, and
/// trick streams and updates @ref m-raceInputState accordingly.
void KPadGhostController::calcImpl() {
    if (!m_ghostBuffer || !m_acceptingInputs) {
        return;
    }

    m_raceInputState.buttons = m_buttonsStreams[0]->readFrame();
    u8 sticks = m_buttonsStreams[1]->readFrame();
    m_raceInputState.stickXRaw = sticks >> 4 & 0xF;
    m_raceInputState.stickYRaw = sticks & 0xF;
    m_raceInputState.stick = EGG::Vector2f(RawStickToState(m_raceInputState.stickXRaw),
            RawStickToState(m_raceInputState.stickYRaw));
    m_raceInputState.trickRaw = m_buttonsStreams[2]->readFrame();

    switch (m_raceInputState.trickRaw >> 4) {
    case 1:
        m_raceInputState.trick = Trick::Up;
        break;
    case 2:
        m_raceInputState.trick = Trick::Down;
        break;
    case 3:
        m_raceInputState.trick = Trick::Left;
        break;
    case 4:
        m_raceInputState.trick = Trick::Right;
        break;
    default:
        m_raceInputState.trick = Trick::None;
        break;
    }
}

/// @addr{Inlined in 0x80521844}
/// @brief Splits the ghost input data sections into their associated button streams
/// @param buffer The uncompressed input data buffer from the ghost RKG file.
/// @param driftIsAuto `true` for auto transmission, `false` for manual.
/// @details Sets @ref m_ghostBuffer to the provided `buffer` and @ref m_driftIsAuto to the provided
/// boolean. Creates a @ref EGG::Stream for the entire ghost input data section. Reads the number of
/// face, direction, and trick tuples. Splits the @ref EGG::Stream into separate streams for face,
/// direction, and trick inputs, with each stream's length corresponding to the number of tuples
/// read earlier.
void KPadGhostController::readGhostBuffer(const u8 *buffer, bool driftIsAuto) {
    constexpr u32 SEQUENCE_SIZE = 0x2;

    m_ghostBuffer = buffer;
    m_driftIsAuto = driftIsAuto;

    EGG::RamStream stream = EGG::RamStream(buffer, RKG_UNCOMPRESSED_INPUT_DATA_SECTION_SIZE);

    u16 faceCount = stream.read_u16();
    u16 directionCount = stream.read_u16();
    u16 trickCount = stream.read_u16();

    stream.skip(2);

    m_buttonsStreams[0]->buffer = stream.split(faceCount * SEQUENCE_SIZE);
    m_buttonsStreams[1]->buffer = stream.split(directionCount * SEQUENCE_SIZE);
    m_buttonsStreams[2]->buffer = stream.split(trickCount * SEQUENCE_SIZE);
}

/// @brief Sets whether the controller should accept inputs.
/// @param set `true` to accept inputs, `false` to ignore them.
/// @details Sets @ref m_acceptingInputs accordingly. Effectively, this signals when the
/// countdown has started so that inputs should start being read from the ghost buffer.
void KPadGhostController::setAcceptingInputs(bool set) {
    m_acceptingInputs = set;
}

/// @brief Default constructor
KPadHostController::KPadHostController() = default;

/// @brief Default virtual destructor
KPadHostController::~KPadHostController() = default;

/// @copybrief KPadController::controlSource()
/// @return The source of the control input, @ref ControlSource::Host
ControlSource KPadHostController::controlSource() const {
    return ControlSource::Host;
}

/// @copybrief KPadController::reset()
/// @param driftIsAuto Indicates whether the controller should be set to auto drift mode.
/// @details Sets @ref m_driftIsAuto based on the provided boolean. Resets @ref m_raceInputState
/// and sets @ref m_connected to `true`.
void KPadHostController::reset(bool driftIsAuto) {
    m_driftIsAuto = driftIsAuto;
    m_raceInputState.reset();
    m_connected = true;
}

/// @brief Sets the inputs of the controller.
/// @param state The input state to set
/// @return `true` if the input state is valid, `false` otherwise
bool KPadHostController::setInputs(const RaceInputState &state) {
    return setInputs(state.buttons, state.stick, state.trick);
}

/// @brief Sets the inputs of the controller.
/// @param buttons The button inputs.
/// @param stick The stick inputs, as a 2D vector.
/// @param trick The trick input.
/// @return `true` if the input state is valid, `false` otherwise
bool KPadHostController::setInputs(u16 buttons, const EGG::Vector2f &stick, Trick trick) {
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
/// @return `true` if the input state is valid, `false` otherwise
bool KPadHostController::setInputs(u16 buttons, f32 stickX, f32 stickY, Trick trick) {
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
/// @return `true` if the input state is valid, `false` otherwise
bool KPadHostController::setInputsRawStick(u16 buttons, u8 stickXRaw, u8 stickYRaw, Trick trick) {
    return setInputs(buttons, RawStickToState(stickXRaw), RawStickToState(stickYRaw), trick);
}

/// @brief Sets the inputs of the controller.
/// @details A different name is specified to avoid any ambiguity with the parameters.
/// @param buttons The button inputs.
/// @param stickXRaw The 0-centered raw stick input on the X axis.
/// @param stickYRaw The 0-centered raw stick input on the Y axis.
/// @param trick The trick input.
/// @return `true` if the input state is valid, `false` otherwise
bool KPadHostController::setInputsRawStickZeroCenter(u16 buttons, s8 stickXRaw, s8 stickYRaw,
        Trick trick) {
    return setInputsRawStick(buttons, stickXRaw + 7, stickYRaw + 7, trick);
}

/// @addr{0x80520F64}
/// @brief Constructor
/// @details Initializes the controller pointer to `nullptr` and resets the input state.
KPad::KPad() : m_controller(nullptr) {
    reset();
}

/// @addr{0x805222B4}
/// @brief Default virtual destructor
KPad::~KPad() = default;

/// @addr{0x80521198}
/// @brief Updates @ref m_lastInputState and @ref m_currentInputState to reflect the state of
/// the controller
void KPad::calc() {
    m_lastInputState = m_currentInputState;
    m_currentInputState = m_controller->raceInputState();
}

/// @addr{0x80521110}
/// @brief Resets the controller and its input state
void KPad::reset() {
    if (m_controller) {
        m_controller->reset(m_controller->driftIsAuto());
    }
}

/// @brief Gets the current input state of the pad
/// @return The current input state of the pad, @ref m_currentInputState
const RaceInputState &KPad::currentState() const {
    return m_currentInputState;
}

/// @brief Gets the last input state of the pad
/// @return The last input state of the pad, @ref m_lastInputState
const RaceInputState &KPad::lastState() const {
    return m_lastInputState;
}

/// @brief Checks if the drift is set to automatic
/// @return `true` if the drift is automatic, `false` otherwise
bool KPad::driftIsAuto() const {
    return m_controller->driftIsAuto();
}

/// @addr{0x805220BC}
/// @brief Default constructor
KPadPlayer::KPadPlayer() = default;

/// @addr{0x805222F4}
/// @brief Default virtual destructor
KPadPlayer::~KPadPlayer() = default;

/// @addr{0x80521844}
/// @brief Sets the ghost controller and copies input data into the ghost buffer
/// @param controller Pointer to the ghost controller to drive this pad
/// @param inputs Pointer to the input data to copy into the ghost buffer
/// @param driftIsAuto Whether the drift is set to automatic
void KPadPlayer::setGhostController(KPadGhostController *controller, const u8 *inputs,
        bool driftIsAuto) {
    m_controller = controller;

    if (inputs) {
        memcpy(m_ghostBuffer, inputs, RKG_UNCOMPRESSED_INPUT_DATA_SECTION_SIZE);
    }

    controller->readGhostBuffer(m_ghostBuffer, driftIsAuto);
}

/// @brief Sets the host controller to drive this pad
/// @param controller Pointer to the host controller to drive this pad
/// @param driftIsAuto Whether the drift is set to automatic
void KPadPlayer::setHostController(KPadHostController *controller, bool driftIsAuto) {
    m_controller = controller;
    m_controller->setDriftIsAuto(driftIsAuto);
}

/// @addr{0x805215D4}
/// @brief Starts the ghost proxy, allowing the ghost controller to accept inputs
/// @details Effectively, this signals when the countdown has started so that inputs should
/// start being read from the ghost buffer.
void KPadPlayer::startGhostProxy() {
    if (!m_controller || m_controller->controlSource() != ControlSource::Ghost) {
        return;
    }

    KPadGhostController *ghostController = reinterpret_cast<KPadGhostController *>(m_controller);
    ghostController->setAcceptingInputs(true);
}

/// @addr{0x80521688}
/// @brief Ends the ghost proxy, preventing the ghost controller from accepting inputs
/// @details Effectively, this signals when the race has ended.
void KPadPlayer::endGhostProxy() {
    if (!m_controller || m_controller->controlSource() != ControlSource::Ghost) {
        return;
    }

    KPadGhostController *ghostController = reinterpret_cast<KPadGhostController *>(m_controller);
    ghostController->setAcceptingInputs(false);
}

} // namespace Kinoko::System
