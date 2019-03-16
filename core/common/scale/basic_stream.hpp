/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_SCALE_BASIC_STREAM_HPP
#define KAGOME_SCALE_BASIC_STREAM_HPP

#include "common/scale/types.hpp"

namespace kagome::common::scale {

  /**
   * @class BasicStream implements Stream interface
   * It wraps ByteArray and allows getting bytes
   * from it sequentially.
   */
  class BasicStream : public Stream {
   public:
    explicit BasicStream(const ByteArray &source);

    ~BasicStream() override = default;

    /**
     * @brief hasMore Checks whether n more bytes are available
     * @param n Number of bytes to check
     * @return True if n more bytes are available and false otherwise
     */
    bool hasMore(uint64_t n) const override;

    /**
     * @brief nextByte Takes current byte and moves pointer to the next one
     * @return Current byte
     */
    std::optional<uint8_t> nextByte() override;

   private:
    const ByteArray &source_;  ///< reference to source vector of bytes
    ByteArray::const_iterator current_iterator_;  ///< current byte iterator
    int64_t bytes_left_;                          ///< remaining bytes counter
  };
}  // namespace kagome::common::scale

#endif  // KAGOME_SCALE_BASIC_STREAM_HPP
