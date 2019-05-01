/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TRANSPORT_FIXTURE_HPP
#define KAGOME_TRANSPORT_FIXTURE_HPP

#include <memory>

#include <gtest/gtest.h>
#include <boost/asio/io_context.hpp>
#include <testutil/outcome.hpp>
#include "libp2p/muxer/yamux.hpp"
#include "libp2p/stream/stream.hpp"
#include "libp2p/transport/connection.hpp"
#include "libp2p/transport/transport.hpp"
#include "libp2p/transport/transport_listener.hpp"

namespace libp2p::testing {
  /**
   * Support class, allowing to have a Yamuxed preset TCP connection
   */
  class TransportFixture : public ::testing::Test {
   public:
    void SetUp() override;

    /**
     * Initialize:
     *    - transport
     *    - listener, which wraps new connections into Yamux
     *    - multiaddress, which is listened by the created transport
     *    - Yamux itself
     */
    void init();

    /**
     * Run the context for some time to execute async operations
     */
    void launchContext();

    /**
     * Check that IO operation has finished successfully
     * @param ec returned from the operation
     * @param received_size - how much bytes were received
     * @param expected_size - how much bytes were expected to be received
     */
    static void checkIOSuccess(const std::error_code &ec, size_t received_size,
                               size_t expected_size);

    /**
     * Get a pointer to a new stream
     * @param expected_stream_id - id, which is expected to be assigned to that
     * stream
     * @return pointer to the stream
     */
    std::unique_ptr<libp2p::stream::Stream> getNewStream(
        libp2p::muxer::Yamux::StreamId expected_stream_id =
            kDefaulExpectedStreamId);

    boost::asio::io_context context_;

    std::shared_ptr<libp2p::muxer::Yamux> yamux_;
    std::vector<std::unique_ptr<libp2p::stream::Stream>> accepted_streams_;

    std::unique_ptr<libp2p::transport::Transport> transport_;
    std::shared_ptr<libp2p::transport::TransportListener> transport_listener_;
    std::shared_ptr<libp2p::multi::Multiaddress> multiaddress_;
    std::shared_ptr<libp2p::transport::Connection> connection_;

    static constexpr libp2p::muxer::Yamux::StreamId kDefaulExpectedStreamId = 2;
  };
}  // namespace libp2p::testing

#endif  // KAGOME_TRANSPORT_FIXTURE_HPP
