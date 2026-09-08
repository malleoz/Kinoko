#include "KPadController.hh"

namespace Kinoko::System {

/// @brief Default constructor
KPadGhostButtonsStream::KPadGhostButtonsStream()
    : currentSequence(std::numeric_limits<u16>::max()),
      state(2) {}

/// @brief Default virtual destructor
KPadGhostButtonsStream::~KPadGhostButtonsStream() = default;

/// @addr{0x80520D4C} @addr{0x80522C5C} @addr{0x80522F40}
/// @brief Reads the data from the corresponding tuple in the buffer.
/// @return The input value for the current frame.
/// @details In the base game, this is a virtual function, but no derived class overrides this
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

/// @addr{0x80520730}
/// @brief Default constructor that creates streams for the buttons, trick buttons, and analog stick
KPadGhostController::KPadGhostController() : m_acceptingInputs(false) {
    m_buttonsStreams[0] = EGG::egg_new<KPadGhostFaceButtonsStream>();
    m_buttonsStreams[1] = EGG::egg_new<KPadGhostDirectionButtonsStream>();
    m_buttonsStreams[2] = EGG::egg_new<KPadGhostTrickButtonsStream>();
}

/// @addr{0x80520924}
/// @brief Default virtual destructor
KPadGhostController::~KPadGhostController() = default;

/// @addr{0x80520998}
/// @copydoc KPadController::reset()
/// @param driftIsAuto Indicates whether the controller should be set to auto drift mode.
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

/// @addr{Inlined in 0x80521844}
/// @brief Splits the ghost input data sections into their associated button streams
/// @param buffer The uncompressed input data buffer from the ghost RKG file.
/// @param driftIsAuto True for auto transmission, false for manual.
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

/// @addr{0x80520B9C}
/// @brief Fetches all input data from the ghost buffer streams and updates the race input state
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

} // namespace Kinoko::System
