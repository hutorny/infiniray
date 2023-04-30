/*
 * Copyright (C) 2023 Eugene Hutorny <eugene@hutorny.in.ua>
 *
 * mirror-mmap.h - Infinite Array header
 *
 * Licensed under MIT License, see full text in LICENSE
 * or visit page https://opensource.org/license/mit/
 */
#pragma once
#include <cstdint>
#include <unistd.h>
#include <sys/mman.h>
#include <stdexcept>
#include <system_error>
#include <infiniray/throw-or-abort.h>

namespace infinite {
namespace detail {

class mirrored_region {
public:
    mirrored_region(void * addr, std::size_t size, int fd)
      : addr_{::mmap(addr, size, prot, fixed, fd, 0)}, size_{size} {
          if (addr_ == MAP_FAILED) {
              infiniray_throw_or_abort(std::runtime_error{"mmap failed"});
          }
      }
    mirrored_region(std::size_t size)
     : addr_{::mmap(nullptr, size, prot, flags, -1, 0)}, size_{size} {
        if (addr_ == MAP_FAILED) {
            infiniray_throw_or_abort(std::runtime_error{"mmap failed"});
        }
    }

    ~mirrored_region() {
        if(addr_ != nullptr && addr_ != MAP_FAILED) {
            ::munmap(addr_, size_);
        }
    }
    mirrored_region(mirrored_region&&) = delete;
    mirrored_region(const mirrored_region&) = delete;
    mirrored_region& operator=(const mirrored_region&) = delete;
    static void deallocate_mirror(void * addr, std::size_t size) {
        ::munmap(static_cast<char*>(addr) + size, size);
        ::munmap(addr, size);
        ::munmap(addr, size*2);
    }
    constexpr operator void*() const { return static_cast<void*>(addr_); }
    constexpr operator char*() const { return static_cast<char*>(addr_); }
    constexpr size_t size() const { return size_; }
    void* take() {
        auto addr = addr_;
        addr_ = nullptr;
        return addr;
    }
    static inline bool is_mirroring_valid(const mirrored_region& r1, const mirrored_region& r2) {
        // TODO check r1.size() == r2.size()
        return is_mirroring_valid(static_cast<volatile char*>(r1.addr_), static_cast<volatile char*>(r2.addr_), r1.size());
    }
private:
    static inline bool is_mirroring_valid(volatile char* addr1, volatile char* addr2, std::size_t size) {
        constexpr char aa = static_cast<char>(0xAA);
        if (addr2 != addr1+size) {
            return false;
        }
        addr1[0] = 0;
        addr2[0] = 0x55;
        addr2[size-1] = 0;
        addr1[size-1] = aa;
        if((addr1[0] == 0x55) && (addr2[size-1] == aa)) {
            addr2[0] = 0;
            addr1[size-1] = 0;
            return true;
        }
        return false;
    }

    static constexpr int prot = PROT_READ | PROT_WRITE;
    static constexpr int flags = MAP_PRIVATE | MAP_ANONYMOUS;
    static constexpr int fixed = MAP_SHARED | MAP_FIXED;
    void *addr_;
    std::size_t size_;
};
} // namespace detail

namespace tmpfs {
class region {
public:
    region(size_t size) {
        auto tmp = tmpfile();
        if (tmp == nullptr)
            infiniray_throw_or_abort(std::runtime_error{"tmpfile file not available"});
        fd = fileno(tmp);
        if (fd <= 0)
            infiniray_throw_or_abort(std::runtime_error{"tmpfile fd not available"});
        const auto truncated = ftruncate(fd, static_cast<off_t>(size));
        if (truncated<0)
            infiniray_throw_or_abort(std::system_error(errno, std::generic_category()));
    }
    operator int() const { return fd; }
    ~region() { close (fd); }
private:
    int fd {};
};
} // namespace tmpfs

inline auto shared_region(size_t size, long) { return tmpfs::region{size}; }

inline std::size_t default_allocator_backend::pagesize() noexcept {
    return static_cast<std::size_t>(sysconf(_SC_PAGESIZE));
}

inline void* default_allocator_backend::allocate(std::size_t bytes) {
    using namespace detail;
    mirrored_region base{ bytes * 2 };
    auto region { shared_region(bytes, 0) };
    mirrored_region r1{ base, bytes, region };
    mirrored_region r2{ base + bytes, bytes, region };
    if (!mirrored_region::is_mirroring_valid(r1, r2)) {
        infiniray_throw_or_abort(std::runtime_error{"mirroring failed"});
        return nullptr;
    }
    base.take();
    r2.take();
    return r1.take();
}

inline void default_allocator_backend::deallocate(void* addr, std::size_t bytes) {
    detail::mirrored_region::deallocate_mirror(addr, detail::roundup(bytes, pagesize()));
}
} // namespace infinite
