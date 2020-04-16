/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 *
 * Kagome is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * Kagome is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef LIBP2P_MESSAGE_READ_WRITER_HELPER_HPP
#define LIBP2P_MESSAGE_READ_WRITER_HELPER_HPP

#include <memory>
#include <vector>

#include "mock/libp2p/basic/read_writer_mock.hpp"
#include "mock/libp2p/connection/stream_mock.hpp"

/**
 * Basically, methods in this file do the same thing as
 * libp2p::basic::MessageReadWriter, but with a mock
 */

namespace libp2p::basic {
  /**
   * Set read expectations for the provided (\param read_writer_mock) to read
   * (\param msg) with a prepended varint
   */
  void setReadExpectations(
      const std::shared_ptr<ReadWriterMock> &read_writer_mock,
      const std::vector<uint8_t> &msg);
  void setReadExpectations(
      const std::shared_ptr<connection::StreamMock> &stream_mock,
      const std::vector<uint8_t> &msg);

  /**
   * Set write expectations for the provided (\param read_writer_mock) to write
   * (\param msg) with a prepended varint
   */
  void setWriteExpectations(
      const std::shared_ptr<ReadWriterMock> &read_writer_mock,
      std::vector<uint8_t> msg);
  void setWriteExpectations(
      const std::shared_ptr<connection::StreamMock> &stream_mock,
      std::vector<uint8_t> msg);
}  // namespace libp2p::basic

#endif  // LIBP2P_MESSAGE_READ_WRITER_HELPER_HPP
