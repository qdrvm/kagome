/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BUFFER_CODEC_HPP
#define KAGOME_BUFFER_CODEC_HPP

#include "scale/collection.hpp"
#include "scale/scale_codec.hpp"

namespace kagome::scale {

  class BufferScaleCodec : public ScaleEncoder<common::Buffer>,
                           public ScaleDecoder<common::Buffer> {
   public:
    outcome::result<common::Buffer> encode(const common::Buffer &val) final;

    outcome::result<common::Buffer> decode(common::ByteStream &stream) final;
  };

  template <>
  struct TypeDecoder<common::Buffer> {
    outcome::result<common::Buffer> decode(common::ByteStream &stream) {
      OUTCOME_TRY(c, collection::decodeCollection<uint8_t>(stream));
      return common::Buffer(c);
    }
  };
}  // namespace kagome::scale

#endif  // KAGOME_BUFFER_CODEC_HPP
