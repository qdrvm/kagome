/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_MULTIBASE_CODEC_MOCK_HPP
#define KAGOME_MULTIBASE_CODEC_MOCK_HPP

#include <gmock/gmock.h>
#include "libp2p/multi/multibase_codec.hpp"

namespace libp2p::multi {
  class MultibaseCodecMock : public MultibaseCodec {
    MOCK_CONST_METHOD2(encode,
                       std::string(const kagome::common::Buffer &, Encoding));
    MOCK_CONST_METHOD1(decode,
                       kagome::expected::Result<kagome::common::Buffer,
                                                std::string>(std::string_view));
  };
}  // namespace libp2p::multi

#endif  // KAGOME_MULTIBASE_CODEC_MOCK_HPP
