/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "testutil/libp2p/message_read_writer_helper.hpp"

#include "libp2p/multi/uvarint.hpp"

ACTION_P(PutBytes, bytes) {  // NOLINT
  std::copy(bytes.begin(), bytes.end(), arg0.begin());
  arg2(bytes.size());
}

ACTION_P(CheckBytes, bytes) {  // NOLINT
  ASSERT_EQ(arg0, bytes);      // NOLINT
  arg2(bytes.size());
}

namespace libp2p::basic {
  using multi::UVarint;
  using testing::_;

  void setReadExpectations(
      const std::shared_ptr<connection::StreamMock> &stream_mock,
      const std::vector<uint8_t> &msg) {
    // read varint
    UVarint varint_to_read{msg.size()};
    for (size_t i = 0; i < varint_to_read.size(); ++i) {
      EXPECT_CALL(*stream_mock, read(_, 1, _))
          .WillOnce(PutBytes(varint_to_read.toBytes().subspan(i, 1)));
    }

    // read message
    EXPECT_CALL(*stream_mock, read(_, msg.size(), _)).WillOnce(PutBytes(msg));
  }

  void setWriteExpectations(
      const std::shared_ptr<connection::StreamMock> &stream_mock,
      const std::vector<uint8_t> &msg) {
    // write varint
    UVarint varint_to_write{msg.size()};
    for (size_t i = 0; i < varint_to_write.size(); ++i) {
      EXPECT_CALL(*stream_mock, write(_, 1, _))  // NOLINT
          .WillOnce(CheckBytes(varint_to_write.toBytes().subspan(i, 1)));
    }

    // write message
    EXPECT_CALL(*stream_mock, write(_, msg.size(), _))
        .WillOnce(CheckBytes(gsl::make_span(msg)));
  }
}  // namespace libp2p::basic
