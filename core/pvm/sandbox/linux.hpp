/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <fcntl.h>
#include "pvm/config.hpp"
#include "pvm/native/linux.hpp"
#include "pvm/sandbox/polkavm_zygote.hpp"
#include "pvm/types.hpp"
#include "pvm/allocator/linux/shared_memory_allocator.hpp"

namespace kagome::pvm::sandbox {
  extern size_t get_native_page_size();
}

namespace kagome::pvm::sandbox::linux {
  using Fd = native::linux::Fd;

  inline size_t align_to_next_page_usize(size_t page, size_t size) {
    assert((page & (page - 1)) == 0);
    return (size + (page - 1)) & ~(page - 1);
  }

  inline Result<Fd> create_empty_memfd(const char *name) {
    const unsigned int MFD_CLOEXEC = 0x0001;        // Флаг CLOEXEC
    const unsigned int MFD_ALLOW_SEALING = 0x0002;  // Флаг ALLOW_SEALING
    return native::linux::sys_memfd_create(name,
                                           MFD_CLOEXEC | MFD_ALLOW_SEALING);
  }

  template <size_t N>
  inline Result<size_t> writev(Fd fd, const std::span<uint8_t> (&data)[N]) {
    native::linux::iovec iv[N];
    for (size_t i = 0; i < N; ++i) {
      iv[i].iov_base = data[i].data();
      iv[i].iov_len = data[i].size();
    }
    return native::linux::sys_writev(fd, iv);
  }

  template <size_t N>
  inline Result<Fd> prepare_sealed_memfd(Fd memfd,
                                         size_t length,
                                         const std::span<uint8_t> (&data)[N]) {
    const auto native_page_size = get_native_page_size();
    if (length % native_page_size != 0) {
      return Error::LEN_UNALIGNED;
    }

    OUTCOME_TRY(native::linux::sys_ftruncate(memfd, (unsigned long)length));
    size_t expected_bytes_written = 0;
    for (const auto &d : data) {
      expected_bytes_written += d.size();
    }

    OUTCOME_TRY(bytes_written, writev(memfd, data));
    if (bytes_written != expected_bytes_written) {
      return Error::MEMFD_INCOMPLETE_WRITE;
    }

    OUTCOME_TRY(native::linux::sys_fcntl(
        memfd,
        F_ADD_SEALS,
        F_SEAL_SEAL | F_SEAL_SHRINK | F_SEAL_GROW | F_SEAL_WRITE));
    return outcome::success(memfd);
  }

  inline Result<Fd> prepare_zygote() {
    const auto native_page_size = get_native_page_size();
    const auto length_aligned =
        align_to_next_page_usize(native_page_size, ZYGOTE_BLOB_LEN);
    OUTCOME_TRY(memfd, create_empty_memfd("polkavm_zygote"));
    return prepare_sealed_memfd(memfd, length_aligned, {ZYGOTE_BLOB});
  }

  struct GlobalState {
    // shared_memory: ShmAllocator,
    bool uffd_available;
    Fd zygote_memfd;

    static Result<GlobalState> create(const Config &config) {
      const auto uffd_available = config.allow_dynamic_paging;
      if (uffd_available) {
        OUTCOME_TRY(userfaultfd, native::linux::sys_userfaultfd(O_CLOEXEC));
        (void)userfaultfd;
        // TODO(iceseer): userfaultfd features
      }

      OUTCOME_TRY(zygote_memfd, prepare_zygote());
      return GlobalState{
          // shared_memory: ShmAllocator::new()?,
          .uffd_available = uffd_available,
          .zygote_memfd = zygote_memfd,
      };
    }
  };

}  // namespace kagome::pvm::sandbox::linux
