/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstdint>

namespace kagome::storage {

  enum Space : uint8_t {
    // must have spaces
    kDefault = 0,
    kLookupKey,

    // application-defined spaces
    kHeader,
    kBlockBody,
    kJustification,
    kTrieNode,
    kTrieValue,
    kDisputeData,
    kBeefyJustification,

    kTotal
  };
}
