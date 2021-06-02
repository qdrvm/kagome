/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_APP_CONFIGURATION_HPP
#define KAGOME_APP_CONFIGURATION_HPP

#include <memory>
#include <string>

#include <boost/asio/ip/tcp.hpp>
#include <boost/filesystem.hpp>
#include <boost/optional.hpp>
#include <libp2p/multi/multiaddress.hpp>

#include "crypto/ed25519_types.hpp"
#include "log/logger.hpp"
#include "network/peering_config.hpp"
#include "network/types/roles.hpp"

namespace kagome::application {

  /**
   * Parse and store application config.
   */
  class AppConfiguration {
   public:
    static constexpr uint32_t kAbsolutMinBlocksInResponse = 10;
    static constexpr uint32_t kAbsolutMaxBlocksInResponse = 128;
    static constexpr uint32_t kNodeNameMaxLength = 64;

    static_assert(kAbsolutMinBlocksInResponse <= kAbsolutMaxBlocksInResponse,
                  "Check max and min page bounding values!");

    virtual ~AppConfiguration() = default;

    /**
     * @return roles of current run
     */
    virtual network::Roles roles() const = 0;

    /**
     * @return file path with genesis configuration.
     */
    virtual boost::filesystem::path chainSpecPath() const = 0;

    /**
     * @return path to the node's directory for the chain \arg chain_id
     * (contains key storage and database)
     */
    virtual boost::filesystem::path chainPath(std::string chain_id) const = 0;

    /**
     * @return path to the node's database for the chain \arg chain_id
     */
    virtual boost::filesystem::path databasePath(
        std::string chain_id) const = 0;

    /**
     * @return path to the node's keystore for the chain \arg chain_id
     */
    virtual boost::filesystem::path keystorePath(
        std::string chain_id) const = 0;

    /**
     * @return the secret key to use for libp2p networking
     */
    virtual const boost::optional<crypto::Ed25519PrivateKey> &nodeKey()
        const = 0;

    /**
     * @return the path to key used for libp2p networking
     */
    virtual const boost::optional<std::string> &nodeKeyFile() const = 0;

    /**
     * @return port for peer to peer interactions.
     */
    virtual uint16_t p2pPort() const = 0;

    /**
     * @return multiaddresses of bootstrat nodes
     */
    virtual const std::vector<libp2p::multi::Multiaddress> &bootNodes()
        const = 0;

    /**
     * @return multiaddresses the node listens for open connections on
     */
    virtual const std::vector<libp2p::multi::Multiaddress> &listenAddresses()
        const = 0;

    /**
     * @return multiaddresses the node could be accessed from the network
     */
    virtual const std::vector<libp2p::multi::Multiaddress> &publicAddresses()
        const = 0;

    /**
     * @return endpoint for RPC over HTTP.
     */
    virtual const boost::asio::ip::tcp::endpoint &rpcHttpEndpoint() const = 0;

    /**
     * @return endpoint for RPC over Websocket protocol.
     */
    virtual const boost::asio::ip::tcp::endpoint &rpcWsEndpoint() const = 0;

    /**
     * @return endpoint for OpenMetrics over HTTP protocol.
     */
    virtual const boost::asio::ip::tcp::endpoint &openmetricsHttpEndpoint() const = 0;

    /**
     * @return maximum number of WS RPC connections
     */
    virtual uint32_t maxWsConnections() const = 0;

    /**
     * @return log level (0-trace, 5-only critical, 6-no logs).
     */
    virtual log::Level verbosity() const = 0;

    /**
     * If whole nodes was stopped, would not any active node to synchronize.
     * This option gives ability to continue block production at cold start.
     * @return true if need to force block production
     */
    virtual bool isAlreadySynchronized() const = 0;

    /**
     * @return max blocks count per response while syncing
     */
    virtual uint32_t maxBlocksInResponse() const = 0;

    /**
     * Slots strategy
     * @return true if we should count slots as `unix_epoch_time() /
     * slot_duration`. Otherwise slots are counting from 0 and false is returned
     */
    virtual bool isUnixSlotsStrategy() const = 0;

    /**
     * Config for PeerManager
     */
    virtual const network::PeeringConfig &peeringConfig() const = 0;

    /**
     * @return true if node allowed to run in development mode
     */
    virtual bool isRunInDevMode() const = 0;

    /**
     * @return string representation of human-readable node name.
     * The name of node is going to be used in telemetry, etc.
     */
    virtual const std::string &nodeName() const = 0;
  };

}  // namespace kagome::application

#endif  // KAGOME_APP_CONFIGURATION_HPP
