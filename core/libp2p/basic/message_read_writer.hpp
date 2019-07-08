/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_MESSAGE_READ_WRITER_HPP
#define KAGOME_MESSAGE_READ_WRITER_HPP

#include <memory>

#include <gsl/span>
#include <outcome/outcome.hpp>
#include "libp2p/basic/readwriter.hpp"

namespace libp2p::basic {
  /**
   * Allows to read and write messages, which are prepended with a varint -
   * standard, for example, for Protobuf messages in Libp2p
   */
  class MessageReadWriter
      : public std::enable_shared_from_this<MessageReadWriter> {
   public:
    /**
     * Create an instance of MessageReadWriter
     * @param conn, from which to read/write messages
     */
    explicit MessageReadWriter(std::shared_ptr<ReadWriter> conn);

    /**
     * Read a message, which is prepended with a varint
     * @param buffer, to which the message is to be read; MUST be big enough for
     * that message
     * @param cb, which is called, when the message is read or error happens
     * @note if MessageReadWriterError::LITTLE_BUFFER happens, the second
     * argument of the callback will be size of the message, which is to be
     * read; as varint will already be read from the connection by that time,
     * you can just use simple conn->read(..) method
     */
    void read(gsl::span<uint8_t> buffer,
              std::function<void(outcome::result<size_t>, size_t)> cb);

    /**
     * Write a message; varint with its length will be prepended to it
     * @param buffer - the message to be written
     * @param cb, which is called, when the message is read or error happens
     */
    void write(gsl::span<const uint8_t> buffer, Writer::WriteCallbackFunc cb);

   private:
    std::shared_ptr<ReadWriter> conn_;
  };
}  // namespace libp2p::basic

#endif  // KAGOME_MESSAGE_READ_WRITER_HPP
