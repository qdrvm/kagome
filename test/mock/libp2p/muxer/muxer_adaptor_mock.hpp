/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_MUXER_ADAPTOR_MOCK_HPP
#define KAGOME_MUXER_ADAPTOR_MOCK_HPP

#include "libp2p/muxer/muxer_adaptor.hpp"

namespace libp2p::muxer {
  struct MuxerAdaptorMock : public MuxerAdaptor {};
}  // namespace libp2p::muxer

#endif  // KAGOME_MUXER_ADAPTOR_MOCK_HPP
