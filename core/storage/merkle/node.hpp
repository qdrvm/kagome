/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_NODE_HPP
#define KAGOME_NODE_HPP

namespace kagome::storage::merkle {

  struct Node {
    virtual ~Node() = default;

    // returns type of a node
    virtual int getType() const = 0;
  };

}  // namespace kagome::storage::merkle

#endif  // KAGOME_NODE_HPP
