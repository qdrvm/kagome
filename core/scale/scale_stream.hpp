/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_SCALE_SCALE_STREAM_HPP
#define KAGOME_CORE_SCALE_SCALE_STREAM_HPP

#include "scale/scale_decoder_stream.hpp"
#include "scale/scale_encoder_stream.hpp"

namespace kagome::scale {
  class ScaleStream : public ScaleEncoderStream, public ScaleDecoderStream {
   public:

  };
}  // namespace kagome::scale

#endif  // KAGOME_CORE_SCALE_SCALE_STREAM_HPP
