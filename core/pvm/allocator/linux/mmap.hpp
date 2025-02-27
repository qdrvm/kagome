/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "pvm/native/linux.hpp"
#include "pvm/types.hpp"

namespace kagome::pvm::native::linux {

    struct Mmap : final {
        Mmap(const Mmap &) = delete;
        Mmap& operator=(const Mmap &) = delete;

        static Result<Mmap> create(void *address, size_t length, uint32_t protection, uint32_t flags, Opt<Fd> fd, uint64_t offset) {
            //auto pointer = sys_mmap(address, length, protection, flags, fd, offset)?;
            //Ok(Self { pointer, length })
            return Error::NOT_IMPLEMENTED;
        }

    private:
        Mmap(void *p, size_t l) : pointer(p), length(l) {}
        void *pointer;
        size_t length;
    };

}

