/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "scale/buffer_codec.hpp"

#include <gsl/span>

#include "common/buffer.hpp"
#include "scale/collection.hpp"
#include "scale/scale_encoder_stream.hpp"
#include "scale/scale.hpp"

namespace kagome::scale {

  // TODO(yuraz): refactor
  outcome::result<common::Buffer> BufferScaleCodec::encode(
      const common::Buffer &val) {
    OUTCOME_TRY(res, scale::encode(val));
    return common::Buffer(res);
  }

  // TODO(yuraz): don't implement
  outcome::result<common::Buffer> BufferScaleCodec::decode(
      common::ByteStream &stream) {
    return common::Buffer{};
  }
}  // namespace kagome::scale
