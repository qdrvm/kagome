/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_SCALE_BLOB_CODEC_HPP
#define KAGOME_CORE_SCALE_BLOB_CODEC_HPP

#include "common/blob.hpp"
#include "common/buffer.hpp"
#include "common/byte_stream.hpp"
#include "scale/fixedwidth.hpp"
#include "scale/type_decoder.hpp"

namespace kagome::scale {
  // TODO(yuraz): PRE-119 refactor scale
  /**
   * @brief scale decodes common::Blob
   * @tparam sz Blob size
   */
  template <size_t sz>
  struct TypeDecoder<common::Blob<sz>> {
    using Blob = common::Blob<sz>;
    outcome::result<Blob> decode(common::ByteStream &stream) {
      Blob blob;
      for (size_t i = 0; i < Blob::size(); i++) {
        OUTCOME_TRY(byte, fixedwidth::decodeUint8(stream));
        blob[i] = byte;
      }
      return blob;
    }
  };
}  // namespace kagome::scale

#endif  // KAGOME_CORE_SCALE_BLOB_CODEC_HPP
