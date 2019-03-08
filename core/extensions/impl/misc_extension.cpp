/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <exception>

#include "extensions/impl/misc_extension.hpp"

namespace kagome::extensions {
  uint64_t MiscExtension::ext_chain_id() {
    std::terminate();
  }
}  // namespace extensions
