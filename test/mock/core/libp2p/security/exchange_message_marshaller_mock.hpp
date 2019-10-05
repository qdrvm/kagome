/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_EXCHANGE_MESSAGE_MARSHALLER_MOCK_HPP
#define KAGOME_EXCHANGE_MESSAGE_MARSHALLER_MOCK_HPP

#include <gmock/gmock.h>
#include "libp2p/security/plaintext/exchange_message_marshaller.hpp"
#include "libp2p/security/plaintext/exchange_message.hpp"

namespace libp2p::security::plaintext {
  class ExchangeMessageMarshallerMock : public ExchangeMessageMarshaller {
   public:
    ~ExchangeMessageMarshallerMock() override = default;

    MOCK_CONST_METHOD1(marshal, outcome::result<std::vector<uint8_t>>(const ExchangeMessage&));
    MOCK_CONST_METHOD1(unmarshal, outcome::result<ExchangeMessage>(gsl::span<const uint8_t>));
  };
}
#endif  // KAGOME_EXCHANGE_MESSAGE_MARSHALLER_MOCK_HPP
