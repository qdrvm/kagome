/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TEST_CORE_STORAGE_MERKLE_MOCK_CODEC_HPP_
#define KAGOME_TEST_CORE_STORAGE_MERKLE_MOCK_CODEC_HPP_

#include "storage/merkle/codec.hpp"

#include <gmock/gmock.h>

namespace kagome::storage::merkle {

  class MockCodec : public Codec {
   public:
    MOCK_CONST_METHOD1(nodeEncode, common::Buffer(const Node &));
    MOCK_CONST_METHOD1(nodeDecode,
                       std::shared_ptr<Node>(const common::Buffer &));
    MOCK_CONST_METHOD1(hash256, common::Hash256(const common::Buffer &));
    MOCK_CONST_METHOD1(orderedTrieRoot,
                       common::Hash256(const std::vector<common::Buffer> &));
  };

}  // namespace kagome::storage::merkle

#endif  // KAGOME_TEST_CORE_STORAGE_MERKLE_MOCK_CODEC_HPP_
