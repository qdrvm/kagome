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
    MOCK_METHOD(outcome::result<std::unique_ptr<BlockBuilder>>,
                create,
                (const primitives::BlockId &, primitives::Digest),
                (const, override));
  };

}  // namespace kagome::authorship

#endif  // KAGOME_TEST_MOCK_CORE_AUTHORSHIP_BLOCK_BUILDER_FACTORY_MOCK_HPP
