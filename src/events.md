# Events

Attention!!  
Sender `(void*)0` is always "global" target, which will be processed after processing individual targets

Events will be ordered as it was emitted

```cpp
// sender structure
// usually its pointer to owner of EventEmitter, like widget class
struct Sender { int id; };

// some abstract sender
auto sender1 = Sender{ .id= 1 };
auto sender2 = Sender{ .id= 2 };

// our event types
struct Event1 { int x; };
struct Event2 { int y; };

// Global or scoped listener
// It should live more than emitters
EventListener listener;

// Every emitter should be connected to some Listener
EventEmitter<Sender, Event1, Event2> emitter(&listener);

// register listeners for specific senders
// usually you will register for same sender
emitter.add_listener<Event2>(&sender2, [](Event2 const& e1, auto sender) {
    std::cout << "event2 fired " << e1.y << " for sender=" << sender->id << std::endl;
});

emitter.add_listener<Event1>(&sender1, [](Event1 const& e1, auto sender) {
    std::cout << "event1 fired " << e1.x << " for sender=" << sender->id << std::endl;
});

// emit events
emitter.emit(&sender1, Event1 {
    .x = 333
});
emitter.emit(&sender2, Event2 {
    .y = 444
});

// here we process all emitted events
listener.flush_events();
```

output:

```
event1 fired 333 for sender=1
event2 fired 444 for sender=2
```

# With class helper

```cpp

struct Event1 { int x; };
struct Event2 { int y; };

class Emitter : public EventEmitter_Base<Emitter, Event1, Event2> {
public:
	std::string id;

	Emitter(EventListener* listener, std::string id) : EventEmitter_Base(listener), id(id) {}
};


int main(int argc, char **argv)
{
	EventListener listener;
	Emitter emitter1(&listener, "hello");
	Emitter emitter2(&listener, "world");

    // now sender will be this Emitter

	emitter1.emit(Event1 {
		.x = 10
	});
	emitter2.emit(Event2 {
		.y = 20
	});

	emitter1.add_listener<Event1>([](Event1 const& e1, auto sender) {
		std::cout << "event1 fired " << e1.x << " for sender=" << sender->id << std::endl;
	});

	emitter2.add_listener<Event2>([](Event2 const& e1, auto sender) {
		std::cout << "event2 fired " << e1.y << " for sender=" << sender->id << std::endl;
	});

	listener.flush_events();

    return 0;
}

```

output:

```
event1 fired 10 for sender=hello
event2 fired 20 for sender=world
```

# Event emitting modes

- `EVENT_EMIT_PUSH`
  By default you should emit all events from same thread, where `EventListener` was created.  
  Also all emitted event will be processed in same order as it was emitted.  
  Its `EVENT_EMIT_PUSH` mode that is set by default.

- `EVENT_EMIT_DEBOUNCE`
  Events with this flag will be debounced on emitting; only last emitted event will remain in event queue and processed.  
  For async events, debounce will work only for async queue.

- `EVENT_EMIT_ASYNC`
  Without this flag, any emitment not from `EventListener's` thread will rise exception.  
  Emitting event with this flag will queue events in different slower async queue, but with sync.

# "run smth on flush" utility

Simplest way to sync some external thread with main is special `SyncEvent`:

```cpp
emitter.emit(SyncEvent([]() {
	// here we are synced
}), EVENT_EMIT_ASYNC);
```

Its special event type that handles some function that will be executed in events processor

Order is same as with any other event

# "events_app" utils

In current app, there are global `EventListener` and multiple emitters

To implement emitter object:

```cpp

struct MyEmitter_Event1 {};
struct MyEmitter_Event2 {};

class MyEmitter : public EventEmitterApp_Base {
public:
	IMPL_EVENTS(MyEmitter_Event1, MyEmitter_Event2)
};
```

`IMPL_EVENTS` macro will implement `emit / add_listener` methods for specified events  
`EventEmitterApp_Base` has implementation and connection to App's EventListener

So then you could just:

```cpp
myEmitter->emit(MyEmitter_Event2{});
```

And it will be sent from `myEmitter` as sender to App's EventListener.
