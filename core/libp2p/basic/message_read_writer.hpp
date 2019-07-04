/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_MESSAGE_READ_WRITER_HPP
#define KAGOME_MESSAGE_READ_WRITER_HPP

#include <memory>

#include <outcome/outcome.hpp>
#include "libp2p/basic/readwriter.hpp"

namespace libp2p::basic {
  enum class MessageReadWriterError { VARINT_EXPECTED = 1 };

  /**
   * Allows to read and write messages of (\tparam Message) type
   * @tparam Message - type of the message; for now, works with Protobuf
   * messages only
   */
  template <typename Message>
  class MessageReadWriter
      : public std::enable_shared_from_this<MessageReadWriter<Message>> {
   public:
    void read(std::shared_ptr<ReadWriter> conn,
              std::function<void(outcome::result<Message>)> cb);

    void write(const std::shared_ptr<ReadWriter> &conn, Message &&msg,
               Writer::WriteCallbackFunc cb);
  };
}  // namespace libp2p::basic

OUTCOME_HPP_DECLARE_ERROR(libp2p::basic, MessageReadWriterError)

#endif  // KAGOME_MESSAGE_READ_WRITER_HPP
