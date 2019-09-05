/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TEST_MOCK_CORE_AUTHORSHIP_BLOCK_BUILDER_FACTORY_MOCK_HPP
#define KAGOME_TEST_MOCK_CORE_AUTHORSHIP_BLOCK_BUILDER_FACTORY_MOCK_HPP

#include "authorship/block_builder_factory.hpp"

#include <gmock/gmock.h>

namespace kagome::authorship {

  class BlockBuilderFactoryMock : public BlockBuilderFactory {
   public:
    // Dirty hack from https://stackoverflow.com/a/11548191 to overcome issue
    // with returning unique_ptr from gmock
    outcome::result<std::unique_ptr<BlockBuilder>> create(
        const primitives::BlockId &block_id,
        std::vector<primitives::Digest> digests) const override {
      return std::unique_ptr<BlockBuilder>(
          createProxy(block_id, std::move(digests)));
    }

    MOCK_CONST_METHOD2(createProxy,
                       BlockBuilder *(const primitives::BlockId &,
                                      std::vector<primitives::Digest>));
  };

}  // namespace kagome::authorship

#endif  // KAGOME_TEST_MOCK_CORE_AUTHORSHIP_BLOCK_BUILDER_FACTORY_MOCK_HPP
