/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_NODE_IMPL_HPP
#define KAGOME_NODE_IMPL_HPP

#include "storage/merkle/node.hpp"

#include "common/buffer.hpp"

namespace kagome::storage::merkle {

  const int kMaxChildren = 16;

  struct PolkadotNode : public Node {
    ~PolkadotNode() override = default;

    bool is_dirty;
    common::Buffer key_nibbles;
    common::Buffer value;
  };

  struct LeafNode : public PolkadotNode {
    ~LeafNode() override = default;

    Type getType() const override;
  };

  struct BranchNode : public PolkadotNode {
    ~BranchNode() override = default;

    Type getType() const override;

    uint16_t childrenBitmap() const;

    // has 1..16 children
    std::array<std::shared_ptr<Node>, kMaxChildren> children;
  };

}  // namespace kagome::storage::merkle

#endif  // KAGOME_NODE_IMPL_HPP
