/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/basic/scale_message_read_writer.hpp"

namespace libp2p::basic {
  ScaleMessageReadWriter::ScaleMessageReadWriter(
      std::shared_ptr<MessageReadWriter> read_writer)
      : read_writer_{std::move(read_writer)} {}

  ScaleMessageReadWriter::ScaleMessageReadWriter(
      const std::shared_ptr<ReadWriter> &read_writer)
      : read_writer_{std::make_shared<MessageReadWriter>(read_writer)} {}
}  // namespace libp2p::basic
