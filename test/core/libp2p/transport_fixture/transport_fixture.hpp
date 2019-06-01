/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TRANSPORT_FIXTURE_HPP
#define KAGOME_TRANSPORT_FIXTURE_HPP

#include <memory>

#include <gtest/gtest.h>
#include <boost/asio/io_context.hpp>
#include "libp2p/multi/multiaddress.hpp"
#include "libp2p/transport/transport.hpp"
#include "libp2p/transport/transport_listener.hpp"
#include "mock/libp2p/transport/upgrader_mock.hpp"
//#include "testutil/stream_operators.hpp"

namespace libp2p::testing {
  /**
   * Support class, allowing to have a preset TCP connection
   */
  class TransportFixture : public ::testing::Test {
   public:
    TransportFixture();

    void SetUp() override;

    /**
     * Provide functions to be executed as a server side of the connection
     * @param on_success - function to be executed in case of success
     * @param on_error  - function to be executed in case of failure
     */
    void server(const transport::TransportListener::HandlerFunc &on_success,
                const transport::TransportListener::ErrorFunc &on_error);

    /**
     * Provide functions to be executed as a client side of the connection
     * @param on_success - function to be executed in case of success
     * @param on_error  - function to be executed in case of failure
     */
    void client(const transport::TransportListener::HandlerFunc &on_success,
                const transport::TransportListener::ErrorFunc &on_error);

    /**
     * Run the context for some time, enough to execute async operations
     */
    void launchContext();

    /**
     * Create a connection upgrader, which is going to do nothing with the
     * connection
     * @return upgrader
     */
    static auto makeUpgrader();

   private:
    boost::asio::io_context context_;
    boost::asio::io_context::executor_type executor_;

    std::shared_ptr<libp2p::transport::Transport> transport_;
    std::shared_ptr<libp2p::transport::TransportListener> transport_listener_;
    std::shared_ptr<libp2p::multi::Multiaddress> multiaddress_;
  };
}  // namespace libp2p::testing

#endif  // KAGOME_TRANSPORT_FIXTURE_HPP
