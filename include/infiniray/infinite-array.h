/*
 * Copyright (C) 2023 Eugene Hutorny <eugene@hutorny.in.ua>
 *
 * infinite-array.h - Infinite Array header
 *
 * Licensed under MIT License, see full text in LICENSE
 * or visit page https://opensource.org/license/mit/
 */
#pragma once
#include <infiniray/throw-or-abort.h>
#include <cstdint>
#include <iterator>
#include <type_traits>
#include <bit>

namespace infinite {
namespace detail {
struct novalue {
    constexpr novalue() noexcept = default;
    template<typename ... Arg>
    constexpr novalue(Arg ...) noexcept {}
    constexpr novalue(const novalue&) noexcept = default;
    constexpr novalue(novalue&&) noexcept = default;
    constexpr novalue& operator=(novalue&&) noexcept = default;
    constexpr novalue& operator=(const novalue&) noexcept = default;
};

template<typename A, typename B>
constexpr auto roundup(A value, B measure) noexcept {
    return measure * ((value + measure - 1) / measure);
}

template<typename A, typename B, typename C>
constexpr auto mulmod(A a, B b, C mod) noexcept {
    return ((a % mod) * (b % mod)) % mod;
}
template<typename PointerTo, typename PointerFrom>
constexpr PointerTo dbl_cast(PointerFrom ptr) { return static_cast<PointerTo>(static_cast<void*>(ptr)); }

}

template<class Pointer, class SizeType = std::size_t>
struct allocation_result { // To use std::allocation_result when available
    Pointer ptr;
    SizeType count;
    SizeType size;
};

struct default_allocator_backend {
    static void* allocate(std::size_t size_bytes);
    static void deallocate(void*, std::size_t size_bytes);
    static std::size_t pagesize() noexcept;
};

template<typename T, class Backend = default_allocator_backend>
class allocator {
public:
    using value_type = T;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    [[nodiscard]] constexpr T* allocate(size_type n) {
        return static_cast<T*>(Backend::allocate(array_size(n)));
    }
    [[nodiscard]] constexpr allocation_result<T*, size_type> allocate_at_least(size_type n) {
        const auto buffer_size = detail::roundup(array_size(n), Backend::pagesize());
        return { static_cast<T*>(Backend::allocate(buffer_size)), buffer_size / sizeof(T), buffer_size };
    }
    constexpr void deallocate(T* p, size_type n) {
        Backend::deallocate(p, array_size(n));
    }
private:
    static constexpr size_type array_size(size_type n) noexcept { return n * sizeof(T); }
};

template<typename T, class Allocator = allocator<T>>
class array {
public:
    using allocator_type = Allocator;
    using difference_type = typename Allocator::difference_type;
    using size_type = typename Allocator::size_type;
    using value_type = T;
    using reference = value_type&;
    using const_reference = const value_type&;
    using pointer = value_type*;
    using const_pointer = const value_type*;
    using iterator = pointer;
    using const_iterator = const_pointer;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    array(size_type capacity_elements) : array{Allocator{}.allocate_at_least(capacity_elements)} {}
    array(const array&) = delete;
    constexpr array(array&&)  = default;
    array& operator=(const array&) = delete;
    ~array() {
        destruct(end(), begin());
        Allocator{}.deallocate(data_, capacity_);
    }
    constexpr size_type capacity() const { return capacity_; }
    constexpr size_type size() const { return size_; }
    constexpr bool empty() const { return size_ == 0; }
    constexpr iterator begin() noexcept {
        if constexpr(sizeof_value_type_is_power_of) {
            return &data_[pos_ % capacity_];
        } else {
            return detail::dbl_cast<pointer>(detail::dbl_cast<char*>(data_) + detail::mulmod(pos_, sizeof(T), buffer_size_));
        }
    }
    constexpr const_iterator begin() const noexcept { return cbegin(); }
    constexpr const_iterator cbegin() const noexcept {
        if constexpr(sizeof_value_type_is_power_of) {
            return &data_[pos_ % capacity_];
        } else {
            return detail::dbl_cast<const_pointer>(detail::dbl_cast<const char*>(data_) + detail::mulmod(pos_, sizeof(T), buffer_size_));
        }
    }
    constexpr reverse_iterator rbegin() noexcept { return reverse_iterator{end()}; }
    constexpr const_reverse_iterator rbegin() const noexcept { return crbegin(); }
    constexpr const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator{cend()}; }

    constexpr iterator end() noexcept { return begin() + size_; }
    constexpr const_iterator end() const noexcept { return cend(); }
    constexpr const_iterator cend() const noexcept { return cbegin() + size_; }
    constexpr reverse_iterator rend() noexcept { return reverse_iterator{begin()}; }
    constexpr const_reverse_iterator rend() const noexcept { return crend(); }
    constexpr const_reverse_iterator crend() const noexcept { return const_reverse_iterator{cbegin()}; }

    constexpr const_reference at(size_type pos) const {
        if(pos >= size_) {
            infiniray_throw_or_abort(std::out_of_range("Index out of range"));
        }
        return cbegin()[pos];
    }
    constexpr reference at(size_type pos) {
        if(pos >= size_) {
            infiniray_throw_or_abort(std::out_of_range("Index out of range"));
        }
        return begin()[pos];
    }
    constexpr const_reference operator[](size_type pos) const { return cbegin()[pos]; }
    constexpr reference operator[](size_type pos) { return begin()[pos]; }
    constexpr reference front() { return begin()[0]; }
    constexpr const_reference front() const { return cbegin()[0]; }
    constexpr reference back() { return begin()[size_]; }
    constexpr const_reference back() const { return cbegin()[size_]; }
    constexpr pointer data() noexcept { return begin(); }
    constexpr const_pointer data() const noexcept { return cbegin(); }

    template<class... Args>
    constexpr void emplace_back(Args&&... args) {
        if (size_ >= capacity_) {
            infiniray_throw_or_abort(std::length_error("array capacity is exhausted"));
        }
        allocator_traits_::construct(allocator, &data_[(pos_+size_++) % capacity_], std::move(args...));
    }
    constexpr void push_back(const value_type& value) {
        if (size_ >= capacity_) {
            infiniray_throw_or_abort(std::length_error("array capacity is exhausted"));
        }
        allocator_traits_::construct(allocator, &data_[(pos_+size_++) % capacity_], value);
    }
    constexpr void push_back(value_type&& value) {
        if (size_ >= capacity_) {
            infiniray_throw_or_abort(std::length_error("array capacity is exhausted"));
        }
        allocator_traits_::construct(allocator, &data_[(pos_+size_++) % capacity_], std::move(value));
    }
    template <class InputIterator>
    constexpr void append(InputIterator first, InputIterator last) {
        while(first != last) {
            emplace_back(*first);
            first++;
        }
    }
    template <class Container>
    constexpr void append(const Container& cont) {
        append(std::cbegin(cont), std::end(cont));
    }
    constexpr void append(std::initializer_list<T> ilist) {
        for(auto i : ilist)
            push_back(i);
    }
    constexpr void resize(size_type count) {
        if (count > capacity_) {
            infiniray_throw_or_abort(std::length_error("resize attempt beyond array capacity"));
        }
        if (count == 0)
            clear();
        else if (count == size_)
            return;
        else if (count > size_) {
            if constexpr (std::is_trivially_default_constructible_v<value_type> && std::is_trivially_destructible_v<value_type>) {
                size_ = count;
                return;
            }
            else
                resize(count, value_type{});
        } else {
            if constexpr(std::is_trivially_destructible_v<value_type>) {
                size_ = count;
            } else {
                destruct(end(), end() + (count - size_));
            }
        }
    }
    constexpr void resize(size_type count, const value_type& value) {
        if (count > capacity_) {
            infiniray_throw_or_abort(std::length_error("resize attempt beyond array capacity"));
        }
        if (count == 0)
            clear();
        else if (count == size_)
            return;
        else if (count > size_) {
            while(size_ != count)
                emplace_back(value);
        } else {
            if constexpr(std::is_trivially_destructible_v<value_type>) {
                size_ = count;
            } else {
                destruct(end(), end() + (count - size_));
            }
        }
        // TODO ensure size_ = count;
    }
    constexpr void clear() noexcept {
        if constexpr(!std::is_trivially_destructible_v<value_type>) {
            destruct(end(), begin());
        }
        size_ = 0;
        pos_ = 0;
    }
    constexpr void erase(size_type n) noexcept {
        if constexpr(!std::is_trivially_destructible_v<value_type>) {
            destruct(end(), begin() + n);
        } else {
            n = std::min(size_, n);
            size_ -= n;
            pos_ += n;
        }
    }

private:
    static constexpr bool sizeof_value_type_is_power_of = ((1ULL << 63) % sizeof(T)) == 0;
    static constexpr bool value_type_has_acceptable_alignment() noexcept {
#       if defined(__cpp_lib_bitops) && (__cpp_lib_bitops >= 201907L)
            return alignof(T) <= (1LL << std::countr_zero(sizeof(T)));
#       else
            return alignof(T) <= (1LL << __builtin_ctzl(sizeof(T)));
#       endif
    }
    using allocator_traits_ = std::allocator_traits<allocator_type>;
    array(allocation_result<pointer, size_type>&& alloc)
      : data_ {alloc.ptr}, capacity_ {alloc.count}, buffer_size_ {alloc.size } {}
    constexpr void destruct(pointer rstart, pointer rfinish) {
        while(rstart != rfinish) {
            (--rstart)->~value_type();
            size_--;
        }
    }
    T* data_;
    size_type capacity_;
    std::conditional_t<sizeof_value_type_is_power_of, detail::novalue, size_type> buffer_size_;
    size_type pos_ {};
    size_type size_ {};
    allocator_type allocator{};
    static_assert(sizeof_value_type_is_power_of || value_type_has_acceptable_alignment(),
            "Type T does not wrap safely on page edges");
};

} // namespace
