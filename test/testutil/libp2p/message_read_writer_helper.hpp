/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_MESSAGE_READ_WRITER_HELPER_HPP
#define KAGOME_MESSAGE_READ_WRITER_HELPER_HPP

#include <memory>
#include <vector>

#include "mock/libp2p/connection/stream_mock.hpp"

/**
 * Basically, methods in this file do the same thing as
 * libp2p::basic::MessageReadWriter, but with a stream mock
 */

namespace libp2p::basic {
  /**
   * Set read expectations for the provided (\param stream_mock) to read (\param
   * msg) with a prepended varint
   */
  void setReadExpectations(
      const std::shared_ptr<connection::StreamMock> &stream_mock,
      const std::vector<uint8_t> &msg);

  /**
   * Set write expectations for the provided (\param stream_mock) to write
   * (\param msg) with a prepended varint
   */
  void setWriteExpectations(
      const std::shared_ptr<connection::StreamMock> &stream_mock,
      std::vector<uint8_t> msg);
}  // namespace libp2p::basic

#endif  // KAGOME_MESSAGE_READ_WRITER_HELPER_HPP
