#include "./binary_semaphore.hpp"

#if _HAS_CXX20

#else

binary_semaphore::binary_semaphore(const size_t& desired) : desired(desired), avail(desired) { }
binary_semaphore::binary_semaphore(const binary_semaphore& s) : desired(s.desired), avail(s.avail) { }

void binary_semaphore::acquire() {
    std::unique_lock<std::mutex> lk(m);
    cv.wait(lk, [this] { return avail > 0; });
    avail--;
    lk.unlock();
}

void binary_semaphore::release() {
    m.lock();
    avail++;
    m.unlock();
    cv.notify_one();
}

#endif