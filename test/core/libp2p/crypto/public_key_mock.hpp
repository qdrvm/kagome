/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PUBLIC_KEY_MOCK_HPP
#define KAGOME_PUBLIC_KEY_MOCK_HPP

#include <gmock/gmock.h>
#include "libp2p/crypto/public_key.hpp"

namespace libp2p::crypto {
  class PublicKeyMock : public PublicKey {
   public:
    MOCK_CONST_METHOD0(getType, common::KeyType());

    MOCK_CONST_METHOD0(getBytes, const kagome::common::Buffer &());

    MOCK_CONST_METHOD0(toString, std::string());

    bool operator==(const Key &other) const override {
      return getBytes() == other.getBytes() && getType() == other.getType();
    }
    bool operator!=(const Key &other) const override {
      return !(*this == other);
    }

    ~PublicKeyMock() override = default;
  };
}  // namespace libp2p::crypto

#endif  // KAGOME_PUBLIC_KEY_MOCK_HPP
