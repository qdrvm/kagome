/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_MUXER_ADAPTOR_MOCK_HPP
#define KAGOME_MUXER_ADAPTOR_MOCK_HPP

#include <gmock/gmock.h>
#include "libp2p/muxer/muxer_adaptor.hpp"

namespace libp2p::muxer {
  struct MuxerAdaptorMock : public MuxerAdaptor {
    MOCK_CONST_METHOD0(getProtocolId, peer::Protocol());

    MOCK_CONST_METHOD1(
        muxConnection,
        outcome::result<std::shared_ptr<connection::CapableConnection>>(
            std::shared_ptr<connection::SecureConnection>));
  };
}  // namespace libp2p::muxer

#endif  // KAGOME_MUXER_ADAPTOR_MOCK_HPP
