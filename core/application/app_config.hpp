/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_APP_CONFIG_HPP
#define KAGOME_APP_CONFIG_HPP

#include <spdlog/spdlog.h>
#include <boost/asio/ip/tcp.hpp>
#include <memory>
#include <string>

namespace kagome::application {

  /**
   * Parse and store application config.
   */
  class AppConfiguration {
   public:
    enum struct LoadScheme {
      kBlockProducing,
      kValidating,
      kFullSyncing,
    };

   public:
    virtual ~AppConfiguration() = default;

    /**
     * @return file path with genesis configuration.
     */
    virtual const std::string &genesis_path() const = 0;

    /**
     * @return keystore directory path.
     */
    virtual const std::string &keystore_path() const = 0;

    /**
     * @return leveldb directory path.
     */
    virtual const std::string &leveldb_path() const = 0;

    /**
     * @return port for peer to peer interactions.
     */
    virtual uint16_t p2p_port() const = 0;

    /**
     * @return endpoint for RPC over HTTP.
     */
    virtual const boost::asio::ip::tcp::endpoint &rpc_http_endpoint() const = 0;

    /**
     * @return endpoint for RPC over Websocket protocol.
     */
    virtual const boost::asio::ip::tcp::endpoint &rpc_ws_endpoint() const = 0;

    /**
     * @return log level (0-trace, 5-only critical, 6-no logs).
     */
    virtual spdlog::level::level_enum verbosity() const = 0;

    /**
     * @return true if node in only finalizing mode, otherwise false.
     */
    virtual bool is_only_finalizing() const = 0;

    /**
     * If whole nodes was stopped, would not any active node to synchronize.
     * This option gives ability to continue block production at cold start.
     * @return true if need to force block production
     */
    virtual bool is_already_synchronized() const = 0;
  };

  using AppConfigPtr = std::shared_ptr<AppConfiguration>;

}  // namespace kagome::application

#endif  // KAGOME_APP_CONFIG_HPP
