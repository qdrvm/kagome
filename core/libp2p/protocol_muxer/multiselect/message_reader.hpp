/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_MESSAGE_READER_HPP
#define KAGOME_MESSAGE_READER_HPP

#include <memory>

#include <outcome/outcome.hpp>
#include "libp2p/basic/readwritecloser.hpp"
#include "libp2p/protocol_muxer/multiselect/message_manager.hpp"

namespace libp2p::protocol_muxer {
  /**
   * Reads messages of Multiselect format
   */
  class MessageReader {
   public:
    /**
     * Read next Multistream message
     * @param connection to read from
     * @return message in case of success, error otherwise
     */
    static outcome::result<MessageManager::MultiselectMessage> readNextMessage(
        const std::shared_ptr<basic::ReadWriteCloser> &connection);

    enum class ReaderError { VARINT_EXPECTED = 1, MSG_TOO_SHORT };
  };
}  // namespace libp2p::protocol_muxer

OUTCOME_HPP_DECLARE_ERROR(libp2p::protocol_muxer, MessageReader::ReaderError)

#endif  // KAGOME_MESSAGE_READER_HPP
