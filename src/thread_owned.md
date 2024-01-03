# THREAD_OWNED_STRUCT

Make struct that is accessible for write only from owner thread

Data is cache line aligned (64 bytes)

Example

```cpp

THREAD_OWNED_STRUCT(Data,
    std::string str1;
    float f2;
)

Data test_data;

// in thread1
test_data.set_owner(thread2);

// in thread1
test_data.access_read().f2; // ok
test_data.access_write().f2; // exception

// in thread2
test_data.access_read().f2; // ok
test_data.access_write().f2; // ok

```