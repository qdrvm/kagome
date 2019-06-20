/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TRANSPORT_FIXTURE_HPP
#define KAGOME_TRANSPORT_FIXTURE_HPP

#include <chrono>
#include <memory>

#include <gtest/gtest.h>
#include <boost/asio/io_context.hpp>
#include "libp2p/multi/multiaddress.hpp"
#include "libp2p/transport/transport.hpp"
#include "libp2p/transport/transport_listener.hpp"
#include "mock/libp2p/transport/upgrader_mock.hpp"

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
     * @param handler - function to be executed on new connection
     */
    void server(const transport::TransportListener::HandlerFunc &handler);

    /**
     * Provide functions to be executed as a client side of the connection
     * @param handler - function to be executed on
     */
    void client(const transport::TransportListener::HandlerFunc &handler);

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

   protected:
    boost::asio::io_context context_;  // NOLINT

   private:
    std::shared_ptr<libp2p::transport::Transport> transport_;
    std::shared_ptr<libp2p::transport::TransportListener> transport_listener_;
    std::shared_ptr<libp2p::multi::Multiaddress> multiaddress_;
  };
}  // namespace libp2p::testing

#endif  // KAGOME_TRANSPORT_FIXTURE_HPP
