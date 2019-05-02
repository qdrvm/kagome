/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_NODE_HPP
#define KAGOME_NODE_HPP

namespace kagome::storage::merkle {

  struct Node {
    enum class Type { Leaf, Branch };

    virtual ~Node() = default;

    // returns type of a node
    virtual Type getType() const = 0;
  };

}  // namespace kagome::storage::merkle

#endif  // KAGOME_NODE_HPP
