/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/basic/protobuf_message_read_writer.hpp"

#include <boost/assert.hpp>

namespace libp2p::basic {
  ProtobufMessageReadWriter::ProtobufMessageReadWriter(
      std::shared_ptr<MessageReadWriter> read_writer)
      : read_writer_{std::move(read_writer)} {
    BOOST_ASSERT(read_writer_);
  }
}  // namespace libp2p::basic
