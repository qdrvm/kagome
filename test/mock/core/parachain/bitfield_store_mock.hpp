/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BITFIELD_STORE_MOCK_HPP
#define KAGOME_BITFIELD_STORE_MOCK_HPP

#include <gmock/gmock.h>

#include "parachain/availability/bitfield/store.hpp"

namespace kagome::parachain {

  class BitfieldStoreMock : public BitfieldStore {
   public:
    MOCK_METHOD(void,
                putBitfield,
                (const primitives::BlockHash &, const SignedBitfield &),
                (override));

    MOCK_METHOD(std::vector<SignedBitfield>,
                getBitfields,
                (const primitives::BlockHash &),
                (const, override));

    MOCK_METHOD(void, remove, (const BlockHash &), (override));
  };

}  // namespace kagome::parachain

#endif /* KAGOME_BITFIELD_STORE_MOCK_HPP */
