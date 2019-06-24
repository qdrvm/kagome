/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PROTOCOL_MUXER_MOCK_HPP
#define KAGOME_PROTOCOL_MUXER_MOCK_HPP

#include <gmock/gmock.h>
#include "libp2p/protocol_muxer/protocol_muxer.hpp"

namespace libp2p::protocol_muxer {
  class ProtocolMuxerMock : public ProtocolMuxer {
   public:
    MOCK_CONST_METHOD4(
        selectOneOf,
        void(gsl::span<const peer::Protocol>,
             std::shared_ptr<basic::ReadWriteCloser>, bool,
             std::function<void(outcome::result<peer::Protocol>)>));
  };
}  // namespace libp2p::protocol_muxer

#endif  // KAGOME_PROTOCOL_MUXER_MOC_HPP
