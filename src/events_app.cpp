#include "./events_app.hpp"

#include "app/app.hpp"

EventListener* EventEmitterApp_Base_get_app_listener() {
    return &Application::instance()->event_listener;
}

void EventEmitterApp_add_frame_listener(std::function<void (App_OnFrame const&)>&& listener) {
    EventEmitter_Impl::add_listener<App_OnFrame>(0, EventEmitterApp_Base_get_app_listener(), std::move(listener), EVENT_LISTENER_MODE::EVENT_LISTENER_NORMAL);
}