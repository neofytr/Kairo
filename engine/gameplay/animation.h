#pragma once
#include "core/types.h"
#include <string>
#include <unordered_map>
#include <vector>
#include <functional>

namespace kairo {

// a single animation clip — sequence of frame indices with timing
struct AnimationClip {
    std::string name;
    std::vector<i32> frames;       // indices into a sprite sheet
    float frame_duration = 0.1f;   // seconds per frame
    bool looping = true;
};

// transition rule between states
struct AnimationTransition {
    std::string from_state;
    std::string to_state;
    std::function<bool()> condition;  // fires when condition returns true
};

// animation state machine — manages clips, transitions, and current frame
class AnimationStateMachine {
public:
    // register an animation clip as a state
    void add_state(const AnimationClip& clip);

    // add a transition between states
    void add_transition(const std::string& from, const std::string& to, std::function<bool()> condition);

    // set the initial/current state
    void set_state(const std::string& name);

    // update — advances frame timer and checks transitions
    void update(float dt);

    // get current frame index (into sprite sheet)
    i32 get_current_frame() const;

    // get current state name
    const std::string& get_current_state() const;

    // check if current animation just finished (for non-looping clips)
    bool is_finished() const;

    // reset current animation to frame 0
    void reset();

private:
    std::unordered_map<std::string, AnimationClip> m_states;
    std::vector<AnimationTransition> m_transitions;

    std::string m_current_state;
    float m_elapsed = 0.0f;
    i32 m_current_frame_index = 0;  // index within the clip's frames array
    bool m_finished = false;
};

// ecs component that attaches an animation state machine to an entity
struct AnimatorComponent {
    AnimationStateMachine state_machine;
};

} // namespace kairo
