#pragma once

#include "core/types.h"
#include <string>
#include <unordered_map>
#include <functional>
#include <vector>

namespace kairo {

// generic finite state machine — usable for AI states, game states, menus, etc.
template<typename StateID>
class StateMachine {
public:
    struct State {
        StateID id;
        std::function<void()> on_enter;
        std::function<void()> on_exit;
        std::function<void(float dt)> on_update;
    };

    struct Transition {
        StateID from;
        StateID to;
        std::function<bool()> condition;
    };

    void add_state(StateID id,
                   std::function<void()> on_enter = nullptr,
                   std::function<void(float)> on_update = nullptr,
                   std::function<void()> on_exit = nullptr);

    void add_transition(StateID from, StateID to, std::function<bool()> condition);

    void set_state(StateID id);
    void update(float dt);

    StateID get_current() const { return m_current; }
    bool is_in_state(StateID id) const { return m_current == id; }

private:
    std::unordered_map<StateID, State> m_states;
    std::vector<Transition> m_transitions;
    StateID m_current{};
    bool m_initialized = false;
};

// --- implementation ---

template<typename StateID>
void StateMachine<StateID>::add_state(StateID id, std::function<void()> on_enter,
                                      std::function<void(float)> on_update,
                                      std::function<void()> on_exit) {
    m_states[id] = { id, std::move(on_enter), std::move(on_exit), std::move(on_update) };
}

template<typename StateID>
void StateMachine<StateID>::add_transition(StateID from, StateID to, std::function<bool()> condition) {
    m_transitions.push_back({ from, to, std::move(condition) });
}

template<typename StateID>
void StateMachine<StateID>::set_state(StateID id) {
    // exit the current state if we're already running
    if (m_initialized) {
        auto it = m_states.find(m_current);
        if (it != m_states.end() && it->second.on_exit) it->second.on_exit();
    }

    m_current = id;
    m_initialized = true;

    auto it = m_states.find(id);
    if (it != m_states.end() && it->second.on_enter) it->second.on_enter();
}

template<typename StateID>
void StateMachine<StateID>::update(float dt) {
    if (!m_initialized) return;

    // evaluate transitions from the current state
    for (auto& t : m_transitions) {
        if (t.from == m_current && t.condition && t.condition()) {
            set_state(t.to);
            return; // skip update on the frame a transition fires
        }
    }

    auto it = m_states.find(m_current);
    if (it != m_states.end() && it->second.on_update) {
        it->second.on_update(dt);
    }
}

} // namespace kairo
