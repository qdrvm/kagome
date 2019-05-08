/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BUFFER_CODEC_HPP
#define KAGOME_BUFFER_CODEC_HPP

#include "scale/scale_codec.hpp"

namespace kagome::scale {

  class BufferScaleCodec : public ScaleEncoder<common::Buffer> {
   public:
    outcome::result<common::Buffer> encode(const common::Buffer &val) final;
  };

}  // namespace kagome::scale

#endif  // KAGOME_BUFFER_CODEC_HPP
