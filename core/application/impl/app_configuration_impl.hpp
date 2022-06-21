/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_APP_CONFIGURATION_IMPL_HPP
#define KAGOME_APP_CONFIGURATION_IMPL_HPP

#include "application/app_configuration.hpp"

#define RAPIDJSON_NO_SIZETYPEDEFINE
namespace rapidjson {
  using SizeType = ::std::size_t;
}
#include <rapidjson/document.h>
#undef RAPIDJSON_NO_SIZETYPEDEFINE

#include <array>
#include <cstdio>
#include <memory>

#include "log/logger.hpp"

#ifdef DECLARE_PROPERTY
#error DECLARE_PROPERTY already defined!
#endif  // DECLARE_PROPERTY
#define DECLARE_PROPERTY(T, N)                                                 \
 private:                                                                      \
  T N##_;                                                                      \
                                                                               \
 public:                                                                       \
  std::conditional<std::is_trivial<T>::value && (sizeof(T) <= sizeof(size_t)), \
                   T,                                                          \
                   const T &>::type                                            \
  N() const override {                                                         \
    return N##_;                                                               \
  }

namespace kagome::application {

  // clang-format off
  /**
   * Reads app configuration from multiple sources with the given priority:
   *
   *      COMMAND LINE ARGUMENTS          <- max priority
   *                V
   *        CONFIGURATION FILE
   *                V
   *          DEFAULT VALUES              <- low priority
   */
  // clang-format on

  class AppConfigurationImpl final : public AppConfiguration {
    using FilePtr = std::unique_ptr<std::FILE, decltype(&std::fclose)>;

   public:
    explicit AppConfigurationImpl(log::Logger logger);
    ~AppConfigurationImpl() override = default;

    AppConfigurationImpl(const AppConfigurationImpl &) = delete;
    AppConfigurationImpl &operator=(const AppConfigurationImpl &) = delete;

    AppConfigurationImpl(AppConfigurationImpl &&) = default;
    AppConfigurationImpl &operator=(AppConfigurationImpl &&) = default;

    [[nodiscard]] bool initializeFromArgs(int argc, const char **argv);

    network::Roles roles() const override {
      return roles_;
    }
    boost::filesystem::path chainSpecPath() const override;
    boost::filesystem::path cachedRuntimePath(
        std::string runtime_hash) const override;
    boost::filesystem::path chainPath(std::string chain_id) const override;
    boost::filesystem::path databasePath(std::string chain_id) const override;
    boost::filesystem::path keystorePath(std::string chain_id) const override;

    const std::optional<crypto::Ed25519PrivateKey> &nodeKey() const override {
      return node_key_;
    }

    const std::optional<std::string> &nodeKeyFile() const override {
      return node_key_file_;
    };

    const std::vector<libp2p::multi::Multiaddress> &listenAddresses()
        const override {
      return listen_addresses_;
    }

    const std::vector<libp2p::multi::Multiaddress> &publicAddresses()
        const override {
      return public_addresses_;
    }

    const std::vector<libp2p::multi::Multiaddress> &bootNodes() const override {
      return boot_nodes_;
    }

    uint16_t p2pPort() const override {
      return p2p_port_;
    }
    uint32_t outPeers() const override {
      return out_peers_;
    }
    uint32_t inPeers() const override {
      return in_peers_;
    }
    uint32_t inPeersLght() const override {
      return in_peers_light_;
    }
    const boost::asio::ip::tcp::endpoint &rpcHttpEndpoint() const override {
      return rpc_http_endpoint_;
    }
    const boost::asio::ip::tcp::endpoint &rpcWsEndpoint() const override {
      return rpc_ws_endpoint_;
    }
    const boost::asio::ip::tcp::endpoint &openmetricsHttpEndpoint()
        const override {
      return openmetrics_http_endpoint_;
    }
    uint32_t maxWsConnections() const override {
      return max_ws_connections_;
    }
    const std::vector<std::string> &log() const override {
      return logger_tuning_config_;
    }
    uint32_t maxBlocksInResponse() const override {
      return max_blocks_in_response_;
    }
    const network::PeeringConfig &peeringConfig() const override {
      return peering_config_;
    }
    bool isRunInDevMode() const override {
      return dev_mode_;
    }
    const std::string &nodeName() const override {
      return node_name_;
    }
    const std::string &nodeVersion() const override {
      return node_version_;
    }
    bool isTelemetryEnabled() const override {
      return is_telemetry_enabled_;
    }
    const std::vector<telemetry::TelemetryEndpoint> &telemetryEndpoints()
        const override {
      return telemetry_endpoints_;
    }
    RuntimeExecutionMethod runtimeExecMethod() const override {
      return runtime_exec_method_;
    }
    bool useWavmCache() const override {
      return use_wavm_cache_;
    }
    OffchainWorkerMode offchainWorkerMode() const override {
      return offchain_worker_mode_;
    }
    bool isOffchainIndexingEnabled() const override {
      return enable_offchain_indexing_;
    }
    virtual std::optional<primitives::BlockId> recoverState() const override {
      return recovery_state_;
    }

   private:
    void parse_general_segment(rapidjson::Value &val);
    void parse_blockchain_segment(rapidjson::Value &val);
    void parse_storage_segment(rapidjson::Value &val);
    void parse_network_segment(rapidjson::Value &val);
    void parse_additional_segment(rapidjson::Value &val);

    /// TODO(iceseer): PRE-476 make handler calls via lambda-calls, remove
    /// member-function ptrs
    struct SegmentHandler {
      using Handler = std::function<void(rapidjson::Value &)>;
      char const *segment_name;
      Handler handler;
    };

    // clang-format off
    std::vector<SegmentHandler> handlers_ = {
        SegmentHandler{"general",    std::bind(&AppConfigurationImpl::parse_general_segment, this, std::placeholders::_1)},
        SegmentHandler{"blockchain", std::bind(&AppConfigurationImpl::parse_blockchain_segment, this, std::placeholders::_1)},
        SegmentHandler{"storage",    std::bind(&AppConfigurationImpl::parse_storage_segment, this, std::placeholders::_1)},
        SegmentHandler{"network",    std::bind(&AppConfigurationImpl::parse_network_segment, this, std::placeholders::_1)},
        SegmentHandler{"additional", std::bind(&AppConfigurationImpl::parse_additional_segment, this, std::placeholders::_1)},
    };
    // clang-format on

    bool validate_config();

    void read_config_from_file(const std::string &filepath);

    bool load_ms(const rapidjson::Value &val,
                 char const *name,
                 std::vector<std::string> &target);

    bool load_ma(const rapidjson::Value &val,
                 char const *name,
                 std::vector<libp2p::multi::Multiaddress> &target);
    bool load_telemetry_uris(const rapidjson::Value &val,
                             char const *name,
                             std::vector<telemetry::TelemetryEndpoint> &target);
    bool load_str(const rapidjson::Value &val,
                  char const *name,
                  std::string &target);
    bool load_u16(const rapidjson::Value &val,
                  char const *name,
                  uint16_t &target);
    bool load_u32(const rapidjson::Value &val,
                  char const *name,
                  uint32_t &target);
    bool load_bool(const rapidjson::Value &val, char const *name, bool &target);

    /**
     * Convert given values into boost tcp::endpoint representation format
     * @param host - host name
     * @param port - port value
     * @return boost tcp::endpoint constructed
     */
    boost::asio::ip::tcp::endpoint getEndpointFrom(const std::string &host,
                                                   uint16_t port) const;

    /**
     * Convert a given libp2p multiaddress into a boost tcp::endpoint format.
     * @param multiaddress - an address to be converted. Should contain a valid
     * interface name or IP4/IP6 address and a port value to listen on.
     * @return boost tcp::endpoint when well-formed multiaddress is passed,
     * otherwise - an error
     */
    outcome::result<boost::asio::ip::tcp::endpoint> getEndpointFrom(
        const libp2p::multi::Multiaddress &multiaddress) const;

    /**
     * Checks whether configured listen addresses are available.
     * @return true when addresses are available, false - when at least one
     * address is not available
     */
    bool testListenAddresses() const;

    /**
     * Parses telemetry endpoint URI and verbosity level from a single string
     * record of format: "<endpoint URI> <verbosity: 0-9>"
     * @param record - input string
     * @return - constructed instance of kagome::telemetry::TelemetryEndpoint or
     * std::nullopt in case of error
     */
    std::optional<telemetry::TelemetryEndpoint> parseTelemetryEndpoint(
        const std::string &record) const;

    FilePtr open_file(const std::string &filepath);

    log::Logger logger_;

    network::Roles roles_;
    std::optional<crypto::Ed25519PrivateKey> node_key_;
    std::optional<std::string> node_key_file_;
    std::vector<libp2p::multi::Multiaddress> listen_addresses_;
    std::vector<libp2p::multi::Multiaddress> public_addresses_;
    std::vector<libp2p::multi::Multiaddress> boot_nodes_;
    std::vector<telemetry::TelemetryEndpoint> telemetry_endpoints_;
    bool is_telemetry_enabled_;
    uint16_t p2p_port_;
    boost::asio::ip::tcp::endpoint rpc_http_endpoint_;
    boost::asio::ip::tcp::endpoint rpc_ws_endpoint_;
    boost::asio::ip::tcp::endpoint openmetrics_http_endpoint_;
    std::vector<std::string> logger_tuning_config_;
    uint32_t max_blocks_in_response_;
    std::string rpc_http_host_;
    std::string rpc_ws_host_;
    std::string openmetrics_http_host_;
    boost::filesystem::path chain_spec_path_;
    boost::filesystem::path base_path_;
    uint16_t rpc_http_port_;
    uint16_t rpc_ws_port_;
    uint16_t openmetrics_http_port_;
    uint32_t out_peers_;
    uint32_t in_peers_;
    uint32_t in_peers_light_;
    network::PeeringConfig peering_config_;
    bool dev_mode_;
    std::string node_name_;
    std::string node_version_;
    uint32_t max_ws_connections_;
    RuntimeExecutionMethod runtime_exec_method_;
    bool use_wavm_cache_;
    OffchainWorkerMode offchain_worker_mode_;
    bool enable_offchain_indexing_;
    std::optional<primitives::BlockId> recovery_state_;
  };

}  // namespace kagome::application

#undef DECLARE_PROPERTY

#endif  // KAGOME_APP_CONFIGURATION_IMPL_HPP
