
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

#include "testutil/libp2p/message_read_writer_helper.hpp"

#include "libp2p/multi/uvarint.hpp"

ACTION_P(PutBytes, bytes) {  // NOLINT
  std::copy(bytes.begin(), bytes.end(), arg0.begin());
  arg2(bytes.size());
}

ACTION_P(CheckBytes, bytes) {                                          // NOLINT
  ASSERT_EQ((std::vector<uint8_t>{arg0.begin(), arg0.end()}), bytes);  // NOLINT
  arg2(bytes.size());
}

namespace libp2p::basic {
  using multi::UVarint;
  using testing::_;

  void setReadExpectations(
      const std::shared_ptr<ReadWriterMock> &read_writer_mock,
      const std::vector<uint8_t> &msg) {
    // read varint
    UVarint varint_to_read{msg.size()};
    for (size_t i = 0; i < varint_to_read.size(); ++i) {
      EXPECT_CALL(*read_writer_mock, read(_, 1, _))
          .WillOnce(
              PutBytes(std::vector<uint8_t>{varint_to_read.toVector()[i]}));
    }

    // read message
    EXPECT_CALL(*read_writer_mock, read(_, msg.size(), _))
        .WillOnce(PutBytes(msg));
  }

  void setReadExpectations(
      const std::shared_ptr<connection::StreamMock> &stream_mock,
      const std::vector<uint8_t> &msg) {
    // read varint
    UVarint varint_to_read{msg.size()};
    for (size_t i = 0; i < varint_to_read.size(); ++i) {
      EXPECT_CALL(*stream_mock, read(_, 1, _))
          .WillOnce(
              PutBytes(std::vector<uint8_t>{varint_to_read.toVector()[i]}));
    }

    // read message
    EXPECT_CALL(*stream_mock, read(_, msg.size(), _)).WillOnce(PutBytes(msg));
  }

  void setWriteExpectations(
      const std::shared_ptr<ReadWriterMock> &read_writer_mock,
      std::vector<uint8_t> msg) {
    UVarint varint_to_write{msg.size()};
    msg.insert(msg.begin(), varint_to_write.toVector().begin(),
               varint_to_write.toVector().end());
    EXPECT_CALL(*read_writer_mock, write(_, msg.size(), _))
        .WillOnce(CheckBytes(msg));
  }

  void setWriteExpectations(
      const std::shared_ptr<connection::StreamMock> &stream_mock,
      std::vector<uint8_t> msg) {
    UVarint varint_to_write{msg.size()};
    msg.insert(msg.begin(), varint_to_write.toVector().begin(),
               varint_to_write.toVector().end());
    EXPECT_CALL(*stream_mock, write(_, msg.size(), _))  // NOLINT
        .WillOnce(CheckBytes(msg));
  }
}  // namespace libp2p::basic
