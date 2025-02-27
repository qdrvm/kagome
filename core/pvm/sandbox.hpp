/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <variant>
#include "pvm/types.hpp"
#include "pvm/sandbox/linux.hpp"
#include "pvm/config.hpp"

namespace kagome::pvm::sandbox {
    extern void init_native_page_size();
    extern size_t get_native_page_size();

    using GlobalStateKind = std::variant<linux::GlobalState>;
    inline outcome::result<GlobalStateKind> createGlobalState(SandboxKind kind, const Config &config) {
        switch (kind.value) {
            case SandboxKind::Linux: {
                OUTCOME_TRY(global_state, linux::GlobalState::create(config));
                return GlobalStateKind(global_state);
            }
            case SandboxKind::Generic:
                return Error::NOT_IMPLEMENTED;
        }
    }

}
