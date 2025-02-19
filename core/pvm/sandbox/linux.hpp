/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "pvm/types.hpp"

namespace kagome::pvm::sandbox::linux {

    struct GlobalState {
        shared_memory: ShmAllocator,
        uffd_available: bool,
        zygote_memfd: Fd,
    };

}
