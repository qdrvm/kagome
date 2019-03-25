/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_MERKLE_UTIL_HPP
#define KAGOME_MERKLE_UTIL_HPP

#include "common/buffer.hpp"
#include "common/result.hpp"

#include "storage/merkle/node.hpp"

namespace kagome::storage::merkle {

  using common::Buffer;

  class Codec {
   public:
    virtual Buffer nodeEncode(const Node &node) const = 0;

    virtual std::shared_ptr<Node> nodeDecode(const Buffer &node) const = 0;

    virtual Buffer hash256(const Buffer& buf) const = 0;

  };

}  // namespace kagome::merkle

#endif  // KAGOME_MERKLE_UTIL_HPP
