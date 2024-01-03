#pragma once

#include "./events2.hpp"
#include "./utils.hpp"
#include "./macro_map.hpp"
#include "./no_inline.hpp"

#include <atomic>
#include <type_traits>

class Application;
struct App_OnFrame {};

EventListener* EventEmitterApp_Base_get_app_listener();
void EventEmitterApp_add_frame_listener(std::function<void (App_OnFrame const&)>&& listener);

class EventEmitterApp_Base {
    size_t _uid;
public:
    template<typename EventT>
    NO_INLINE void _event_emit(EventT const& event, EVENT_EMIT_MODE emit_mode = EVENT_EMIT_MODE::EVENT_EMIT_PUSH) {
        #if DEBUG_Events == 1
            std::cout << "events " << typeid(*this).name() << "(" << this << ")::emit event " << typeid(EventT).name() << " type_hash=" << typeid(EventT).hash_code() << std::endl;
        #endif

        EventEmitter_Impl::emit<EventT>(_uid, EventEmitterApp_Base_get_app_listener(), event, emit_mode);
    }

    template<typename EventT, typename Func>
    inline void _event_add_listener(Func&& handler, EVENT_LISTENER_MODE listener_mode = EVENT_LISTENER_MODE::EVENT_LISTENER_NORMAL) {
        #if DEBUG_Events == 1
            std::cout << "events " << typeid(*this).name() << "(" << this << ")::add_listener for event " << typeid(EventT).name() << " type_hash=" << typeid(EventT).hash_code() << std::endl;
        #endif

        EventEmitter_Impl::add_listener<EventT, Func>(_uid, EventEmitterApp_Base_get_app_listener(), std::move(handler), listener_mode);
    }

    inline EventEmitterApp_Base() noexcept {
        _uid = EventListener::next_global_uid();
    }
};

#define IMPLEMENT_SINGLE_EVENT( EventT ) \
    inline void emit(EventT const& e, EVENT_EMIT_MODE emit_mode = EVENT_EMIT_MODE::EVENT_EMIT_PUSH) { _event_emit<EventT>(e, emit_mode); } \
    template<> inline void add_listener< EventT >(std::function<void ( EventT )>&& handler, EVENT_LISTENER_MODE listener_mode) { \
        _event_add_listener<EventT, std::function<void ( EventT )>>(std::move(handler), listener_mode); \
    }

#define IMPL_EVENTS(...) \
    template<typename EventT> void add_listener(std::function<void (EventT)>&& handler, EVENT_LISTENER_MODE listener_mode = EVENT_LISTENER_MODE::EVENT_LISTENER_NORMAL); \
    MACRO_MAP(IMPLEMENT_SINGLE_EVENT, __VA_ARGS__) \
    IMPLEMENT_SINGLE_EVENT(SyncEvent)

// lightweight variant of SyncEvent
// better use EVENT_EMIT_ASYNC if possible
template<typename EmitEventT>
class AppFrameSync {
public:
    std::atomic<bool> _should_emit = false;
    EmitEventT _event;

    inline void set() {
        _should_emit = true;
    }

    inline void set(EmitEventT&& evt) {
        _event = evt;
        _should_emit = true;
    }

    AppFrameSync() = delete;

    template<typename SelfT>
    AppFrameSync(SelfT* self) {
        EventEmitterApp_add_frame_listener([this, self](auto) {
            if (this->_should_emit) {
                this->_should_emit = false;
                self->emit(this->_event, EVENT_EMIT_MODE::EVENT_EMIT_DEBOUNCE);
            }
        });
    }
};
