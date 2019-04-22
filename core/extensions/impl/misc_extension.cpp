/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <exception>

#include "extensions/impl/misc_extension.hpp"

namespace kagome::extensions {

  MiscExtension::MiscExtension(uint64_t chain_id):
    chain_id_ {chain_id} {}

  uint64_t MiscExtension::ext_chain_id() const {
    return chain_id_;
  }
}  // namespace extensions
