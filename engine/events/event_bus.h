#pragma once

#include "events/event.h"
#include "core/types.h"

#include <functional>
#include <vector>
#include <unordered_map>
#include <typeindex>
#include <any>

namespace kairo {

// type-safe event bus for decoupled system communication
// systems can publish events without knowing who listens,
// and subscribe without knowing who publishes
class EventBus {
public:
    // subscribe to an event type
    // returns a subscription ID that can be used to unsubscribe
    template<typename T>
    u32 subscribe(std::function<void(const T&)> callback) {
        auto type = get_event_id<T>();
        u32 id = m_next_id++;

        m_handlers[type].push_back({
            id,
            [cb = std::move(callback)](const std::any& event) {
                cb(std::any_cast<const T&>(event));
            }
        });

        return id;
    }

    // unsubscribe by ID
    void unsubscribe(u32 id) {
        for (auto& [type, handlers] : m_handlers) {
            handlers.erase(
                std::remove_if(handlers.begin(), handlers.end(),
                    [id](const Handler& h) { return h.id == id; }),
                handlers.end()
            );
        }
    }

    // publish an event — all subscribers of this type get notified immediately
    template<typename T>
    void publish(const T& event) {
        auto type = get_event_id<T>();
        auto it = m_handlers.find(type);
        if (it == m_handlers.end()) return;

        for (auto& handler : it->second) {
            handler.callback(event);
        }
    }

    // queue an event for deferred processing (call flush() later)
    template<typename T>
    void queue(const T& event) {
        auto type = get_event_id<T>();
        m_queued.push_back({ type, event });
    }

    // process all queued events
    void flush() {
        // process in order, allowing events to queue more events
        // use index-based loop since the vector may grow during iteration
        for (size_t i = 0; i < m_queued.size(); i++) {
            auto& queued = m_queued[i];
            auto it = m_handlers.find(queued.type);
            if (it != m_handlers.end()) {
                for (auto& handler : it->second) {
                    handler.callback(queued.event);
                }
            }
        }
        m_queued.clear();
    }

    // remove all handlers
    void clear() {
        m_handlers.clear();
        m_queued.clear();
    }

private:
    struct Handler {
        u32 id;
        std::function<void(const std::any&)> callback;
    };

    struct QueuedEvent {
        std::type_index type;
        std::any event;
    };

    std::unordered_map<std::type_index, std::vector<Handler>> m_handlers;
    std::vector<QueuedEvent> m_queued;
    u32 m_next_id = 1;
};

} // namespace kairo
