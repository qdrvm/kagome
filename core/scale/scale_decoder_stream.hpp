/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_SCALE_SCALE_DECODER_STREAM_HPP
#define KAGOME_CORE_SCALE_SCALE_DECODER_STREAM_HPP

#include <gsl/span>
#include "common/byte_stream.hpp"

namespace kagome::scale {
  class ScaleDecoderStream : public common::ByteStream {
   public:
    explicit ScaleDecoderStream(gsl::span<const uint8_t> span) : data_{span} {}

    // TODO(yuraz): PRE-119 implement override functions
    bool hasMore(uint64_t n) const override {
      return false;
    }

    std::optional<uint8_t> nextByte() override {
      return std::nullopt;
    }

    outcome::result<void> advance(uint64_t dist) override {
      return outcome::success();
    }

   private:
    gsl::span<const uint8_t> data_;
  };

}  // namespace kagome::scale

#endif  // KAGOME_CORE_SCALE_SCALE_DECODER_STREAM_HPP
