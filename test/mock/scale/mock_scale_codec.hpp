/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_MOCK_SCALE_BUFFER_CODEC_HPP
#define KAGOME_MOCK_SCALE_BUFFER_CODEC_HPP

#include <gmock/gmock.h>
#include "scale/scale_codec.hpp"

namespace kagome::scale {

  template <typename T>
  struct MockScaleEncoder : public ScaleEncoder<T> {
    MOCK_METHOD1_T(encode, outcome::result<common::Buffer>(const T &));
  };

  template <typename T>
  struct MockScaleDecoder : public ScaleDecoder<T> {
    MOCK_METHOD1_T(decode, outcome::result<T>(const common::Buffer &));
  };

}  // namespace kagome::scale

#endif  // KAGOME_MOCK_SCALE_BUFFER_CODEC_HPP
