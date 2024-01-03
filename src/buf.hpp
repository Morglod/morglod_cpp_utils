#pragma once

#include <stdint.h>
#include <memory>

// not tested

// use it as foo(BufSlice buf)
// without const&
// best iterate direct with buf.data

struct _BufSlice {
    uint8_t* _mem = nullptr;
    size_t _size = 0;

    constexpr inline uint8_t* mem() const {
        return _mem;
    }

    constexpr inline size_t size() const {
        return _size;
    }

    constexpr _BufSlice() = default;
    inline constexpr _BufSlice(uint8_t* m, size_t s) noexcept : _mem(m), _size(s) {}

    template<typename T>
    constexpr T* as() noexcept {
        return (T*)_mem;
    }

    template<typename T>
    constexpr T* as_const() const noexcept {
        return (T*)_mem;
    }

    constexpr inline uint8_t* cbegin() const noexcept {
        return _mem;
    }

    constexpr inline uint8_t* cend() const noexcept {
        return _mem + _size;
    }

    constexpr inline uint8_t* begin() noexcept {
        return _mem;
    }

    constexpr inline uint8_t* end() noexcept {
        return _mem + _size;
    }

    constexpr inline _BufSlice slice(size_t const offset) const noexcept {
        return _BufSlice(_mem + offset, _size - offset);
    }

    constexpr inline _BufSlice unsafe_slice(size_t const offset, size_t const len) const noexcept {
        return _BufSlice(_mem + offset, len);
    }

    constexpr inline _BufSlice slice(uint8_t* const begin) const noexcept {
        return _BufSlice(begin, _size - (begin - _mem));
    }

    constexpr inline _BufSlice slice(uint8_t* const begin, uint8_t* const end) const noexcept {
        return _BufSlice(begin, end - begin);
    }
};

template<typename Alloc = std::allocator<uint8_t>>
class _Buf : _BufSlice {
public:
    using allocator_t = Alloc;
    using Self = _Buf<Alloc>;
    typedef std::shared_ptr<_Buf> Ptr;

    constexpr static std::shared_ptr<Self> alloc(size_t const size) {
        uint8_t* mem = allocator_t().allocate(size);
        return std::make_shared<Self>(mem, size);
    }

    constexpr static std::shared_ptr<Self> alloc(size_t const size, size_t const padding) {
        uint8_t* mem = allocator_t().allocate(size);
        return std::make_shared<Self>(mem, size);
    }

    template<typename T>
    constexpr static std::shared_ptr<Self> copy_memory_of(T const t) {
        auto buf = Self::alloc(sizeof(T));
        *((T*)(buf->mem)) = t;
        return buf;
    }

    constexpr static std::shared_ptr<Self> copy_from(_BufSlice slice) {
        auto buf = Self::alloc(slice.size());
        memcpy(buf, slice.mem(), slice.size());
        return buf;
    }

    constexpr static std::shared_ptr<Self> take_ownership(uint8_t* mem, size_t const size) noexcept {
        return std::make_shared<Self>(mem, size);
    }

    constexpr _Buf() = delete;
    constexpr _Buf(_Buf const& other) = delete;
    constexpr _Buf(uint8_t* m, size_t const sz) noexcept : _BufSlice(m, sz) {}
    constexpr ~_Buf() {
        if (_mem) {
            allocator_t().deallocate(_mem, _size);
        }
    }
};

using BufSlice = const _BufSlice;
using Buf = _Buf<>;
