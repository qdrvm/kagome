/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_MESSAGE_READER_HPP
#define KAGOME_MESSAGE_READER_HPP

#include <memory>

#include "libp2p/protocol_muxer/multiselect/connection_state.hpp"

namespace libp2p::protocol_muxer {
  class Multiselect;

  /**
   * Reads messages of Multiselect format
   */
  class MessageReader {
   public:
    /**
     * Read next Multistream message
     * @param connection_state - state of the connection
     * @note will call Multiselect->onReadCompleted(..) after successful read
     */
    static void readNextMessage(ConnectionState connection_state);

   private:
    /**
     * Read next varint from the connection
     * @param connection_state - state of the connection
     */
    static void readNextVarint(ConnectionState connection_state);

    /**
     * Completion handler of varint read operation
     * @param connection_state - state of the connection
     */
    static void onReadVarintCompleted(ConnectionState connection_state);

    /**
     * Read specified number of bytes from the connection
     * @param connection_state - state of the connection
     * @param bytes_to_read - how much bytes are to be read
     * @param final_callback - in case of success, this callback is called
     */
    static void readNextBytes(
        ConnectionState connection_state, uint64_t bytes_to_read,
        std::function<void(ConnectionState)> final_callback);

    /**
     * Completion handler for read bytes operation in case a single line was
     * expected to be read
     * @param connection_state - state of the connection
     * @param read_bytes - how much bytes were read (or in this line)
     */
    static void onReadLineCompleted(ConnectionState connection_state,
                                    uint64_t read_bytes);

    /**
     * Completion handler for read bytes operation in case several lines (with
     * protocols) were expected to be read
     * @param connection_state - state of the connection
     * @param expected_protocols_number - how much protocols were to be read
     */
    static void onReadProtocolsCompleted(ConnectionState connection_state,
                                         uint64_t expected_protocols_number);
  };
}  // namespace libp2p::protocol_muxer

#endif  // KAGOME_MESSAGE_READER_HPP
