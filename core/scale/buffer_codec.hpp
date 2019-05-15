/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BUFFER_CODEC_HPP
#define KAGOME_BUFFER_CODEC_HPP

#include "scale/scale_codec.hpp"
#include "scale/collection.hpp"

namespace kagome::scale {

  class BufferScaleCodec : public ScaleEncoder<common::Buffer>,
                           public ScaleDecoder<common::Buffer> {
   public:
    outcome::result<common::Buffer> encode(const common::Buffer &val) final;

    outcome::result<common::Buffer> decode(common::ByteStream &stream) final;
  };

  // TODO(yuraz): PRE-119 this will be refactored soon
  // there's no better place than that
  template <>
  struct TypeDecoder<common::Buffer> {
    outcome::result<common::Buffer> decode(common::ByteStream &stream) {
      OUTCOME_TRY(data, collection::decodeCollection<uint8_t>(stream));
      return common::Buffer(std::move(data));
    }
  };

  template <>
  struct TypeEncoder<common::Buffer> {
    outcome::result<void> encode(common::Buffer &value, common::Buffer &out) {
      OUTCOME_TRY(compact::encodeInteger(value.size(), out));
      out.putBuffer(value);
      return outcome::success();
    }
  };

  extern template struct TypeEncoder<common::Buffer>;
  extern template struct TypeDecoder<common::Buffer>;

}  // namespace kagome::scale

#endif  // KAGOME_BUFFER_CODEC_HPP
