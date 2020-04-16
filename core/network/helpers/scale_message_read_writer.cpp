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
