#pragma once

#include <functional>
#include <vector>

// simple straightforward event
template<typename ...ArgsT>
class CallbackList {
public:
    typedef std::function<void (ArgsT...)> callback_t;
    std::vector<callback_t> callbacks;

    void emit(ArgsT... args) const {
        if (callbacks.empty()) return;
        for (auto it = callbacks.begin(), end = callbacks.end(); it != end; ++it) {
            (*it)(args...);
        }
    }

    inline void add(callback_t cb) {
        callbacks.emplace_back(cb);
    }

    void remove(callback_t cb) {
        auto it = std::find(callbacks.begin(), callbacks.end(), cb);
        if (it != callbacks.end()) {
            callbacks.erase(it);
        }
    }

    inline bool is_empty() {
        return callbacks.empty();
    }

    inline CallbackList() {
        callbacks.reserve(6);
    }
};
