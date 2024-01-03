#pragma once

#include <stdexcept>
#include <thread>
#include <atomic>
#include <exception>

struct _ThreadOwnedBase {
    static const std::thread::id no_thread_id;
    
    inline bool no_owner_or_self(std::thread::id const& self_id = std::this_thread::get_id()) const {
        const auto owner_id = _owner.load();
        return owner_id == no_thread_id || owner_id == self_id;
    }

    inline void set_owner(std::thread const& thread) {
        _owner = thread.get_id();
    }
protected:
    std::atomic<std::thread::id> _owner;
};

struct thread_owned_other_thread_error : public std::runtime_error {
    thread_owned_other_thread_error(const char* str);
    thread_owned_other_thread_error(std::string const& str);
};

#define THREAD_OWNED_STRUCT(_NAME, _FIELDS) \
    struct alignas(64) _NAME : public _ThreadOwnedBase { \
        using Self = _NAME; \
        struct _Data { \
            friend struct _NAME; \
            _FIELDS \
            _Data(_Data const&) = delete; \
        protected: \
            _Data() = default; \
        }; \
        inline _Data& access_write() { \
            if (no_owner_or_self()) return _data; \
            throw thread_owned_other_thread_error("THREAD_OWNED_STRUCT::access_write failed; not from owner thread; at " __FILE__ " " + std::to_string(__LINE__) ); \
        } \
        inline _Data const& access_read() { \
            return _data; \
        } \
    protected: \
        _Data _data; \
    };
