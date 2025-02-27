/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <errno.h>
#include <sys/syscall.h>
#include <fstream>
#include "pvm/types.hpp"

namespace kagome::pvm::native::linux {
  using Fd = long int;

  struct iovec {
    void *iov_base;
    size_t iov_len;
  };

  inline Result<void> check_syscall(Fd result) {
    auto unprivileged_userfaultfd = []() {
      std::ifstream file("/proc/sys/vm/unprivileged_userfaultfd",
                         std::ios::binary);
      if (file) {
        char buffer[2];
        if (file.read(buffer, 2)) {
          return buffer[0] == '0' && buffer[1] == '\n';
        }
      }
      return false;
    };

    if (result >= -4095 && result < 0) {
      if (errno == EPERM && unprivileged_userfaultfd()) {
        return Error::SYS_CALL_NOT_PERMITTED;
      }
      return Error::SYS_CALL_FAILED;
    }
    return outcome::success();
  }

    inline size_t encode_to_machine_word(void *v) {
        return (size_t)v;
    }
    inline size_t encode_to_machine_word(const void *v) {
        return (size_t)v;
    }
    inline size_t encode_to_machine_word(uint64_t v) {
        return (size_t)v;
    }
    inline size_t encode_to_machine_word(uint32_t v) {
        return (size_t)v;
    }
    inline size_t encode_to_machine_word(int64_t v) {
        return (size_t)(std::intptr_t)v;
    }
    inline size_t encode_to_machine_word(int32_t v) {
        return (size_t)(std::intptr_t)v;
    }
    inline size_t encode_to_machine_word(Opt<Fd> v) {
        if (!v) {
            return encode_to_machine_word(int64_t(-1));    
        }
        return encode_to_machine_word(*v);
    }

  template <typename... Args>
  inline Result<Fd> __syscall(long int sysno, Args &&...args) {
    auto fd = syscall(sysno, encode_to_machine_word(args)...);
    OUTCOME_TRY(check_syscall(fd));
    return outcome::success(fd);
  }

  inline Result<Fd> sys_userfaultfd(unsigned int flags) {
    return __syscall(SYS_userfaultfd, flags);
  }

  inline Result<Fd> sys_memfd_create(const char *name, unsigned int flags) {
    return __syscall(SYS_memfd_create, name, flags);
  }

  inline Result<void> sys_ftruncate(Fd fd, size_t length) {
    OUTCOME_TRY(_, __syscall(SYS_ftruncate, fd, length));
    (void)_;
    return outcome::success();
  }

  template <size_t N>
  inline Result<size_t> sys_writev(Fd fd, const iovec (&iv)[N]) {
    OUTCOME_TRY(result, __syscall(SYS_writev, fd, iv, N));
    return outcome::success(size_t(result));
  }

  inline Result<int32_t> sys_fcntl(Fd fd, uint32_t cmd, uint32_t arg) {
    OUTCOME_TRY(result, __syscall(SYS_fcntl, fd, cmd, arg));
    return outcome::success(int32_t(result));
  }

inline Result<void*> sys_mmap(
    void *address,
    size_t length,
    uint32_t protection,
    uint32_t flags,
    Opt<Fd> fd,
    uint64_t offset
) {
    OUTCOME_TRY(result, __syscall(SYS_mmap, address, length, protection, flags, fd, offset));
    return outcome::success((void*)result);
}


}  // namespace kagome::pvm::native::linux
