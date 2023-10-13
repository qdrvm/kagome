/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/helpers/scale_message_read_writer.hpp"

namespace kagome::network {
  ScaleMessageReadWriter::ScaleMessageReadWriter(
      std::shared_ptr<libp2p::basic::MessageReadWriter> read_writer)
      : read_writer_{std::move(read_writer)} {}

  ScaleMessageReadWriter::ScaleMessageReadWriter(
      const std::shared_ptr<libp2p::basic::ReadWriter> &read_writer)
      : read_writer_{std::make_shared<libp2p::basic::MessageReadWriterUvarint>(
          read_writer)} {}
}  // namespace kagome::network
