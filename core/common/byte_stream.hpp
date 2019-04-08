/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_COMMON_BYTE_STREAM_HPP_
#define KAGOME_CORE_COMMON_BYTE_STREAM_HPP_

#include <optional>
#include <outcome/outcome.hpp>

namespace kagome::common {
  /**
   * @brief interface for Decoders
   */
  class ByteStream {
   public:
    enum class AdvanceErrc { OUT_OF_BOUNDARIES = 1 };

    virtual ~ByteStream() = default;

    /**
     * @brief hasMore Checks whether n more bytes are available
     * @param n Number of bytes to check
     * @return True if n more bytes are available and false otherwise
     */
    virtual bool hasMore(uint64_t n) const = 0;

    /**
     * @brief nextByte Takes current byte and moves pointer to the next one
     * @return Current byte
     */
    virtual std::optional<uint8_t> nextByte() = 0;

    /**
     * @brief Shifts pointer by the giuven distance
     * @return void if advance successful, error if advance is not possible
     */
    virtual outcome::result<void> advance(uint64_t dist) = 0;
  };
}  // namespace kagome::common

OUTCOME_HPP_DECLARE_ERROR(kagome::common, ByteStream::AdvanceErrc)

#endif  // KAGOME_CORE_COMMON_BYTE_STREAM_HPP_
