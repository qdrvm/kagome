/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <type_traits>
#include "primitives/math.hpp"
#include "pvm/types.hpp"

namespace kagome:: pvm {
    enum BackendKind {
        Compiler,
        Interpreter,
    };

    enum SandboxKind {
        Linux,
        Generic,
    };

    struct Config {
        Opt<BackendKind> backend = std::nullopt;
        Opt<SandboxKind> sandbox = std::nullopt;
        bool crosscheck = false;
        bool allow_experimental = false;
        bool allow_dynamic_paging = false;
        size_t worker_count = 2;
        bool cache_enabled = false;
        uint32_t lru_cache_size = 0;
    };
}
