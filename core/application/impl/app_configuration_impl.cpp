/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "application/impl/app_configuration_impl.hpp"

#include <string>

#include <rapidjson/document.h>
#include <rapidjson/filereadstream.h>
#include <boost/algorithm/string.hpp>
#include <boost/program_options.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "assets/assets.hpp"
#include "chain_spec_impl.hpp"
#include "common/hexutil.hpp"
#include "filesystem/directories.hpp"

namespace {
  namespace fs = kagome::filesystem;

  template <typename T, typename Func>
  inline void find_argument(boost::program_options::variables_map &vm,
                            char const *name,
                            Func &&f) {
    assert(nullptr != name);
    if (auto it = vm.find(name); it != vm.end()) {
      std::forward<Func>(f)(it->second.as<T>());
    }
  }

  const std::string def_rpc_http_host = "0.0.0.0";
  const std::string def_rpc_ws_host = "0.0.0.0";
  const std::string def_openmetrics_http_host = "0.0.0.0";
  const uint16_t def_rpc_http_port = 9933;
  const uint16_t def_rpc_ws_port = 9944;
  const uint16_t def_openmetrics_http_port = 9615;
  const uint32_t def_ws_max_connections = 100;
  const uint16_t def_p2p_port = 30363;
  const int def_verbosity = static_cast<int>(kagome::log::Level::INFO);
  const bool def_is_already_synchronized = false;
  const bool def_is_unix_slots_strategy = false;
  const bool def_dev_mode = false;
  const kagome::network::Roles def_roles = [] {
    kagome::network::Roles roles;
    roles.flags.full = 1;
    return roles;
  }();

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
}  // namespace

namespace kagome::application {

  AppConfigurationImpl::AppConfigurationImpl(log::Logger logger)
      : logger_(std::move(logger)),
        roles_(def_roles),
        p2p_port_(def_p2p_port),
        verbosity_(static_cast<log::Level>(def_verbosity)),
        is_already_synchronized_(def_is_already_synchronized),
        max_blocks_in_response_(kAbsolutMaxBlocksInResponse),
        is_unix_slots_strategy_(def_is_unix_slots_strategy),
        rpc_http_host_(def_rpc_http_host),
        rpc_ws_host_(def_rpc_ws_host),
        openmetrics_http_host_(def_openmetrics_http_host),
        rpc_http_port_(def_rpc_http_port),
        rpc_ws_port_(def_rpc_ws_port),
        openmetrics_http_port_(def_openmetrics_http_port),
        dev_mode_(def_dev_mode),
        node_name_(randomNodeName()),
        max_ws_connections_(def_ws_max_connections) {}

  fs::path AppConfigurationImpl::chainSpecPath() const {
    return chain_spec_path_.native();
  }

  boost::filesystem::path AppConfigurationImpl::chainPath(
      std::string chain_id) const {
    return base_path_ / chain_id;
  }

  fs::path AppConfigurationImpl::databasePath(std::string chain_id) const {
    return chainPath(chain_id) / "db";
  }

  fs::path AppConfigurationImpl::keystorePath(std::string chain_id) const {
    return chainPath(chain_id) / "keystore";
  }

  AppConfigurationImpl::FilePtr AppConfigurationImpl::open_file(
      const std::string &filepath) {
    assert(!filepath.empty());
    return AppConfigurationImpl::FilePtr(std::fopen(filepath.c_str(), "r"),
                                         &std::fclose);
  }

  bool AppConfigurationImpl::load_ma(
      const rapidjson::Value &val,
      char const *name,
      std::vector<libp2p::multi::Multiaddress> &target) {
    for (auto it = val.FindMember(name); it != val.MemberEnd(); ++it) {
      auto &value = it->value;
      auto ma_res = libp2p::multi::Multiaddress::create(
          std::string(value.GetString(), value.GetStringLength()));
      if (not ma_res) {
        return false;
      }
      target.emplace_back(std::move(ma_res.value()));
      return true;
    }
    return not target.empty();
  }

  bool AppConfigurationImpl::load_str(const rapidjson::Value &val,
                                      char const *name,
                                      std::string &target) {
    auto m = val.FindMember(name);
    if (val.MemberEnd() != m && m->value.IsString()) {
      target.assign(m->value.GetString(), m->value.GetStringLength());
      return true;
    }
    return false;
  }

  bool AppConfigurationImpl::load_bool(const rapidjson::Value &val,
                                       char const *name,
                                       bool &target) {
    auto m = val.FindMember(name);
    if (val.MemberEnd() != m && m->value.IsBool()) {
      target = m->value.GetBool();
      return true;
    }
    return false;
  }

  bool AppConfigurationImpl::load_u16(const rapidjson::Value &val,
                                      char const *name,
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
                                      char const *name,
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

  void AppConfigurationImpl::parse_general_segment(rapidjson::Value &val) {
    bool validator_mode = false;
    load_bool(val, "validator", validator_mode);
    if (validator_mode) {
      roles_.flags.authority = 1;
    }

    uint16_t v{};
    if (not load_u16(val, "verbosity", v)) {
      return;
    }
    auto level = static_cast<log::Level>(v + def_verbosity);
    if (level >= log::Level::OFF && level <= log::Level::TRACE) {
      verbosity_ = level;
    }
  }

  void AppConfigurationImpl::parse_blockchain_segment(rapidjson::Value &val) {
    std::string chain_spec_path_str;
    load_str(val, "chain", chain_spec_path_str);
    chain_spec_path_ = fs::path(chain_spec_path_str);
  }

  void AppConfigurationImpl::parse_storage_segment(rapidjson::Value &val) {
    std::string base_path_str;
    load_str(val, "base-path", base_path_str);
    base_path_ = fs::path(base_path_str);
  }

  void AppConfigurationImpl::parse_network_segment(rapidjson::Value &val) {
    load_ma(val, "listen-addr", listen_addresses_);
    load_ma(val, "public-addr", public_addresses_);
    load_ma(val, "bootnodes", boot_nodes_);
    load_u16(val, "port", p2p_port_);
    load_str(val, "rpc-host", rpc_http_host_);
    load_u16(val, "rpc-port", rpc_http_port_);
    load_str(val, "ws-host", rpc_ws_host_);
    load_u16(val, "ws-port", rpc_ws_port_);
    load_u32(val, "ws-max-connections", max_ws_connections_);
    load_str(val, "prometheus-host", openmetrics_http_host_);
    load_u16(val, "prometheus-port", openmetrics_http_port_);
    load_str(val, "name", node_name_);
  }

  void AppConfigurationImpl::parse_additional_segment(rapidjson::Value &val) {
    load_bool(val, "already-synchronized", is_already_synchronized_);
    load_u32(val, "max-blocks-in-response", max_blocks_in_response_);
    load_bool(val, "is-unix-slots-strategy", is_unix_slots_strategy_);
    load_bool(val, "dev", dev_mode_);
  }

  bool AppConfigurationImpl::validate_config() {
    if (not fs::exists(chain_spec_path_)) {
      logger_->error(
          "Chain path {} does not exist, "
          "please specify a valid path with --chain option",
          chain_spec_path_);
      return false;
    }

    if (base_path_.empty() or !fs::createDirectoryRecursive(base_path_)) {
      logger_->error(
          "Base path {} does not exist, "
          "please specify a valid path with -d option",
          base_path_);
      return false;
    }

    if (not listen_addresses_.empty()) {
      logger_->info(
          "Listen addresses are set. The p2p port value would be ignored "
          "then.");
    } else if (p2p_port_ == 0) {
      logger_->error(
          "p2p port is 0, "
          "please specify a valid path with -p option");
      return false;
    }

    if (rpc_ws_port_ == 0) {
      logger_->error(
          "RPC ws port is 0, "
          "please specify a valid path with --ws-port option");
      return false;
    }

    if (rpc_http_port_ == 0) {
      logger_->error(
          "RPC http port is 0, "
          "please specify a valid path with --rpc-port option");
      return false;
    }

    if (node_name_.length() > kNodeNameMaxLength) {
      logger_->error("Node name exceeds the maximum length of {} characters",
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
      logger_->error(
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
      logger_->error("Configuration file {} parse failed with error {}",
                     filepath,
                     document.GetParseError());
      return;
    }

    for (auto &handler : handlers_) {
      auto it = document.FindMember(handler.segment_name);
      if (document.MemberEnd() != it) {
        handler.handler(it->value);
      }
    }
  }

  boost::asio::ip::tcp::endpoint AppConfigurationImpl::get_endpoint_from(
      std::string const &host, uint16_t port) {
    boost::asio::ip::tcp::endpoint endpoint;
    boost::system::error_code err;

    endpoint.address(boost::asio::ip::address::from_string(host, err));
    if (err.failed()) {
      logger_->error("RPC address '{}' is invalid", host);
      exit(EXIT_FAILURE);
    }

    endpoint.port(port);
    return endpoint;
  }

  bool AppConfigurationImpl::initialize_from_args(int argc, char **argv) {
    namespace po = boost::program_options;

    // clang-format off
    po::options_description desc("General options");
    desc.add_options()
        ("help,h", "show this help message")
        ("verbosity,v", po::value<int>(), "Log level: 0 - trace, 1 - debug, 2 - info, 3 - warn, 4 - error, 5 - critical, 6 - no log")
        ("validator", "Enable validator node")
        ("config-file,c", po::value<std::string>(), "Filepath to load configuration from.")
        ;

    po::options_description blockhain_desc("Blockchain options");
    blockhain_desc.add_options()
        ("chain", po::value<std::string>(), "required, chainspec file path")
        ;

    po::options_description storage_desc("Storage options");
    storage_desc.add_options()
        ("base-path,d", po::value<std::string>(), "required, node base path (keeps storage and keys for known chains)")
        ;

    po::options_description network_desc("Network options");
    network_desc.add_options()
        ("listen-addr", po::value<std::vector<std::string>>()->multitoken(), "multiaddresses the node listens for open connections on")
        ("public-addr", po::value<std::vector<std::string>>()->multitoken(), "multiaddresses that other nodes use to connect to it")
        ("node-key", po::value<std::string>(), "the secret key to use for libp2p networking")
        ("node-key-file", po::value<std::string>(), "path to the secret key used for libp2p networking (raw binary or hex-encoded")
        ("bootnodes", po::value<std::vector<std::string>>()->multitoken(), "multiaddresses of bootstrap nodes")
        ("port,p", po::value<uint16_t>(), "port for peer to peer interactions")
        ("rpc-host", po::value<std::string>(), "address for RPC over HTTP")
        ("rpc-port", po::value<uint16_t>(), "port for RPC over HTTP")
        ("ws-host", po::value<std::string>(), "address for RPC over Websocket protocol")
        ("ws-port", po::value<uint16_t>(), "port for RPC over Websocket protocol")
        ("ws-max-connections", po::value<uint32_t>(), "maximum number of WS RPC server connections")
        ("prometheus-host", po::value<std::string>(), "address for OpenMetrics over HTTP")
        ("prometheus-port", po::value<uint16_t>(), "port for OpenMetrics over HTTP")
        ("max-blocks-in-response", po::value<int>(), "max block per response while syncing")
        ("name", po::value<std::string>(), "the human-readable name for this node")
        ;

    po::options_description additional_desc("Additional options");
    additional_desc.add_options()
        ("already-synchronized,s", "if need to consider synchronized")
        ("unix-slots,u", "if slots are calculated from unix epoch")
        ;

    po::options_description development_desc("Development options");
    additional_desc.add_options()
        ("dev", "if node run in development mode")
        ("dev-with-wipe", "if needed to wipe base path (only for dev mode)")
        ;
    // clang-format on

    po::variables_map vm;
    po::parsed_options parsed = po::command_line_parser(argc, argv)
                                    .options(desc)
                                    .allow_unregistered()
                                    .run();
    po::store(parsed, vm);
    po::notify(vm);

    desc.add(blockhain_desc)
        .add(storage_desc)
        .add(network_desc)
        .add(additional_desc);

    if (vm.count("help") > 0) {
      std::cout << desc << std::endl;
      return false;
    }

    desc.add(development_desc);

    try {
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
          boost::filesystem::remove_all(dev_env_path);
        }

        if (not boost::filesystem::exists(chain_spec_path_)) {
          boost::filesystem::create_directories(chain_spec_path_.parent_path());

          std::ofstream ofs;
          ofs.open(chain_spec_path_.native(), std::ios::ate);
          ofs << kagome::assets::embedded_chainspec;
          ofs.close();

          auto chain_spec = ChainSpecImpl::loadFrom(chain_spec_path_.native());
          auto path = keystorePath(chain_spec.value()->id());

          boost::filesystem::create_directories(path);

          for (auto key_descr : kagome::assets::embedded_keys) {
            ofs.open((path / key_descr.first).native(), std::ios::ate);
            ofs << key_descr.second;
            ofs.close();
          }
        }

        roles_.flags.full = 1;
        roles_.flags.authority = 1;
        p2p_port_ = def_p2p_port;
        is_already_synchronized_ = true;
        rpc_http_host_ = def_rpc_http_host;
        rpc_ws_host_ = def_rpc_ws_host;
        openmetrics_http_host_ = def_openmetrics_http_host;
        rpc_http_port_ = def_rpc_http_port;
        rpc_ws_port_ = def_rpc_ws_port;
        openmetrics_http_port_ = def_openmetrics_http_port;

        auto ma_res =
            libp2p::multi::Multiaddress::create("/ip4/127.0.0.1/tcp/30363");
        assert(ma_res.has_value());
        listen_addresses_.emplace_back(std::move(ma_res.value()));
      }
    }

    find_argument<std::string>(vm, "config-file", [&](std::string const &path) {
      if (dev_mode_) {
        std::cerr << "Warning: config file has ignored because dev mode"
                  << std::endl;
      } else {
        read_config_from_file(path);
      }
    });

    if (vm.end() != vm.find("validator")) {
      roles_.flags.authority = 1;
    }

    if (vm.end() != vm.find("already-synchronized")) {
      if (roles_.flags.authority == 0) {
        logger_->error("Non authority node is run as already synced");
        std::cerr
            << "Non authority node can't be presented as already synced;\n"
               "Provide --validator option or remove --already-synchronized"
            << std::endl;
        return false;
      }
      is_already_synchronized_ = true;
    }

    if (vm.end() != vm.find("unix-slots")) is_unix_slots_strategy_ = true;

    find_argument<std::string>(
        vm, "chain", [&](const std::string &val) { chain_spec_path_ = val; });

    find_argument<std::string>(
        vm, "base-path", [&](const std::string &val) { base_path_ = val; });

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
          auto err_msg = "Bootnode '" + addr_str
                         + "' is invalid: " + ma_res.error().message();
          logger_->error(err_msg);
          std::cout << err_msg << std::endl;
          return false;
        }
        auto peer_id_base58_opt = ma_res.value().getPeerId();
        if (not peer_id_base58_opt) {
          auto err_msg = "Bootnode '" + addr_str + "' has not peer_id";
          logger_->error(err_msg);
          std::cout << err_msg << std::endl;
          return false;
        }
        boot_nodes_.emplace_back(std::move(ma_res.value()));
      }
    }

    boost::optional<std::string> node_key;
    find_argument<std::string>(
        vm, "node-key", [&](const std::string &val) { node_key.emplace(val); });
    if (node_key.has_value()) {
      auto key_res = crypto::Ed25519PrivateKey::fromHex(node_key.get());
      if (not key_res.has_value()) {
        auto err_msg = "Node key '" + node_key.get()
                       + "' is invalid: " + key_res.error().message();
        logger_->error(err_msg);
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

    find_argument<uint16_t>(vm, "port", [&](uint16_t val) { p2p_port_ = val; });

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
          logger_->error("Address {} passed as value to {} is invalid: {}",
                         s,
                         param_name,
                         ma_res.error().message());
          return false;
        }
        output_field.emplace_back(std::move(ma_res.value()));
      }
      return true;
    };

    if (not parse_multiaddrs("listen-addr", listen_addresses_)) {
      return false;  // just proxy erroneous case to the top level
    }
    if (not parse_multiaddrs("public-addr", public_addresses_)) {
      return false;  // just proxy erroneous case to the top level
    }

    // this goes before transforming p2p port option to listen addresses,
    // because it does not make sense to announce 0.0.0.0 as a public address.
    // Moreover, this is ok to have empty set of public addresses at this point
    // of time
    if (public_addresses_.empty() and not listen_addresses_.empty()) {
      logger_->info(
          "Public addresses are not specified. Using listen addresses as "
          "node's public addresses");
      public_addresses_ = listen_addresses_;
    }

    if (0 != p2p_port_ and listen_addresses_.empty()) {
      // IPv6
      {
        auto ma_res = libp2p::multi::Multiaddress::create(
            "/ip6/::/tcp/" + std::to_string(p2p_port_));
        if (not ma_res) {
          logger_->error(
              "Cannot construct IPv6 listen multiaddress from port {}. Error: "
              "{}",
              p2p_port_,
              ma_res.error().message());
        } else {
          logger_->info("Automatically added IPv6 listen address {}",
                        ma_res.value().getStringAddress());
          listen_addresses_.emplace_back(std::move(ma_res.value()));
        }
      }

      // IPv4
      {
        auto ma_res = libp2p::multi::Multiaddress::create(
            "/ip4/0.0.0.0/tcp/" + std::to_string(p2p_port_));
        if (not ma_res) {
          logger_->error(
              "Cannot construct IPv4 listen multiaddress from port {}. Error: "
              "{}",
              p2p_port_,
              ma_res.error().message());
        } else {
          logger_->info("Automatically added IPv4 listen address {}",
                        ma_res.value().getStringAddress());
          listen_addresses_.emplace_back(std::move(ma_res.value()));
        }
      }
    }

    find_argument<uint32_t>(vm, "max-blocks-in-response", [&](uint32_t val) {
      max_blocks_in_response_ = val;
    });

    find_argument<int32_t>(vm, "verbosity", [&](int32_t val) {
      auto level = static_cast<log::Level>(val + def_verbosity);
      if (level >= log::Level::OFF && level <= log::Level::TRACE)
        verbosity_ = level;
    });

    find_argument<std::string>(
        vm, "rpc-host", [&](std::string const &val) { rpc_http_host_ = val; });

    find_argument<std::string>(
        vm, "ws-host", [&](std::string const &val) { rpc_ws_host_ = val; });

    find_argument<std::string>(
        vm, "prometheus-host", [&](std::string const &val) { openmetrics_http_host_ = val; });

    find_argument<uint16_t>(
        vm, "rpc-port", [&](uint16_t val) { rpc_http_port_ = val; });

    find_argument<uint16_t>(
        vm, "ws-port", [&](uint16_t val) { rpc_ws_port_ = val; });

    find_argument<uint16_t>(
        vm, "prometheus-port", [&](uint16_t val) { openmetrics_http_port_ = val; });

    find_argument<uint32_t>(vm, "ws-max-connections", [&](uint32_t val) {
      max_ws_connections_ = val;
    });

    rpc_http_endpoint_ = get_endpoint_from(rpc_http_host_, rpc_http_port_);
    rpc_ws_endpoint_ = get_endpoint_from(rpc_ws_host_, rpc_ws_port_);
    openmetrics_http_endpoint_ = get_endpoint_from(openmetrics_http_host_, openmetrics_http_port_);

    find_argument<std::string>(
        vm, "name", [&](std::string const &val) { node_name_ = val; });

    // if something wrong with config print help message
    if (not validate_config()) {
      std::cout << desc << std::endl;
      return false;
    }
    return true;
  }
}  // namespace kagome::application
