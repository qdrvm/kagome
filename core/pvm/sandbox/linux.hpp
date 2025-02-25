/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "pvm/types.hpp"
#include "pvm/config.hpp"
#include "pvm/native/linux.hpp"
#include <fcntl.h>

namespace kagome::pvm::sandbox {
    extern size_t get_native_page_size();
}

namespace kagome::pvm::sandbox::linux {
    using Fd = native::linux::Fd;

    inline size_t align_to_next_page_usize(size_t page, size_t size) {
        assert((page & (page - 1)) == 0);
        return (size + (page - 1)) & ~(page - 1);
    }

    inline Result<Fd> prepare_zygote() {
        const auto native_page_size = get_native_page_size();
        const auto length_aligned = align_to_next_page_usize(native_page_size, ZYGOTE_BLOB.len());
        prepare_sealed_memfd(create_empty_memfd(cstr!("polkavm_zygote"))?, length_aligned, [ZYGOTE_BLOB])
    }

    struct GlobalState {
        //shared_memory: ShmAllocator,
        bool uffd_available;
        Fd zygote_memfd;
        
        static Result<GlobalState> create(const Config &config) {
            const auto uffd_available = config.allow_dynamic_paging;
            if (uffd_available) {
                OUTCOME_TRY(userfaultfd, native::linux::sys_userfaultfd(O_CLOEXEC));
                // TODO(iceseer): userfaultfd features
            }

            OUTCOME_TRY(zygote_memfd, prepare_zygote());
            return GlobalState {
                //shared_memory: ShmAllocator::new()?,
                .uffd_available = uffd_available,
                .zygote_memfd = zygote_memfd,
            };
        }
    };

}
