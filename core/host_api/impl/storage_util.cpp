/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "host_api/impl/storage_util.hpp"

namespace kagome::host_api::detail {
  kagome::storage::trie::StateVersion toStateVersion(
      kagome::runtime::WasmI32 state_version_int) {
    if (state_version_int == 0) {
      return kagome::storage::trie::StateVersion::V0;
    } else if (state_version_int == 1) {
      return kagome::storage::trie::StateVersion::V1;
    } else {
      throw std::runtime_error(fmt::format(
          "Invalid state version: {}. Expected 0 or 1", state_version_int));
    }
  }
}  // namespace kagome::host_api::detail
