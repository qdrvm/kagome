/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "scale/buffer_codec.hpp"

#include "scale/collection.hpp"
#include "scale/scale_encoder_stream.hpp"

namespace kagome::scale {

  // TODO(yuraz): refactor
  outcome::result<common::Buffer> BufferScaleCodec::encode(
      const common::Buffer &val) {
    ScaleEncoderStream out;
    out << val.toVector();
    return out.getBuffer();
  }

  // TODO(yuraz): don't implement
  outcome::result<common::Buffer> BufferScaleCodec::decode(
      common::ByteStream &stream) {
    return common::Buffer{};
  }
}  // namespace kagome::scale
