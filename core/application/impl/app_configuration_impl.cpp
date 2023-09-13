/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "application/impl/app_configuration_impl.hpp"

#include <limits>
#include <regex>
#include <string>

#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <rapidjson/filereadstream.h>
#include <boost/algorithm/string.hpp>
#include <boost/program_options.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <charconv>
#include <libp2p/layer/websocket/wss_adaptor.hpp>

#include "api/transport/tuner.hpp"
#include "application/build_version.hpp"
#include "assets/assets.hpp"
#include "assets/embedded_chainspec.hpp"
#include "chain_spec_impl.hpp"
#include "common/hexutil.hpp"
#include "common/uri.hpp"
#include "filesystem/common.hpp"
#include "filesystem/directories.hpp"
#include "log/formatters/filepath.hpp"
#include "log/formatters/optional.hpp"
#include "utils/read_file.hpp"

namespace {
  namespace fs = kagome::filesystem;
  using kagome::application::AppConfiguration;

  template <typename T, typename Func>
  void find_argument(boost::program_options::variables_map &vm,
                     const char *name,
                     Func &&f) {
    assert(nullptr != name);
    if (auto it = vm.find(name); it != vm.end()) {
      if (it->second.defaulted()) {
        return;
      }
      std::forward<Func>(f)(it->second.as<T>());
    }
  }

  template <typename T>
  std::optional<T> find_argument(boost::program_options::variables_map &vm,
                                 const std::string &name) {
    if (auto it = vm.find(name); it != vm.end()) {
      if (!it->second.defaulted()) {
        return it->second.as<T>();
      }
    }
    return std::nullopt;
  }

  bool find_argument(boost::program_options::variables_map &vm,
                     const std::string &name) {
    if (auto it = vm.find(name); it != vm.end()) {
      if (!it->second.defaulted()) {
        return true;
      }
    }
    return false;
  }

  const std::string def_rpc_host = "0.0.0.0";
  const std::string def_openmetrics_http_host = "0.0.0.0";
  const uint16_t def_rpc_port = 9944;
  const uint16_t def_openmetrics_http_port = 9615;
  const uint32_t def_ws_max_connections = 500;
  const uint16_t def_p2p_port = 30363;
  const bool def_dev_mode = false;
  const kagome::network::Roles def_roles = [] {
    kagome::network::Roles roles;
    roles.flags.full = 1;
    return roles;
  }();
  const auto def_sync_method =
      kagome::application::AppConfiguration::SyncMethod::Full;
  const auto def_runtime_exec_method =
      kagome::application::AppConfiguration::RuntimeExecutionMethod::Interpret;
  const auto def_use_wavm_cache_ = false;
  const auto def_purge_wavm_cache_ = false;
  const auto def_offchain_worker_mode =
      kagome::application::AppConfiguration::OffchainWorkerMode::WhenValidating;
  const bool def_enable_offchain_indexing = false;
  const std::optional<kagome::primitives::BlockId> def_block_to_recover =
      std::nullopt;
  const auto def_offchain_worker = "WhenValidating";
  const uint32_t def_out_peers = 75;
  const uint32_t def_in_peers = 75;
  const uint32_t def_in_peers_light = 100;
  const auto def_lucky_peers = 4;
  const uint32_t def_random_walk_interval = 15;
  const auto def_full_sync = "Full";
  const auto def_wasm_execution = "Interpreted";
  const uint32_t def_db_cache_size = 1024;
  const uint32_t def_parachain_runtime_instance_cache_size = 100;

  /**
   * Generate once at run random node name if form of UUID
   * @return UUID as string value
   */
  const std::string &randomNodeName() {
    static std::string name;
    if (name.empty()) {
      auto uuid = boost::uuids::random_generator()();
      name = boost::uuids::to_string(uuid);
    }
    auto max_len = kagome::application::AppConfiguration::kNodeNameMaxLength;
    if (name.length() > max_len) {
      name = name.substr(0, max_len);
    }
    return name;
  }

  std::optional<kagome::application::AppConfiguration::SyncMethod>
  str_to_sync_method(std::string_view str) {
    using SM = kagome::application::AppConfiguration::SyncMethod;
    if (str == "Full") {
      return SM::Full;
    }
    if (str == "Fast") {
      return SM::Fast;
    }
    if (str == "FastWithoutState") {
      return SM::FastWithoutState;
    }
    if (str == "Warp") {
      return SM::Warp;
    }
    if (str == "Auto") {
      return SM::Auto;
    }
    return std::nullopt;
  }

  std::optional<AppConfiguration::AllowUnsafeRpc> parseAllowUnsafeRpc(
      std::string_view str) {
    if (str == "unsafe") {
      return AppConfiguration::AllowUnsafeRpc::kUnsafe;
    }
    if (str == "safe") {
      return AppConfiguration::AllowUnsafeRpc::kSafe;
    }
    if (str == "auto") {
      return AppConfiguration::AllowUnsafeRpc::kAuto;
    }
    return std::nullopt;
  }

  std::optional<kagome::application::AppConfiguration::RuntimeExecutionMethod>
  str_to_runtime_exec_method(std::string_view str) {
    using REM = kagome::application::AppConfiguration::RuntimeExecutionMethod;
    if (str == "Interpreted") {
      return REM::Interpret;
    }
    if (str == "Compiled") {
      return REM::Compile;
    }
    return std::nullopt;
  }

  std::optional<kagome::application::AppConfiguration::OffchainWorkerMode>
  str_to_offchain_worker_mode(std::string_view str) {
    using Mode = kagome::application::AppConfiguration::OffchainWorkerMode;
    if (str == "Always") {
      return Mode::Always;
    }
    if (str == "Never") {
      return Mode::Never;
    }
    if (str == "WhenValidating") {
      return Mode::WhenValidating;
    }
    return std::nullopt;
  }

  std::optional<kagome::primitives::BlockId> str_to_recovery_state(
      std::string_view str) {
    kagome::primitives::BlockNumber bn;
    auto res = kagome::primitives::BlockHash::fromHex(str);
    if (res.has_value()) {
      return {{res.value()}};
    }

    auto result = std::from_chars(str.data(), str.data() + str.size(), bn);
    if (result.ec != std::errc::invalid_argument && std::to_string(bn) == str) {
      return {{bn}};
    }

    return std::nullopt;
  }

  auto &devAccounts() {
    using Account =
        std::tuple<const char *, std::string_view, std::string_view>;
    static const std::array<Account, 8> accounts{
        Account{"alice", "Alice", "//Alice"},
        Account{"bob", "Bob", "//Bob"},
        Account{"charlie", "Charlie", "//Charlie"},
        Account{"dave", "Dave", "//Dave"},
        Account{"eve", "Eve", "//Eve"},
        Account{"ferdie", "Ferdie", "//Ferdie"},
        Account{"one", "One", "//One"},
        Account{"two", "Two", "//Two"},
    };
    return accounts;
  }

  inline bool chainspecExists(const fs::path &path) {
    return kagome::assets::getEmbeddedChainspec(path.native())
        or fs::exists(path);
  }
}  // namespace

namespace kagome::application {

  AppConfigurationImpl::AppConfigurationImpl(log::Logger logger)
      : logger_(std::move(logger)),
        roles_(def_roles),
        save_node_key_(false),
        is_telemetry_enabled_(true),
        p2p_port_(def_p2p_port),
        p2p_port_explicitly_defined_(false),
        max_blocks_in_response_(kAbsolutMaxBlocksInResponse),
        rpc_host_(def_rpc_host),
        openmetrics_http_host_(def_openmetrics_http_host),
        rpc_port_(def_rpc_port),
        openmetrics_http_port_(def_openmetrics_http_port),
        out_peers_(def_out_peers),
        in_peers_(def_in_peers),
        in_peers_light_(def_in_peers_light),
        lucky_peers_(def_lucky_peers),
        dev_mode_(def_dev_mode),
        node_name_(randomNodeName()),
        node_version_(buildVersion()),
        max_ws_connections_(def_ws_max_connections),
        random_walk_interval_(def_random_walk_interval),
        sync_method_{def_sync_method},
        runtime_exec_method_{def_runtime_exec_method},
        use_wavm_cache_(def_use_wavm_cache_),
        purge_wavm_cache_(def_purge_wavm_cache_),
        offchain_worker_mode_{def_offchain_worker_mode},
        enable_offchain_indexing_{def_enable_offchain_indexing},
        recovery_state_{def_block_to_recover},
        db_cache_size_{def_db_cache_size},
        state_pruning_depth_{} {
    SL_INFO(logger_, "Kagome started. Version: {} ", buildVersion());
  }

  fs::path AppConfigurationImpl::chainSpecPath() const {
    return chain_spec_path_.native();
  }

  kagome::filesystem::path AppConfigurationImpl::runtimeCacheDirPath() const {
    return kagome::filesystem::temp_directory_path() / "kagome/runtimes-cache";
  }

  kagome::filesystem::path AppConfigurationImpl::runtimeCachePath(
      std::string runtime_hash) const {
    return runtimeCacheDirPath() / runtime_hash;
  }

  kagome::filesystem::path AppConfigurationImpl::chainPath(
      std::string chain_id) const {
    return base_path_ / "chains" / chain_id;
  }

  fs::path AppConfigurationImpl::databasePath(std::string chain_id) const {
    return chainPath(chain_id) / "db";
  }

  fs::path AppConfigurationImpl::keystorePath(std::string chain_id) const {
    if (keystore_path_) {
      return *keystore_path_ / chain_id / "keystore";
    }
    return chainPath(chain_id) / "keystore";
  }

  AppConfigurationImpl::FilePtr AppConfigurationImpl::open_file(
      const std::string &filepath) {
    assert(!filepath.empty());
    return AppConfigurationImpl::FilePtr(std::fopen(filepath.c_str(), "r"),
                                         &std::fclose);
  }

  bool AppConfigurationImpl::load_ms(const rapidjson::Value &val,
                                     const char *name,
                                     std::vector<std::string> &target) {
    for (auto it = val.FindMember(name); it != val.MemberEnd(); ++it) {
      auto &value = it->value;
      target.emplace_back(value.GetString(), value.GetStringLength());
    }
    return not target.empty();
  }

  bool AppConfigurationImpl::load_ma(
      const rapidjson::Value &val,
      const char *name,
      std::vector<libp2p::multi::Multiaddress> &target) {
    for (auto it = val.FindMember(name); it != val.MemberEnd(); ++it) {
      auto &value = it->value;
      auto ma_res = libp2p::multi::Multiaddress::create(
          std::string(value.GetString(), value.GetStringLength()));
      if (not ma_res) {
        return false;
      }
      target.emplace_back(std::move(ma_res.value()));
    }
    return not target.empty();
  }

  bool AppConfigurationImpl::load_telemetry_uris(
      const rapidjson::Value &val,
      const char *name,
      std::vector<telemetry::TelemetryEndpoint> &target) {
    auto it = val.FindMember(name);
    if (it != val.MemberEnd() and it->value.IsArray()) {
      for (auto &v : it->value.GetArray()) {
        if (v.IsString()) {
          auto result = parseTelemetryEndpoint(v.GetString());
          if (result.has_value()) {
            target.emplace_back(std::move(result.value()));
            continue;
          }
        }
        return false;
      }  // for
    }
    return true;
  }

  bool AppConfigurationImpl::load_str(const rapidjson::Value &val,
                                      const char *name,
                                      std::string &target) {
    auto m = val.FindMember(name);
    if (val.MemberEnd() != m && m->value.IsString()) {
      target.assign(m->value.GetString(), m->value.GetStringLength());
      return true;
    }
    return false;
  }

  bool AppConfigurationImpl::load_bool(const rapidjson::Value &val,
                                       const char *name,
                                       bool &target) {
    auto m = val.FindMember(name);
    if (val.MemberEnd() != m && m->value.IsBool()) {
      target = m->value.GetBool();
      return true;
    }
    return false;
  }

  bool AppConfigurationImpl::load_u16(const rapidjson::Value &val,
                                      const char *name,
                                      uint16_t &target) {
    uint32_t i;
    if (load_u32(val, name, i)
        && (i & ~std::numeric_limits<uint16_t>::max()) == 0) {
      target = static_cast<uint16_t>(i);
      return true;
    }
    return false;
  }

  bool AppConfigurationImpl::load_u32(const rapidjson::Value &val,
                                      const char *name,
                                      uint32_t &target) {
    if (auto m = val.FindMember(name);
        val.MemberEnd() != m && m->value.IsInt()) {
      const auto v = m->value.GetInt();
      if ((v & (1u << 31u)) == 0) {
        target = static_cast<uint32_t>(v);
        return true;
      }
    }
    return false;
  }

  bool AppConfigurationImpl::load_i32(const rapidjson::Value &val,
                                      const char *name,
                                      int32_t &target) {
    if (auto m = val.FindMember(name);
        val.MemberEnd() != m && m->value.IsInt()) {
      target = m->value.GetInt();
      return true;
    }
    return false;
  }

  void AppConfigurationImpl::parse_general_segment(
      const rapidjson::Value &val) {
    bool validator_mode = false;
    load_bool(val, "validator", validator_mode);
    if (validator_mode) {
      roles_.flags.full = 0;
      roles_.flags.authority = 1;
    }

    load_ms(val, "log", logger_tuning_config_);
  }

  void AppConfigurationImpl::parse_blockchain_segment(
      const rapidjson::Value &val) {
    std::string chain_spec_path_str;
    load_str(val, "chain", chain_spec_path_str);
    chain_spec_path_ = fs::path(chain_spec_path_str);
  }

  void AppConfigurationImpl::parse_storage_segment(
      const rapidjson::Value &val) {
    std::string base_path_str;
    load_str(val, "base-path", base_path_str);
    base_path_ = fs::path(base_path_str);

    std::string database_engine_str;
    if (load_str(val, "database", database_engine_str)) {
      if ("rocksdb" == database_engine_str) {
        storage_backend_ = StorageBackend::RocksDB;
      } else {
        SL_ERROR(logger_,
                 "Unsupported database backend was specified {}, "
                 "available options are [rocksdb]",
                 database_engine_str);
        exit(EXIT_FAILURE);
      }
    }
    load_u32(val, "db-cache", db_cache_size_);
  }

  void AppConfigurationImpl::parse_network_segment(
      const rapidjson::Value &val) {
    load_ma(val, "listen-addr", listen_addresses_);
    load_ma(val, "public-addr", public_addresses_);
    load_ma(val, "bootnodes", boot_nodes_);
    load_u16(val, "port", p2p_port_);
    load_str(val, "rpc-host", rpc_host_);
    load_u16(val, "rpc-port", rpc_port_);
    load_u32(val, "ws-max-connections", max_ws_connections_);
    load_str(val, "prometheus-host", openmetrics_http_host_);
    load_u16(val, "prometheus-port", openmetrics_http_port_);
    load_str(val, "name", node_name_);
    load_u32(val, "out-peers", out_peers_);
    load_u32(val, "in-peers", in_peers_);
    load_u32(val, "in-peers-light", in_peers_light_);
    load_u32(val, "lucky-peers", lucky_peers_);
    load_telemetry_uris(val, "telemetry-endpoints", telemetry_endpoints_);
    load_u32(val, "random-walk-interval", random_walk_interval_);
  }

  void AppConfigurationImpl::parse_additional_segment(
      const rapidjson::Value &val) {
    load_u32(val, "max-blocks-in-response", max_blocks_in_response_);
    load_bool(val, "dev", dev_mode_);
  }

  bool AppConfigurationImpl::validate_config() {
    if (not chainspecExists(chain_spec_path_)) {
      SL_ERROR(logger_,
               "Chain path {} does not exist, "
               "please specify a valid path with --chain option",
               chain_spec_path_);
      return false;
    }

    if (base_path_.empty() or !fs::createDirectoryRecursive(base_path_)) {
      SL_ERROR(logger_,
               "Base path {} does not exist, "
               "please specify a valid path with -d option",
               base_path_);
      return false;
    }

    if (not listen_addresses_.empty()) {
      SL_INFO(logger_,
              "Listen addresses are set. The p2p port value would be ignored "
              "then.");
    } else if (p2p_port_ == 0) {
      SL_ERROR(logger_,
               "p2p port is 0, "
               "please specify a valid path with -p option");
      return false;
    }

    if (rpc_port_ == 0) {
      SL_ERROR(logger_,
               "RPC port is 0, "
               "please specify a valid path with --rpc-port option");
      return false;
    }

    if (node_name_.length() > kNodeNameMaxLength) {
      SL_ERROR(logger_,
               "Node name exceeds the maximum length of {} characters",
               kNodeNameMaxLength);
      return false;
    }

    // pagination page size bounded [kAbsolutMinBlocksInResponse,
    // kAbsolutMaxBlocksInResponse]
    max_blocks_in_response_ = std::clamp(max_blocks_in_response_,
                                         kAbsolutMinBlocksInResponse,
                                         kAbsolutMaxBlocksInResponse);
    return true;
  }

  void AppConfigurationImpl::read_config_from_file(
      const std::string &filepath) {
    assert(!filepath.empty());

    auto file = open_file(filepath);
    if (!file) {
      SL_ERROR(logger_,
               "Configuration file path is invalid: {}, "
               "please specify a valid path with -c option",
               filepath);
      return;
    }

    using FileReadStream = rapidjson::FileReadStream;
    using Document = rapidjson::Document;

    std::array<char, 1024> buffer_size{};
    FileReadStream input_stream(
        file.get(), buffer_size.data(), buffer_size.size());

    Document document;
    document.ParseStream(input_stream);
    if (document.HasParseError()) {
      SL_ERROR(logger_,
               "Configuration file {} parse failed with error {}",
               filepath,
               GetParseError_En(document.GetParseError()));
      return;
    }

    for (auto &handler : handlers_) {
      auto it = document.FindMember(handler.segment_name);
      if (document.MemberEnd() != it) {
        handler.handler(it->value);
      }
    }
  }

  boost::asio::ip::tcp::endpoint AppConfigurationImpl::getEndpointFrom(
      const std::string &host, uint16_t port) const {
    boost::asio::ip::tcp::endpoint endpoint;
    boost::system::error_code err;

    endpoint.address(boost::asio::ip::address::from_string(host, err));
    if (err.failed()) {
      SL_ERROR(logger_, "RPC address '{}' is invalid", host);
      exit(EXIT_FAILURE);
    }

    endpoint.port(port);
    return endpoint;
  }

  outcome::result<boost::asio::ip::tcp::endpoint>
  AppConfigurationImpl::getEndpointFrom(
      const libp2p::multi::Multiaddress &multiaddress) const {
    using proto = libp2p::multi::Protocol::Code;
    constexpr auto NOT_SUPPORTED = std::errc::address_family_not_supported;
    constexpr auto BAD_ADDRESS = std::errc::bad_address;
    auto host = multiaddress.getFirstValueForProtocol(proto::IP4);
    if (not host) {
      host = multiaddress.getFirstValueForProtocol(proto::IP6);
    }
    if (not host) {
      SL_ERROR(logger_,
               "Address cannot be used to bind to ({}). Only IPv4 and IPv6 "
               "interfaces are supported",
               multiaddress.getStringAddress());
      return NOT_SUPPORTED;
    }
    auto port = multiaddress.getFirstValueForProtocol(proto::TCP);
    if (not port) {
      return NOT_SUPPORTED;
    }
    uint16_t port_number = 0;
    try {
      auto wide_port = std::stoul(port.value());
      constexpr auto max_port = std::numeric_limits<uint16_t>::max();
      if (wide_port > max_port or 0 == wide_port) {
        SL_ERROR(
            logger_,
            "Port value ({}) cannot be zero or greater than {} (address {})",
            wide_port,
            max_port,
            multiaddress.getStringAddress());
        return BAD_ADDRESS;
      }
      port_number = static_cast<uint16_t>(wide_port);
    } catch (...) {
      // only std::out_of_range or std::invalid_argument are possible
      SL_ERROR(logger_,
               "Passed value {} is not a valid port number within address {}",
               port.value(),
               multiaddress.getStringAddress());
      return BAD_ADDRESS;
    }
    return getEndpointFrom(host.value(), port_number);
  }

  bool AppConfigurationImpl::testListenAddresses() const {
    auto temp_context = std::make_shared<boost::asio::io_context>();
    constexpr auto kZeroPortTolerance = 0;
    for (const auto &addr : listen_addresses_) {
      // check "/wss" string because of libp2p multiaddress bugs
      if (boost::ends_with(addr.getStringAddress(), "/wss")
          and node_wss_pem_.empty()) {
        SL_ERROR(logger_,
                 "WSS address {} requires --node-wss-pem flag",
                 addr.getStringAddress());
        return false;
      }
      auto endpoint = getEndpointFrom(addr);
      if (not endpoint) {
        SL_ERROR(logger_,
                 "Endpoint cannot be constructed from address {}",
                 addr.getStringAddress());
        return false;
      }
      try {
        boost::system::error_code error_code;
        auto acceptor = api::acceptOnFreePort(
            temp_context, endpoint.value(), kZeroPortTolerance, logger_);
        acceptor->cancel(error_code);
        acceptor->close(error_code);
      } catch (...) {
        SL_ERROR(
            logger_, "Unable to listen on address {}", addr.getStringAddress());
        return false;
      }
    }
    return true;
  }

  std::optional<telemetry::TelemetryEndpoint>
  AppConfigurationImpl::parseTelemetryEndpoint(
      const std::string &record) const {
    /*
     * The only form a telemetry endpoint could be specified is
     * "<endpoint uri> <verbosity level>", where verbosity level is a single
     * numeric character in a range from 0 to 9 inclusively.
     */

    // check there is a space char preceding the verbosity level number
    const auto len = record.length();
    constexpr auto kSpaceChar = ' ';
    if (kSpaceChar != record.at(len - 2)) {
      SL_ERROR(logger_,
               "record '{}' could not be parsed as a valid telemetry endpoint. "
               "The desired format is '<endpoint uri> <verbosity: 0-9>'",
               record);
      return std::nullopt;
    }

    // try to parse verbosity level
    uint8_t verbosity_level{0};
    try {
      auto verbosity_char = record.substr(len - 1);
      int verbosity_level_parsed = std::stoi(verbosity_char);
      // the following check is left intentionally
      // despite possible redundancy
      if (verbosity_level_parsed < 0 or verbosity_level_parsed > 9) {
        throw std::out_of_range("verbosity level value is out of range");
      }
      verbosity_level = static_cast<uint8_t>(verbosity_level_parsed);
    } catch (const std::invalid_argument &e) {
      SL_ERROR(logger_,
               "record '{}' could not be parsed as a valid telemetry endpoint. "
               "The desired format is '<endpoint uri> <verbosity: 0-9>'. "
               "Verbosity level does not meet the format: {}",
               record,
               e.what());
      return std::nullopt;
    } catch (const std::out_of_range &e) {
      SL_ERROR(logger_,
               "record '{}' could not be parsed as a valid telemetry endpoint. "
               "The desired format is '<endpoint uri> <verbosity: 0-9>'. "
               "Verbosity level does not meet the format: {}",
               record,
               e.what());
      return std::nullopt;
    }

    // try to parse endpoint uri
    auto uri_part = record.substr(0, len - 2);

    if (not uri_part.empty() and '/' == uri_part.at(0)) {
      // assume endpoint specified as multiaddress
      auto ma_res = libp2p::multi::Multiaddress::create(uri_part);
      if (ma_res.has_error()) {
        SL_ERROR(logger_,
                 "Telemetry endpoint '{}' cannot be interpreted as a valid "
                 "multiaddress and was skipped due to error: {}",
                 uri_part,
                 ma_res.error());
        return std::nullopt;
      }

      {
        // transform multiaddr of telemetry endpoint into uri form
        auto parts = ma_res.value().getProtocolsWithValues();
        if (parts.size() != 3) {
          SL_ERROR(logger_,
                   "Telemetry endpoint '{}' has unknown format and was skipped",
                   uri_part);
          return std::nullopt;
        }
        auto host = parts[0].second;
        auto schema = parts[2].first.name.substr(std::strlen("x-parity-"));
        auto path = std::regex_replace(parts[2].second, std::regex("%2F"), "/");
        uri_part = fmt::format("{}://{}{}", schema, host, path);
      }
    }

    auto uri = common::Uri::parse(uri_part);
    if (uri.error().has_value()) {
      SL_ERROR(logger_,
               "record '{}' could not be parsed as a valid telemetry endpoint. "
               "The desired format is '<endpoint uri> <verbosity: 0-9>'. "
               "Endpoint URI parsing failed: {}",
               record,
               uri.error().value());
      return std::nullopt;
    }

    return telemetry::TelemetryEndpoint{std::move(uri), verbosity_level};
  }

  bool AppConfigurationImpl::initializeFromArgs(int argc, const char **argv) {
    namespace po = boost::program_options;

    std::optional<std::string> command;
    std::optional<std::string> subcommand;

    using std::string_view_literals::operator""sv;

    if (argc > 0 && argv[0] == "benchmark"sv) {
      command = "benchmark";

      if (argc > 1 && argv[1] == "block"sv) {
        subcommand = "block";
      } else {
        SL_ERROR(logger_, "Usage: kagome benchmark BENCHMARK_TYPE");
        SL_ERROR(logger_, "The only supported BENCHMARK_TYPE is 'block'");
        return false;
      }
    }
    if (subcommand) {
      argc--;
      argv++;
    }

    // clang-format off
    po::options_description desc("General options");
    desc.add_options()
        ("help,h", "show this help message")
        ("log,l", po::value<std::vector<std::string>>(),
          "Sets a custom logging filter. Syntax is `<target>=<level>`, e.g. -llibp2p=off.\n"
          "Log levels (most to least verbose) are trace, debug, verbose, info, warn, error, critical, off. By default, all targets log `info`.\n"
          "The global log level can be set with -l<level>.")
        ("validator", "Enable validator node")
        ("config-file,c", po::value<std::string>(), "Filepath to load configuration from.")
        ;

    po::options_description blockhain_desc("Blockchain options");
    blockhain_desc.add_options()
        ("chain", po::value<std::string>(), "required, chainspec file path")
        ("offchain-worker", po::value<std::string>()->default_value(def_offchain_worker),
          "Should execute offchain workers on every block.\n"
          "Possible values: Always, Never, WhenValidating. WhenValidating is used by default.")
        ("chain-info", po::bool_switch(), "Print chain info as JSON")
        ;

    po::options_description storage_desc("Storage options");
    storage_desc.add_options()
        ("base-path,d", po::value<std::string>(), "required, node base path (keeps storage and keys for known chains)")
        ("keystore", po::value<std::string>(), "required, node keystore")
        ("tmp", "Use temporary storage path")
        ("database", po::value<std::string>()->default_value("rocksdb"), "Database backend to use [rocksdb]")
        ("db-cache", po::value<uint32_t>()->default_value(def_db_cache_size), "Limit the memory the database cache can use <MiB>")
        ("enable-offchain-indexing", po::value<bool>(), "enable Offchain Indexing API, which allow block import to write to offchain DB)")
        ("recovery", po::value<std::string>(), "recovers block storage to state after provided block presented by number or hash, and stop after that")
        ("state-pruning", po::value<std::string>()->default_value("archive"), "state pruning policy. 'archive', 'prune-discarded', or the number of finalized blocks to keep.")
        ("enable-thorough-pruning", po::bool_switch(), "Makes trie node pruner more efficient, but the node starts slowly")
        ;

    po::options_description network_desc("Network options");
    network_desc.add_options()
        ("listen-addr", po::value<std::vector<std::string>>()->multitoken(), "multiaddresses the node listens for open connections on")
        ("public-addr", po::value<std::vector<std::string>>()->multitoken(), "multiaddresses that other nodes use to connect to it")
        ("node-key", po::value<std::string>(), "the secret key to use for libp2p networking")
        ("node-key-file", po::value<std::string>(), "path to the secret key used for libp2p networking (raw binary or hex-encoded")
        ("save-node-key", po::bool_switch(), "save generated libp2p networking key, key will be reused on node restart")
        ("bootnodes", po::value<std::vector<std::string>>()->multitoken(), "multiaddresses of bootstrap nodes")
        ("port,p", po::value<uint16_t>(), "port for peer to peer interactions")
        ("rpc-host", po::value<std::string>(), "address for RPC over HTTP and Websocket")
        ("rpc-port", po::value<uint16_t>(), "port for RPC over HTTP and Websocket")
        ("ws-max-connections", po::value<uint32_t>(), "maximum number of WS RPC server connections")
        ("prometheus-host", po::value<std::string>(), "address for OpenMetrics over HTTP")
        ("prometheus-port", po::value<uint16_t>(), "port for OpenMetrics over HTTP")
        ("out-peers", po::value<uint32_t>()->default_value(def_out_peers), "number of outgoing connections we're trying to maintain")
        ("in-peers", po::value<uint32_t>()->default_value(def_in_peers), "maximum number of inbound full nodes peers")
        ("in-peers-light", po::value<uint32_t>()->default_value(def_in_peers_light), "maximum number of inbound light nodes peers")
        ("lucky-peers", po::value<int32_t>()->default_value(def_lucky_peers), "number of \"lucky\" peers (peers that are being gossiped to). -1 for broadcast." )
        ("max-blocks-in-response", po::value<uint32_t>(), "max block per response while syncing")
        ("name", po::value<std::string>(), "the human-readable name for this node")
        ("no-telemetry", po::bool_switch(), "Disables telemetry broadcasting")
        ("telemetry-url", po::value<std::vector<std::string>>()->multitoken(),
                          "the URL of the telemetry server to connect to and verbosity level (0-9),\n"
                          "e.g. --telemetry-url 'wss://foo/bar 0'")
        ("random-walk-interval", po::value<uint32_t>()->default_value(def_random_walk_interval), "Kademlia random walk interval")
        ("node-wss-pem", po::value<std::string>(), "Path to pem file with SSL certificate for libp2p wss")
        ("rpc-cors", po::value<std::string>(), "(unused, zombienet stub)")
        ("unsafe-rpc-external", po::bool_switch(), "alias for \"--rpc-host 0.0.0.0\"")
        ("rpc-methods", po::value<std::string>(), "\"auto\" (default), \"unsafe\", \"safe\"")
        ("no-mdns", po::bool_switch(), "(unused, zombienet stub)")
        ("prometheus-external", po::bool_switch(), "alias for \"--prometheus-host 0.0.0.0\"")
        ;

    po::options_description development_desc("Additional options");
    development_desc.add_options()
        ("dev", "if node run in development mode")
        ("dev-with-wipe", "if needed to wipe base path (only for dev mode)")
        ("sync", po::value<std::string>()->default_value(def_full_sync),
          "choose the desired sync method (Full, Fast). Full is used by default.")
        ("wasm-execution", po::value<std::string>()->default_value(def_wasm_execution),
          "choose the desired wasm execution method (Compiled, Interpreted)")
        ("unsafe-cached-wavm-runtime", "use WAVM runtime cache")
        ("purge-wavm-cache", "purge WAVM runtime cache")
        ("parachain-runtime-instance-cache-size",
          po::value<uint32_t>()->default_value(def_parachain_runtime_instance_cache_size),
          "Number of parachain runtime instances to keep cached")
        ;
    po::options_description benchmark_desc("Benchmark options");
    benchmark_desc.add_options()
      ("from", po::value<uint32_t>(), "set the initial block for block execution benchmark")
      ("to", po::value<uint32_t>(), "set the final block for block execution benchmark")
      ("repeat", po::value<uint16_t>(), "set the repetition number for block execution benchmark")
      ;

    po::options_description db_editor_desc("kagome db-editor - to view help message for db editor");

    // clang-format on

    for (auto &[flag, name, dev] : devAccounts()) {
      development_desc.add_options()(
          flag,
          po::bool_switch(),
          fmt::format("Shortcut for `--name {} --validator` with session keys "
                      "for `{}` added to keystore",
                      name,
                      name)
              .c_str());
    }

    po::variables_map vm;
    // first-run parse to read only general options and to lookup for "help"
    // all the rest options are ignored
    po::parsed_options parsed = po::command_line_parser(argc, argv)
                                    .options(desc)
                                    .allow_unregistered()
                                    .run();
    po::store(parsed, vm);
    po::notify(vm);

    desc.add(blockhain_desc)
        .add(storage_desc)
        .add(network_desc)
        .add(development_desc);
    if (command == "db-editor") {
      desc.add(db_editor_desc);
    } else if (command == "benchmark") {
      desc.add(benchmark_desc);
    }

    if (vm.count("help") > 0) {
      std::cout
          << "Available subcommands: storage-explorer db-editor benchmark\n";
      std::cout << desc << std::endl;
      return false;
    }

    try {
      // second-run parse to gather all known options
      // with reporting about any unrecognized input
      po::store(po::parse_command_line(argc, argv, desc), vm);
      po::store(parsed, vm);
      po::notify(vm);
    } catch (const std::exception &e) {
      std::cerr << "Error: " << e.what() << '\n'
                << "Try run with option '--help' for more information"
                << std::endl;
      return false;
    }

    // Setup default development settings (and wipe if needed)
    if (vm.count("dev") > 0 or vm.count("dev-with-wipe") > 0) {
      constexpr auto with_kagome_embeddings =
#ifdef USE_KAGOME_EMBEDDINGS
          true;
#else
          false;
#endif  // USE_KAGOME_EMBEDDINGS

      if constexpr (not with_kagome_embeddings) {
        std::cerr << "Warning: developers mode is not available. "
                     "Application was built without developers embeddings "
                     "(EMBEDDINGS option is OFF)."
                  << std::endl;
        return false;
      } else {
        dev_mode_ = true;

        auto dev_env_path = fs::temp_directory_path() / "kagome_dev";
        chain_spec_path_ = dev_env_path / "chainspec.json";
        base_path_ = dev_env_path / "base_path";

        // Wipe base directory on demand
        if (vm.count("dev-with-wipe") > 0) {
          kagome::filesystem::remove_all(dev_env_path);
        }

        if (not kagome::filesystem::exists(chain_spec_path_)) {
          kagome::filesystem::create_directories(
              chain_spec_path_.parent_path());

          std::ofstream ofs;
          ofs.open(chain_spec_path_.native(), std::ios::ate);
          ofs << kagome::assets::embedded_chainspec;
          ofs.close();

          auto chain_spec = ChainSpecImpl::loadFrom(chain_spec_path_.native());
          auto path = keystorePath(chain_spec.value()->id());

          if (not chain_spec.has_value()) {
            std::cerr << "Warning: developers mode chain spec is corrupted."
                      << std::endl;
            return false;
          }

          if (chain_spec.value()->bootNodes().empty()) {
            std::cerr
                << "Warning: developers mode chain spec bootnodes is empty."
                << std::endl;
            return false;
          }

          auto ma_res = chain_spec.value()->bootNodes()[0];
          listen_addresses_.emplace_back(ma_res);

          kagome::filesystem::create_directories(path);

          for (auto key_descr : kagome::assets::embedded_keys) {
            ofs.open((path / key_descr.first).native(), std::ios::ate);
            ofs << key_descr.second;
            ofs.close();
          }
        }

        roles_.flags.full = 0;
        roles_.flags.authority = 1;
        p2p_port_ = def_p2p_port;
        rpc_host_ = def_rpc_host;
        openmetrics_http_host_ = def_openmetrics_http_host;
        rpc_port_ = def_rpc_port;
        openmetrics_http_port_ = def_openmetrics_http_port;
      }
    }

    std::optional<std::string> dev_account_flag;
    for (auto &[flag, name, dev] : devAccounts()) {
      if (auto val = find_argument<bool>(vm, flag); val && *val) {
        if (dev_account_flag) {
          SL_ERROR(
              logger_, "--{} conflicts with --{}", flag, *dev_account_flag);
          return false;
        }
        dev_account_flag = flag;
        node_name_ = name;
        dev_mnemonic_phrase_ = dev;
        // if dev account is passed node is considered as validator
        roles_.flags.full = 0;
        roles_.flags.authority = 1;
      }
    }

    find_argument<std::string>(
        vm, "node-wss-pem", [&](const std::string &path) {
          std::string pem;
          if (not readFile(pem, path)) {
            SL_ERROR(logger_, "--node-wss-pem {}: read error", path);
            return;
          }
          if (auto r = libp2p::layer::WssCertificate::make(pem)) {
            node_wss_pem_ = pem;
          } else {
            SL_ERROR(logger_, "--node-wss-pem {}: {}", path, r.error());
          }
        });

    find_argument<std::string>(vm, "config-file", [&](const std::string &path) {
      if (dev_mode_) {
        std::cerr << "Warning: config file has ignored because dev mode"
                  << std::endl;
      } else {
        read_config_from_file(path);
      }
    });

    if (vm.end() != vm.find("validator")) {
      roles_.flags.full = 0;
      roles_.flags.authority = 1;
    }

    find_argument<std::string>(
        vm, "chain", [&](const std::string &val) { chain_spec_path_ = val; });
    if (not chainspecExists(chain_spec_path_)) {
      std::cerr << "Specified chain spec " << chain_spec_path_
                << " does not exist." << std::endl;
    }

    if (vm.end() != vm.find("tmp")) {
      auto unique_name = filesystem::unique_path();
      base_path_ = (kagome::filesystem::temp_directory_path() / unique_name);
    } else {
      find_argument<std::string>(
          vm, "base-path", [&](const std::string &val) { base_path_ = val; });
    }

    find_argument<std::string>(
        vm, "keystore", [&](const std::string &val) { keystore_path_ = val; });

    bool unknown_database_engine_is_set = false;
    find_argument<std::string>(vm, "database", [&](const std::string &val) {
      if ("rocksdb" == val) {
        storage_backend_ = StorageBackend::RocksDB;
      } else {
        unknown_database_engine_is_set = true;
        SL_ERROR(logger_,
                 "Unsupported database backend was specified {}, "
                 "available options are [rocksdb]",
                 val);
      }
    });
    if (unknown_database_engine_is_set) {
      return false;
    }
    find_argument<uint32_t>(
        vm, "db-cache", [&](uint32_t val) { db_cache_size_ = val; });

    std::vector<std::string> boot_nodes;
    find_argument<std::vector<std::string>>(
        vm, "bootnodes", [&](const std::vector<std::string> &val) {
          boot_nodes = val;
        });
    if (not boot_nodes.empty()) {
      boot_nodes_.clear();
      boot_nodes_.reserve(boot_nodes.size());
      for (auto &addr_str : boot_nodes) {
        auto ma_res = libp2p::multi::Multiaddress::create(addr_str);
        if (not ma_res.has_value()) {
          auto err_msg = fmt::format(
              "Bootnode '{}' is invalid: {}", addr_str, ma_res.error());
          SL_ERROR(logger_, "{}", err_msg);
          std::cout << err_msg << std::endl;
          return false;
        }
        auto peer_id_base58_opt = ma_res.value().getPeerId();
        if (not peer_id_base58_opt) {
          auto err_msg = "Bootnode '" + addr_str + "' has not peer_id";
          SL_ERROR(logger_, "{}", err_msg);
          std::cout << err_msg << std::endl;
          return false;
        }
        boot_nodes_.emplace_back(std::move(ma_res.value()));
      }
    }

    std::optional<std::string> node_key;
    find_argument<std::string>(
        vm, "node-key", [&](const std::string &val) { node_key.emplace(val); });
    if (node_key.has_value()) {
      auto key_res = crypto::Ed25519Seed::fromHex(node_key.value());
      if (not key_res.has_value()) {
        auto err_msg = fmt::format(
            "Node key '{}' is invalid: {}", node_key.value(), key_res.error());
        SL_ERROR(logger_, "{}", err_msg);
        std::cout << err_msg << std::endl;
        return false;
      }
      node_key_.emplace(std::move(key_res.value()));
    }

    if (not node_key_.has_value()) {
      find_argument<std::string>(
          vm, "node-key-file", [&](const std::string &val) {
            node_key_file_ = val;
          });
    }

    find_argument<bool>(
        vm, "save-node-key", [&](bool val) { save_node_key_ = val; });

    find_argument<uint16_t>(vm, "port", [&](uint16_t val) {
      p2p_port_ = val;
      p2p_port_explicitly_defined_ = true;
    });

    auto parse_multiaddrs =
        [&](const std::string &param_name,
            std::vector<libp2p::multi::Multiaddress> &output_field) -> bool {
      std::vector<std::string> addrs;
      find_argument<std::vector<std::string>>(
          vm, param_name.c_str(), [&](const auto &val) { addrs = val; });

      if (not addrs.empty()) {
        output_field.clear();
      }
      for (auto &s : addrs) {
        auto ma_res = libp2p::multi::Multiaddress::create(s);
        if (not ma_res) {
          SL_ERROR(logger_,
                   "Address {} passed as value to {} is invalid: {}",
                   s,
                   param_name,
                   ma_res.error());
          return false;
        }
        output_field.emplace_back(std::move(ma_res.value()));
      }
      return true;
    };

    if (not parse_multiaddrs("listen-addr", listen_addresses_)) {
      return false;  // just proxy erroneous case to the top level
    }

    // Check of possible address ambiguity
    if (p2p_port_explicitly_defined_ and not listen_addresses_.empty()) {
      SL_ERROR(logger_,
               "Port and listen address must not be defined simultaneously; "
               "Leave only one of them");
      return false;
    }

    if (not parse_multiaddrs("public-addr", public_addresses_)) {
      return false;  // just proxy erroneous case to the top level
    }

    auto publish_localhost = [&] {
      auto replace = [&](std::string_view prefix,
                         std::string_view replacement,
                         std::string_view str) {
        if (boost::starts_with(str, prefix)) {
          std::string replaced{replacement};
          replaced += str.substr(prefix.size());
          public_addresses_.emplace_back(
              libp2p::multi::Multiaddress::create(replaced).value());
        }
      };
      for (auto &addr : listen_addresses_) {
        auto str = addr.getStringAddress();
        replace("/ip4/0.0.0.0/", "/ip4/127.0.0.1/", str);
        replace("/ip6/::/", "/ip6/::1/", str);
      }
    };

    // this goes before transforming p2p port option to listen addresses,
    // because it does not make sense to announce 0.0.0.0 as a public address.
    // Moreover, this is ok to have empty set of public addresses at this point
    // of time
    if (public_addresses_.empty() and not listen_addresses_.empty()) {
      SL_INFO(logger_,
              "Public addresses are not specified. Using listen addresses as "
              "node's public addresses");
      public_addresses_ = listen_addresses_;
      publish_localhost();
    }

    // If listen address has not defined, listen P2P TCP-port on all
    // accessible interfaces
    if (listen_addresses_.empty()) {
      // IPv6
      {
        auto ma_res = libp2p::multi::Multiaddress::create(
            "/ip6/::/tcp/" + std::to_string(p2p_port_));
        if (not ma_res) {
          SL_ERROR(
              logger_,
              "Cannot construct IPv6 listen multiaddress from port {}. Error: "
              "{}",
              p2p_port_,
              ma_res.error());
        } else {
          SL_INFO(logger_,
                  "Automatically added IPv6 listen address {}",
                  ma_res.value().getStringAddress());
          listen_addresses_.emplace_back(std::move(ma_res.value()));
        }
      }

      // IPv4
      {
        auto ma_res = libp2p::multi::Multiaddress::create(
            "/ip4/0.0.0.0/tcp/" + std::to_string(p2p_port_));
        if (not ma_res) {
          SL_ERROR(
              logger_,
              "Cannot construct IPv4 listen multiaddress from port {}. Error: "
              "{}",
              p2p_port_,
              ma_res.error());
        } else {
          SL_INFO(logger_,
                  "Automatically added IPv4 listen address {}",
                  ma_res.value().getStringAddress());
          listen_addresses_.emplace_back(std::move(ma_res.value()));
        }

        if (public_addresses_.empty()) {
          publish_localhost();
        }
      }
    }

    if (not testListenAddresses()) {
      SL_ERROR(logger_,
               "One of configured listen addresses is unavailable, the node "
               "cannot start.");
      return false;
    }

    find_argument<uint32_t>(vm, "max-blocks-in-response", [&](uint32_t val) {
      max_blocks_in_response_ = val;
    });

    find_argument<std::vector<std::string>>(
        vm, "log", [&](const std::vector<std::string> &val) {
          logger_tuning_config_ = val;
        });

    find_argument<std::string>(
        vm, "rpc-host", [&](const std::string &val) { rpc_host_ = val; });

    find_argument<std::string>(
        vm, "prometheus-host", [&](const std::string &val) {
          openmetrics_http_host_ = val;
        });

    find_argument<bool>(vm, "unsafe-rpc-external", [&](bool flag) {
      if (flag) {
        rpc_host_ = "0.0.0.0";
      }
    });
    find_argument<bool>(vm, "prometheus-external", [&](bool flag) {
      if (flag) {
        openmetrics_http_host_ = "0.0.0.0";
      }
    });

    find_argument<uint16_t>(
        vm, "rpc-port", [&](uint16_t val) { rpc_port_ = val; });

    find_argument<uint16_t>(vm, "prometheus-port", [&](uint16_t val) {
      openmetrics_http_port_ = val;
    });

    if (auto str = find_argument<std::string>(vm, "rpc-methods")) {
      if (auto value = parseAllowUnsafeRpc(*str)) {
        allow_unsafe_rpc_ = *value;
      } else {
        SL_ERROR(logger_, "Invalid --rpc-methods: \"{}\"", str);
        return false;
      }
    }

    find_argument<uint32_t>(
        vm, "out-peers", [&](uint32_t val) { out_peers_ = val; });

    find_argument<uint32_t>(
        vm, "in-peers", [&](uint32_t val) { in_peers_ = val; });

    find_argument<uint32_t>(
        vm, "in-peers-light", [&](uint32_t val) { in_peers_light_ = val; });

    find_argument<int32_t>(
        vm, "lucky-peers", [&](int32_t val) { lucky_peers_ = val; });

    find_argument<uint32_t>(vm, "ws-max-connections", [&](uint32_t val) {
      max_ws_connections_ = val;
    });

    find_argument<uint32_t>(vm, "random-walk-interval", [&](uint32_t val) {
      random_walk_interval_ = val;
    });

    rpc_endpoint_ = getEndpointFrom(rpc_host_, rpc_port_);
    openmetrics_http_endpoint_ =
        getEndpointFrom(openmetrics_http_host_, openmetrics_http_port_);

    find_argument<std::string>(
        vm, "name", [&](const std::string &val) { node_name_ = val; });

    auto parse_telemetry_urls =
        [&](const std::string &param_name,
            std::vector<telemetry::TelemetryEndpoint> &output_field) -> bool {
      std::vector<std::string> tokens;
      find_argument<std::vector<std::string>>(
          vm, param_name.c_str(), [&](const auto &val) { tokens = val; });

      for (const auto &token : tokens) {
        auto result = parseTelemetryEndpoint(token);
        if (result.has_value()) {
          telemetry_endpoints_.emplace_back(std::move(result.value()));
        } else {
          return false;
        }
      }
      return true;
    };

    find_argument<bool>(vm, "no-telemetry", [&](bool telemetry_disabled) {
      is_telemetry_enabled_ = not telemetry_disabled;
    });

    if (is_telemetry_enabled_) {
      if (not parse_telemetry_urls("telemetry-url", telemetry_endpoints_)) {
        return false;  // just proxy erroneous case to the top level
      }
    }

    bool sync_method_value_error = false;
    find_argument<std::string>(
        vm, "sync", [this, &sync_method_value_error](const std::string &val) {
          auto sync_method_opt = str_to_sync_method(val);
          if (not sync_method_opt) {
            sync_method_value_error = true;
            SL_ERROR(logger_, "Invalid sync method specified: '{}'", val);
          } else {
            sync_method_ = sync_method_opt.value();
          }
        });
    if (sync_method_value_error) {
      return false;
    }

    bool exec_method_value_error = false;
    find_argument<std::string>(
        vm,
        "wasm-execution",
        [this, &exec_method_value_error](const std::string &val) {
          auto runtime_exec_method_opt = str_to_runtime_exec_method(val);
          if (not runtime_exec_method_opt) {
            exec_method_value_error = true;
            SL_ERROR(logger_,
                     "Invalid runtime execution method specified: '{}'",
                     val);
          } else {
            runtime_exec_method_ = runtime_exec_method_opt.value();
          }
        });
    if (exec_method_value_error) {
      return false;
    }

    if (vm.count("unsafe-cached-wavm-runtime") > 0) {
      use_wavm_cache_ = true;
    }

    if (vm.count("purge-wavm-cache") > 0) {
      purge_wavm_cache_ = true;
      if (fs::exists(runtimeCacheDirPath())) {
        std::error_code ec;
        kagome::filesystem::remove_all(runtimeCacheDirPath(), ec);
        if (ec) {
          SL_ERROR(logger_,
                   "Failed to purge cache in {} ['{}']",
                   runtimeCacheDirPath(),
                   ec);
        }
      }
    }

    if (auto arg = find_argument<uint32_t>(
            vm, "parachain-runtime-instance-cache-size");
        arg.has_value()) {
      parachain_runtime_instance_cache_size_ = *arg;
    }

    bool offchain_worker_value_error = false;
    find_argument<std::string>(
        vm,
        "offchain-worker",
        [this, &offchain_worker_value_error](const std::string &val) {
          auto offchain_worker_mode_opt = str_to_offchain_worker_mode(val);
          if (offchain_worker_mode_opt) {
            offchain_worker_mode_ = offchain_worker_mode_opt.value();
          } else {
            offchain_worker_value_error = true;
            SL_ERROR(
                logger_, "Invalid offchain worker mode specified: '{}'", val);
          }
        });
    if (offchain_worker_value_error) {
      return false;
    }

    if (vm.count("enable-offchain-indexing") > 0) {
      enable_offchain_indexing_ = true;
    }

    find_argument<bool>(vm, "chain-info", [&](bool subcommand_chain_info) {
      subcommand_ = Subcommand::ChainInfo;
    });

    if (command == "benchmark" && subcommand == "block") {
      auto from_opt = find_argument<uint32_t>(vm, "from");
      if (!from_opt) {
        SL_ERROR(logger_, "Required argument --from is not provided");
        return false;
      }
      auto to_opt = find_argument<uint32_t>(vm, "to");
      if (!to_opt) {
        SL_ERROR(logger_, "Required argument --to is not provided");
        return false;
      }
      auto repeat_opt = find_argument<uint16_t>(vm, "repeat");
      if (!to_opt) {
        SL_ERROR(logger_, "Required argument --repeat is not provided");
        return false;
      }
      benchmark_config_ = BlockBenchmarkConfig{
          .from = *from_opt,
          .to = *to_opt,
          .times = *repeat_opt,
      };
    }

    bool has_recovery = false;
    find_argument<std::string>(vm, "recovery", [&](const std::string &val) {
      has_recovery = true;
      recovery_state_ = str_to_recovery_state(val);
      if (not recovery_state_) {
        SL_ERROR(logger_, "Invalid recovery state specified: '{}'", val);
      }
    });
    if (has_recovery and not recovery_state_.has_value()) {
      return false;
    }

    if (auto state_pruning_opt =
            find_argument<std::string>(vm, "state-pruning");
        state_pruning_opt.has_value()) {
      const auto &val = state_pruning_opt.value();
      if (val == "archive") {
        state_pruning_depth_ = std::nullopt;
      } else if (val == "prune-discarded") {
        state_pruning_depth_ = std::nullopt;
        prune_discarded_states_ = true;
      } else {
        uint32_t depth{};
        auto err = std::from_chars(&*val.begin(), &*val.end(), depth).ec;
        if (err == std::errc{}) {
          state_pruning_depth_ = depth;
        } else {
          SL_ERROR(logger_,
                   "Failed to parse state-pruning param "
                   "(which should be either 'archive' or an integer): {}",
                   make_error_code(err));
          return false;
        }
      }

      if (find_argument(vm, "enable-thorough-pruning")) {
        enable_thorough_pruning_ = true;
      }
    }

    // if something wrong with config print help message
    if (not validate_config()) {
      std::cout << desc << std::endl;
      return false;
    }
    return true;
  }
}  // namespace kagome::application
