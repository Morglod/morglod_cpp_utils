# Events2

Attention!!  
Sender `(void*)0` is always "global" target, which will be processed after processing individual targets

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
