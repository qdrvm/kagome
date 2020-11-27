/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_APP_CONFIGURATION_HPP
#define KAGOME_APP_CONFIGURATION_HPP

#include <memory>
#include <string>

#include <spdlog/common.h>
#include <boost/asio/ip/tcp.hpp>
#include <boost/filesystem.hpp>

namespace kagome::application {

  /**
   * Parse and store application config.
   */
  class AppConfiguration {
   public:
    static constexpr uint32_t kAbsolutMinBlocksInResponse = 10;
    static constexpr uint32_t kAbsolutMaxBlocksInResponse = 128;

    static_assert(kAbsolutMinBlocksInResponse <= kAbsolutMaxBlocksInResponse,
                  "Check max and min page bounding values!");

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
    virtual boost::filesystem::path genesis_path() const = 0;

    /**
     * @return path to the node's directory for the chain \arg chain_id
     * (contains key storage and database)
     */
    virtual boost::filesystem::path chain_path(std::string chain_id) const = 0;

    /**
     * @return path to the node's database for the chain \arg chain_id
     */
    virtual boost::filesystem::path database_path(
        std::string chain_id) const = 0;

    /**
     * @return path to the node's keystore for the chain \arg chain_id
     */
    virtual boost::filesystem::path keystore_path(
        std::string chain_id) const = 0;

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

    /**
     * @return max blocks count per response while syncing
     */
    virtual uint32_t max_blocks_in_response() const = 0;

    /**
     * Slots strategy
     * @return true if we should count slots as `unix_epoch_time() / slot_duration`.
     * Otherwise slots are counting from 0 and false is returned
     */
    virtual bool is_unix_slots_strategy() const = 0;
  };

}  // namespace kagome::application

#endif  // KAGOME_APP_CONFIGURATION_HPP
