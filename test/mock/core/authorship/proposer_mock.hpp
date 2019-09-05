/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PROPOSER_MOCK_HPP
#define KAGOME_PROPOSER_MOCK_HPP

#include "authorship/proposer.hpp"

#include <gmock/gmock.h>

namespace kagome::authorship {
  struct ProposerMock : public Proposer {
    MOCK_METHOD3(
        propose,
        outcome::result<primitives::Block>(const primitives::BlockId &,
                                           const primitives::InherentData &,
                                           std::vector<primitives::Digest>));
  };
}  // namespace kagome::authorship

#endif  // KAGOME_PROPOSER_MOCK_HPP
