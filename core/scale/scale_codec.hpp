/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_SCALE_CODEC_HPP
#define KAGOME_SCALE_CODEC_HPP

#include "common/buffer.hpp"

namespace kagome::scale {

  template <typename T>
  struct ScaleEncoder {
    virtual ~ScaleEncoder() = default;
    virtual outcome::result<common::Buffer> encode(const T &val) = 0;
  };

  template <typename T>
  struct ScaleDecoder {
    virtual ~ScaleDecoder() = default;
    virtual outcome::result<T> decode(const common::Buffer &buf) = 0;
  };

  template <typename T>
  class ScaleCodec : public ScaleEncoder<T>, public ScaleDecoder<T> {
   public:
    ~ScaleCodec() override = default;
  };

}  // namespace kagome::scale

#endif  // KAGOME_SCALE_CODEC_HPP
