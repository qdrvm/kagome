/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_SPACES_HPP
#define KAGOME_SPACES_HPP

#include <cstdint>

namespace kagome::storage {

  enum Space : uint8_t {
    // must have spaces
    kDefault = 0,
    kLookupKey,

    // application-defined spaces
    kHeader,
    kJustification,
    kBlockData,
    kTrieNode,
    kTriePruner,

    //
    kTotal
  };
}

#endif  // KAGOME_SPACES_HPP
