/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <variant>
#include "pvm/types.hpp"

namespace kagome::pvm::sandbox {
    extern void init_native_page_size();

    using GlobalStateKind = std::variant<linux::GlobalState>;

}
