/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_NODE_HPP
#define KAGOME_NODE_HPP

#include "common/buffer.hpp"

namespace kagome::storage::merkle {

  using common::Buffer;

  enum class NodeType : uint8_t {
    LEAF = 1,
    EXTENSION = 128,
    BRANCH_NO_VALUE = 254,
    BRANCH_WITH_VALUE = 255
  };

  class Node {
   public:
    // returns type of a node
    virtual NodeType getType() const = 0;
  };

}  // namespace kagome::storage::merkle

#endif  // KAGOME_NODE_HPP
