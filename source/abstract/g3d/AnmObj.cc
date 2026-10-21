#include "AnmObj.hh"

#include <cmath>

namespace Kinoko::Abstract::g3d {

/// @addr{0x800604F0}
/// @brief Play policy for an animation that plays once
/// @param frame The current frame of the animation
/// @return Always `frame`
f32 PlayPolicy_Onetime(f32 /*start*/, f32 /*end*/, f32 frame) {
    return frame;
}

/// @addr{0x80060500}
/// @brief Play policy for an animation that loops continuously
/// @param start The start frame of the animation
/// @param end The end frame of the animation
/// @param frame The current frame of the animation
/// @return The adjusted frame within the looped range
f32 PlayPolicy_Loop(f32 start, f32 end, f32 frame) {
    ASSERT(end > start);

    f32 length = end - start;

    if (frame >= 0.0f) {
        return ::fmodf(frame, length);
    }

    f32 offset = ::fmodf(frame + length, length);

    return offset >= 0.0f ? offset : offset + length;
}

/// @brief Constructor
/// @param start The start frame of the animation
/// @param end The end frame of the animation
/// @param policy The play policy function for the animation
/// @details Initialzies @ref m_frame to zero and @ref m_updateRate to `1.0f`. Sets @ref
/// m_startFrame to `start`, @ref m_endFrame to `end`, and @ref m_playPolicy to `policy`.
FrameCtrl::FrameCtrl(f32 start, f32 end, PlayPolicyFunc policy)
    : m_frame(0.0f),
      m_updateRate(1.0f),
      m_startFrame(start),
      m_endFrame(end),
      m_playPolicy(policy) {
    ASSERT(m_playPolicy);
}

/// @brief Default destructor
FrameCtrl::~FrameCtrl() = default;

/// @brief Advances the animation frame
/// @details The animation frame is incremented by @ref m_updateRate times @ref s_baseUpdateRate.
void FrameCtrl::updateFrame() {
    setFrame(m_updateRate * s_baseUpdateRate + m_frame);
}

/// @brief Gets the current frame of the animation
/// @return The current frame of the animation, @ref m_frame
f32 FrameCtrl::frame() const {
    return m_frame;
}

/// @brief Gets the update rate (playback speed) of the animation
/// @return The update rate, @ref m_updateRate
f32 FrameCtrl::rate() const {
    return m_updateRate;
}

/// @brief Gets the play policy function of the animation
/// @return The play policy function, @ref m_playPolicy
PlayPolicyFunc FrameCtrl::playPolicy() const {
    return m_playPolicy;
}

/// @brief Sets the animation frame based on the provided `frame
/// @param frame The new frame to set for the animation
/// @details The frame is adjusted according to the current play policy function. For looping
/// animations, the frame will wrap around within the @ref m_startFrame and @ref m_endFrame range.
void FrameCtrl::setFrame(f32 frame) {
    m_frame = m_playPolicy(m_startFrame, m_endFrame, frame);
}

/// @brief Sets the update rate (playback speed) of the animation
/// @param rate The new update rate to set
void FrameCtrl::setRate(f32 rate) {
    m_updateRate = rate;
}

/// @brief Sets the play policy function for the animation
/// @param func The new play policy function to set
void FrameCtrl::setPlayPolicy(PlayPolicyFunc func) {
    ASSERT(func);
    m_playPolicy = func;
}

f32 FrameCtrl::s_baseUpdateRate = 1.0f;

} // namespace Kinoko::Abstract::g3d
