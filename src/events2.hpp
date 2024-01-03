#pragma once

#include "./buf.hpp"
#include "./utils.hpp"
#include "./type_hash.hpp"
#include "./no_inline.hpp"

#include "./_globals.hpp"

#include <memory>
#include <mutex>
#include <stdint.h>
#include <robin_hood.h>
#include <stdio.h>
#include <variant>
#include <thread>
#include <vector>

// TODO: 2? some event handlign context to make possible to bulk events in add_listener instead of processing events one by one

#ifndef DEBUG_Events
#define DEBUG_Events 0
#endif

#if DEBUG_Events == 1
#include <iostream>
#endif

class EventListener;

enum EVENT_LISTENER_MODE : uint8_t {
    EVENT_LISTENER_NORMAL = 0,
    // EVENT_LISTENER_ONCE = 1, use EVENT_EMIT_DEBOUNCE instead
};

enum EVENT_EMIT_MODE : uint8_t {
    // push every emitted event to processing queue
    EVENT_EMIT_PUSH = 0,
    // only last event will be emitted
    EVENT_EMIT_DEBOUNCE = 1,
    // events not with this flag, not from main thread will throw exception
    EVENT_EMIT_ASYNC = 2,
};

// special event type that calls function on processing
// better use EVENT_EMIT_ASYNC if possible
struct SyncEvent {
    bool executed = false;
    std::function<void ()> call_me;

    SyncEvent() = default;

    inline SyncEvent(std::function<void ()>&& func) noexcept : call_me([f = std::move(func)]() {
        f();
    }) {}
};

namespace {
    // do not use `this` point of object, coz deleted objects may overlay by address with existing
    typedef size_t event_sender_t;

    struct EventPayload {
        event_sender_t sender;
        size_t event_type;
        std::shared_ptr<void> boxed;

        template<typename T>
        inline T* as() const noexcept { return (T*)boxed.get(); }

        #ifdef EVENTS_WITH_DEBUG_ID
        size_t debug_id = 0;
        #endif
    };

    typedef std::function<void (EventPayload const& payload, event_sender_t const& sender)> EventTarget_Listener;
}

struct EventEmitter_Impl {
    inline static void emit_event_mem(event_sender_t sender, EventListener* listener, size_t event_type, std::shared_ptr<void>&& boxed, EVENT_EMIT_MODE emit_mode);
    inline static void add_event_listener(event_sender_t sender, EventListener* listener, EventTarget_Listener&& handler, EVENT_LISTENER_MODE mode);

    template<typename EventT>
    inline static void emit(event_sender_t sender, EventListener* listener, EventT const& event, EVENT_EMIT_MODE emit_mode);

    template<typename EventT, typename Func>
    inline static void add_listener(event_sender_t sender, EventListener* listener, Func&& handler, EVENT_LISTENER_MODE listener_mode);
};

template<typename ...EventTypesT>
struct EventEmitter
{
    template<typename EventT>
    NO_INLINE void emit(EventT const& event, EVENT_EMIT_MODE emit_mode = EVENT_EMIT_MODE::EVENT_EMIT_PUSH) {
        static_assert(( false || ... || std::is_same<EventT, EventTypesT>::value ), "EventEmitter<EventTypesT...>::emit<EventT> compilation failed; EventT should be one of EventTypesT");
        EventEmitter_Impl::emit<EventT>(_uid, _listener, event);
    }

    template<typename EventT, typename Func>
    NO_INLINE void add_listener(Func&& handler, EVENT_LISTENER_MODE listener_mode = EVENT_LISTENER_MODE::EVENT_LISTENER_NORMAL) {
        static_assert(( false || ... || std::is_same<EventT, EventTypesT>::value ), "EventEmitter<EventTypesT...>::add_listener<EventT> compilation failed; EventT should be one of EventTypesT");
        EventEmitter_Impl::add_listener<EventT, Func>(_uid, _listener, std::move(handler), listener_mode);
    }

    EventEmitter() noexcept;
    inline EventEmitter(EventListener* listener) noexcept : _listener(listener) {}
protected:
    size_t _uid;
    EventListener* _listener;
};

class EventListener {
    friend struct EventEmitter_Impl;
    friend class WBase;
    
    static size_t _global_uid;
public:
    void flush_events();
    inline static size_t next_global_uid() noexcept { return ++_global_uid; }
    inline std::thread::id get_main_thread_id() const { return _main_thread_id; }
    void remove_all_events_by_type(event_sender_t sender, size_t event_type);

    template<typename EventT>
    inline constexpr static size_t get_event_type() {
        return type_hash<EventT>();
    }

    EventListener();
    ~EventListener();

protected:
    struct EventTarget {
        std::vector<EventTarget_Listener> listeners;
        std::vector<EventTarget_Listener> once_listeners;
    };

    void _recv_emitted_event(event_sender_t sender, size_t event_type, std::shared_ptr<void>&& boxed, EVENT_EMIT_MODE emit_mode);

    inline void _add_listener(event_sender_t sender, EventTarget_Listener&& listener, EVENT_LISTENER_MODE listener_mode) {
        if (listener_mode == EVENT_LISTENER_MODE::EVENT_LISTENER_NORMAL) _event_targets[sender].listeners.emplace_back(listener);
        else _event_targets[sender].once_listeners.emplace_back(listener);
    }

    std::thread::id _main_thread_id;

    std::vector<EventPayload> _events;
    robin_hood::unordered_node_map<event_sender_t, EventTarget> _event_targets;

    std::mutex _async_events_mtx;
    std::vector<EventPayload> _async_events;
};

template<typename ...EventTypesT>
EventEmitter<EventTypesT...>::EventEmitter() noexcept : _uid(EventListener::next_global_uid()) {}

inline void EventEmitter_Impl::emit_event_mem(event_sender_t sender, EventListener* listener, size_t event_type, std::shared_ptr<void>&& boxed, EVENT_EMIT_MODE emit_mode) {
    listener->_recv_emitted_event(sender, event_type, std::move(boxed), emit_mode);
}

inline void EventEmitter_Impl::add_event_listener(event_sender_t sender, EventListener* listener, EventTarget_Listener&& handler, EVENT_LISTENER_MODE listener_mode) {
    listener->_add_listener(sender, std::move(handler), listener_mode);
}

template<typename EventT>
inline void EventEmitter_Impl::emit(event_sender_t sender, EventListener* listener, EventT const& event, EVENT_EMIT_MODE emit_mode) {
    constexpr size_t event_type = EventListener::get_event_type<EventT>();

    // do not allocate for empty structs
    if constexpr (std::is_empty<EventT>::value) {
        EventEmitter_Impl::emit_event_mem(sender, listener, event_type, nullptr, emit_mode);
    } else {
        EventEmitter_Impl::emit_event_mem(sender, listener, event_type, std::make_shared<EventT>(event), emit_mode);
    }
}

template<typename EventT, typename Func>
inline void EventEmitter_Impl::add_listener(event_sender_t sender, EventListener* listener, Func&& handler, EVENT_LISTENER_MODE listener_mode) {
    constexpr size_t event_type = EventListener::get_event_type<EventT>();

    EventEmitter_Impl::add_event_listener(sender, listener, [handler, event_type](EventPayload const& payload, event_sender_t const& sender) {
        if (payload.event_type != event_type) return;
        
        #if DEBUG_Events == 1
            std::cout << "events sender=" << sender << " handle event " << typeid(EventT).name() << " type_hash=" << typeid(EventT).hash_code() << std::endl;
        #endif
        
        if constexpr (std::is_empty<EventT>::value) {
            handler(EventT{});
        } else {
            handler(*(payload.as<EventT>()));
        }
    }, listener_mode);
}