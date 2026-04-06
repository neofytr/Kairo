#include "gameplay/animation.h"

namespace kairo {

void AnimationStateMachine::add_state(const AnimationClip& clip) {
    m_states[clip.name] = clip;
}

void AnimationStateMachine::add_transition(const std::string& from, const std::string& to, std::function<bool()> condition) {
    m_transitions.push_back({from, to, std::move(condition)});
}

void AnimationStateMachine::set_state(const std::string& name) {
    // ignore unknown states
    if (m_states.find(name) == m_states.end()) return;

    m_current_state = name;
    m_elapsed = 0.0f;
    m_current_frame_index = 0;
    m_finished = false;
}

void AnimationStateMachine::update(float dt) {
    // nothing to do without a valid state
    if (m_current_state.empty()) return;
    auto it = m_states.find(m_current_state);
    if (it == m_states.end()) return;

    const auto& clip = it->second;
    if (clip.frames.empty()) return;

    // advance timer
    m_elapsed += dt;

    // compute frame index from elapsed time
    i32 total_frames = static_cast<i32>(clip.frames.size());
    i32 frame = static_cast<i32>(m_elapsed / clip.frame_duration);

    if (clip.looping) {
        m_current_frame_index = frame % total_frames;
        // keep elapsed from growing unbounded
        if (frame >= total_frames) {
            m_elapsed -= static_cast<float>(total_frames) * clip.frame_duration;
        }
    } else {
        if (frame >= total_frames - 1) {
            m_current_frame_index = total_frames - 1;
            m_finished = true;
        } else {
            m_current_frame_index = frame;
        }
    }

    // check transitions from current state
    for (const auto& t : m_transitions) {
        if (t.from_state == m_current_state && t.condition && t.condition()) {
            set_state(t.to_state);
            return; // state changed, skip remaining transitions
        }
    }
}

i32 AnimationStateMachine::get_current_frame() const {
    if (m_current_state.empty()) return 0;
    auto it = m_states.find(m_current_state);
    if (it == m_states.end() || it->second.frames.empty()) return 0;

    return it->second.frames[m_current_frame_index];
}

const std::string& AnimationStateMachine::get_current_state() const {
    return m_current_state;
}

bool AnimationStateMachine::is_finished() const {
    return m_finished;
}

void AnimationStateMachine::reset() {
    m_elapsed = 0.0f;
    m_current_frame_index = 0;
    m_finished = false;
}

} // namespace kairo
