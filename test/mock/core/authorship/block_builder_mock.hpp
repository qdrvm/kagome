/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TEST_MOCK_CORE_AUTHORSHIP_BLOCK_BUILDER_MOCK_HPP
#define KAGOME_TEST_MOCK_CORE_AUTHORSHIP_BLOCK_BUILDER_MOCK_HPP

#include <gmock/gmock.h>
#include "authorship/block_builder.hpp"

namespace kagome::authorship {

  class BlockBuilderMock : public BlockBuilder {
   public:
    MOCK_METHOD1(pushExtrinsic,
                 outcome::result<void>(const primitives::Extrinsic &));
    MOCK_CONST_METHOD0(bake, outcome::result<primitives::Block>());
  };

}  // namespace kagome::authorship

#endif  // KAGOME_TEST_MOCK_CORE_AUTHORSHIP_BLOCK_BUILDER_MOCK_HPP
