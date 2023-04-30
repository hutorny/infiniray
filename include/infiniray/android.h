#pragma once
#include <cstdint>
#include <system_error>
#include <errno.h>
#include <sys/stat.h>        /* For mode constants */
#include <fcntl.h>           /* For O_* constants */
#include <unistd.h>
#include <linux/ashmem.h>
#include <android/sharedmem_jni.h>
#include <string_view>
#include <infiniray/throw-or-abort.h>

namespace infinite {
namespace ashmem {
// TODO use ASharedMemory_create(nullptr, size) if available
class region {
public:
    static constexpr const char* device = "/dev/ashmem";
    using size_t = std::size_t;
    region(size_t size) : fd_ { open(device, O_RDWR)}, size_{size} {
        if(fd_ < 0) {
            fd_ = mktmp(); // fallback to application's cache directory
            if(fd_ < 0)
                infiniray_throw_or_abort(std::system_error(errno, std::generic_category()));
            const int ret = ftruncate(fd_, size_);
            if (ret < 0)
                infiniray_throw_or_abort(std::system_error(errno, std::generic_category()));
            nounpin = true;
        } else {
            const int ret = ioctl(fd_, ASHMEM_SET_SIZE, size_);
            if (ret < 0)
                infiniray_throw_or_abort(std::system_error(errno, std::generic_category()));
        }
    }
    ~region() {
        if(!nounpin)
            unpin();
        close(fd_);
    }
    operator int() const { return fd_; }
    void unpin() {
        struct ashmem_pin pin = { 0, static_cast<__u32>(size_) };
        ioctl(fd_, ASHMEM_UNPIN, &pin);
    }
    static void settmpdir(std::string_view path) {
        tmpdir = path;
    }
private:
    int fd_;
    size_t size_;
    bool nounpin {};
    static inline std::string tmpdir {"/tmp"};
    static int mktmp() {
        std::string tmpname = tmpdir + "/snake.XXXXXX";
        auto fd = mkstemp(&tmpname[0]);
        unlink(tmpname.c_str());
        return fd;
    }
};
} //namespace ashmem

inline auto shared_region(size_t size, int) { return ashmem::region{size}; }

} //namespace infinite