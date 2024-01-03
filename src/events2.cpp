#include "./events2.hpp"

#include <algorithm>
#include <mutex>
#include <stdexcept>
#include "./log.hpp"

// remember, 0 is "global event target"
size_t EventListener::_global_uid = 1;

#ifdef EVENTS_WITH_DEBUG_ID
static size_t __123_events_debug_id_g = 0;
#endif

void _recv_emitted_event_queue(std::vector<EventPayload>& queue, event_sender_t sender, size_t event_type, std::shared_ptr<void>&& boxed, EVENT_EMIT_MODE emit_mode) {
    if (emit_mode == EVENT_EMIT_MODE::EVENT_EMIT_DEBOUNCE) {
        if (!queue.empty()) {
            auto it = std::find_if(queue.begin(), queue.end(), [sender, event_type](auto const& x) {
                return x.sender == sender && x.event_type == event_type;
            });
            if (it != queue.end()) {
                queue.erase(it);
            }
        }
        queue.emplace_back(EventPayload {
            .sender = sender,
            .event_type = event_type,
            .boxed = boxed,
            #ifdef EVENTS_WITH_DEBUG_ID
            .debug_id = ++__123_events_debug_id_g,
            #endif
        });
    }
    // PUSH by default
    else {
        queue.emplace_back(EventPayload {
            .sender = sender,
            .event_type = event_type,
            .boxed = boxed,
            #ifdef EVENTS_WITH_DEBUG_ID
            .debug_id = ++__123_events_debug_id_g,
            #endif
        });
    }
}

void EventListener::_recv_emitted_event(event_sender_t sender, size_t event_type, std::shared_ptr<void>&& boxed, EVENT_EMIT_MODE emit_mode) {
    if (_main_thread_id != std::this_thread::get_id()) {
        if (emit_mode & EVENT_EMIT_ASYNC) {
            std::lock_guard lock(_async_events_mtx);
            _recv_emitted_event_queue(_async_events, sender, event_type, std::move(boxed), emit_mode);
        } else {
            KS_CRIT_LOG_ERROR("call not from main thread without EVENT_EMIT_ASYNC");
            throw std::runtime_error("EventListener::_recv_emitted_event failed call not from main thread without EVENT_EMIT_ASYNC");
        }
    } else {
        _recv_emitted_event_queue(_events, sender, event_type, std::move(boxed), emit_mode);
    }
}

void EventListener::remove_all_events_by_type(event_sender_t sender, size_t event_type) {
    // if (_main_thread_id != std::this_thread::get_id()) {
    //     KS_CRIT_LOG_ERROR("call not from main thread");
    //     throw std::runtime_error("EventListener::remove_all_events_by_type failed call not from main thread");
    // }

    auto newEnd = std::remove_if(_events.begin(), _events.end(), [sender, event_type](auto const& x) {
        return x.sender == sender && x.event_type == event_type;
    });
    _events.erase(newEnd, _events.end());

    {
        std::lock_guard lock(_async_events_mtx);
        auto newEnd = std::remove_if(_async_events.begin(), _async_events.end(), [sender, event_type](auto const& x) {
            return x.sender == sender && x.event_type == event_type;
        });
        _async_events.erase(newEnd, _async_events.end());
    }
}

void EventListener::flush_events() {
    if (!_events.empty() || !_async_events.empty()) {
        // Iterate only like this, coz _events could grow, resize, remove while iterating!!
        std::vector<EventPayload> events;
        events.swap(_events);

        if (!_async_events.empty()) {
            std::lock_guard lock(_async_events_mtx);
            events.reserve(events.size() + _async_events.size());
            for (auto&& e : _async_events) events.emplace_back(std::move(e));
            _async_events.clear();
        }

        int events_len = (int)events.size();

        for (auto const& event : events) {
            if (event.event_type == get_event_type<SyncEvent>()) {
                SyncEvent* se = event.as<SyncEvent>();
                if (!se->executed) {
                    se->executed = true;
                    se->call_me();
                }
                continue;
            }

            auto& target = _event_targets[event.sender];
            if (!target.listeners.empty()) {
                for (auto const& handler : target.listeners) {
                    handler(event, event.sender);
                }
            }

            if (!target.once_listeners.empty()) {
                for (auto const& handler : target.once_listeners) {
                    handler(event, event.sender);
                }
                target.once_listeners.clear();
            }
        }

        auto& global_target = _event_targets[0];
        if (!global_target.listeners.empty()) {
            for (auto const& event : events) {
                if (event.event_type == get_event_type<SyncEvent>()) {
                    SyncEvent* se = event.as<SyncEvent>();
                    if (!se->executed) {
                        se->executed = true;
                        se->call_me();
                    }
                    continue;
                }

                for (auto const& handler : global_target.listeners) {
                    handler(event, event.sender);
                }

                if (global_target.once_listeners.empty()) continue;
                for (auto const& handler : global_target.once_listeners) {
                    handler(event, event.sender);
                }
                global_target.once_listeners.clear();
            }
        }

        events.clear();
    }
}

EventListener::EventListener() : _main_thread_id(std::this_thread::get_id()) {

}

EventListener::~EventListener() {
}