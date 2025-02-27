/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <atomic>
#include <assert.h>
#include "pvm/sandbox.hpp"

namespace kagome::pvm::sandbox {
    std::atomic_size_t NATIVE_PAGE_SIZE = 0;

    void init_native_page_size() {
        if (NATIVE_PAGE_SIZE.load(std::memory_order_relaxed) != 0) {
            return;
        }

        long page_size = sysconf(_SC_PAGE_SIZE);
        if (page_size == -1) {
            throw std::runtime_error("Unable to get page size!");            
        }        

        NATIVE_PAGE_SIZE.store(page_size);
    }

    size_t get_native_page_size() {
        const auto page_size = NATIVE_PAGE_SIZE.load(std::memory_order::relaxed);
        assert(page_size != 0);
        return page_size;
    }
}