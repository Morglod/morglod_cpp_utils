#include "./thread_owned.hpp"

const std::thread::id _ThreadOwnedBase::no_thread_id;

thread_owned_other_thread_error::thread_owned_other_thread_error(const char* str) : std::runtime_error(str) {};
thread_owned_other_thread_error::thread_owned_other_thread_error(std::string const& str) : std::runtime_error(str) {};