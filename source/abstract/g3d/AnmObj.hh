#pragma once

#include <Common.hh>

// Credit: kiwi515/ogws

namespace Kinoko {

namespace Host {

class Context;

} // namespace Host

/// @brief Contains logic pertaining to drawing 3D graphics
namespace Abstract::g3d {

/// @brief Describes whether the animation should play once or loop
enum class AnmPolicy : u32 {
    OneTime = 0, ///< Play the animation once
    Loop = 1,    ///< Loop the animation continuously
    Max = 2,     ///< Maximum number of animation policies
};

[[nodiscard]] f32 PlayPolicy_Onetime(f32 start, f32 end, f32 frame);
[[nodiscard]] f32 PlayPolicy_Loop(f32 start, f32 end, f32 frame);

/// @brief Function pointer type for animation play policies
/// @param start The start frame of the animation
/// @param end The end frame of the animation
/// @param frame The current frame of the animation
/// @return The adjusted frame based on the play policy
typedef f32 (*PlayPolicyFunc)(f32 start, f32 end, f32 frame);

/// @brief Fetches the appropriate @ref PlayPolicyFunc for the given @ref AnmPolicy
/// @param policy The animation policy
/// @return The associated @ref PlayPolicyFunc
[[nodiscard]] inline PlayPolicyFunc GetAnmPlayPolicy(AnmPolicy policy) {
    constexpr size_t count = static_cast<size_t>(AnmPolicy::Max);
    static constexpr PlayPolicyFunc POLICY_TABLE[count] = {
            PlayPolicy_Onetime,
            PlayPolicy_Loop,
    };

    size_t idx = static_cast<size_t>(policy);
    ASSERT(idx < count);
    return POLICY_TABLE[idx];
}

/// @brief Represents an animation frame/playback controller
/// @details This class tracks the current frame of the animation, the update rate (playback speed),
/// and the @ref AnmPolicy.
class FrameCtrl {
    /// @brief Grants access to @ref s_baseUpdateRate so a @ref Host::Context can restore its value
    /// on context switch
    friend Host::Context;

public:
    FrameCtrl(f32 start, f32 end, PlayPolicyFunc policy);
    ~FrameCtrl();

    void updateFrame();

    /// @beginGetters

    [[nodiscard]] f32 frame() const;
    [[nodiscard]] f32 rate() const;
    [[nodiscard]] PlayPolicyFunc playPolicy() const;

    /// @endGetters

    /// @beginSetters

    void setFrame(f32 frame);
    void setRate(f32 rate);
    void setPlayPolicy(PlayPolicyFunc func);

    /// @endSetters

private:
    f32 m_frame;                 ///< The current frame in the animation
    f32 m_updateRate;            ///< The update rate (playback speed) of the animation
    const f32 m_startFrame;      ///< The start frame of the animation
    const f32 m_endFrame;        ///< The end frame of the animation
    PlayPolicyFunc m_playPolicy; ///< The play policy function for the animation

    static f32 s_baseUpdateRate; ///< Global base animation playback speed
};

} // namespace Abstract::g3d

} // namespace Kinoko
