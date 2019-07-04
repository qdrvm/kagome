/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_KEY_MARSHALLER_MOCK_HPP
#define KAGOME_KEY_MARSHALLER_MOCK_HPP

#include <gmock/gmock.h>
#include "libp2p/crypto/key_marshaller.hpp"

namespace libp2p::crypto::marshaller {
  class KeyMarshallerMock : public KeyMarshaller {
    using Buffer = kagome::common::Buffer;

   public:
    MOCK_CONST_METHOD1(marshal, outcome::result<Buffer>(const PublicKey &));

    MOCK_CONST_METHOD1(marshal, outcome::result<Buffer>(const PrivateKey &));

    MOCK_CONST_METHOD1(unmarshalPublicKey,
                       outcome::result<PublicKey>(const Buffer &));

    MOCK_CONST_METHOD1(unmarshalPrivateKey,
                       outcome::result<PrivateKey>(const Buffer &));
  };
}  // namespace libp2p::crypto::marshaller

#endif  // KAGOME_KEY_MARSHALLER_MOCK_HPP
