/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_SPACES_HPP
#define KAGOME_SPACES_HPP

#include <cstdint>

namespace kagome::storage {

  enum Space : uint8_t {
    kDefault = 0,
    //    kLookupKey,
    //    kHeader,
    //    kBlockData,
    //    kTrieNode,
    //
    kTotal
  };
}

#endif  // KAGOME_SPACES_HPP
