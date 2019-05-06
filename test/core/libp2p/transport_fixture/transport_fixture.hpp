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
     *    - default multiaddress
     */
    void init();

    /**
     * Dial to the default transport via the default multiaddress with a local
     * listener; that listener MUST be created beforehand from the local
     * transport
     */
    void defaultDial();

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

    boost::asio::io_context context_;

    std::unique_ptr<libp2p::transport::Transport> transport_;
    std::shared_ptr<libp2p::transport::TransportListener> transport_listener_;
    std::shared_ptr<libp2p::multi::Multiaddress> multiaddress_;
    std::shared_ptr<libp2p::transport::Connection> connection_;
  };
}  // namespace libp2p::testing

#endif  // KAGOME_TRANSPORT_FIXTURE_HPP
