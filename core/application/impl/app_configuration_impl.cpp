/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "application/impl/app_configuration_impl.hpp"

#include <rapidjson/document.h>
#include <rapidjson/filereadstream.h>
#include <boost/program_options.hpp>
#include <iostream>

namespace {
  namespace fs = boost::filesystem;

  template <typename T, typename Func>
  inline void find_argument(boost::program_options::variables_map &vm,
                            char const *name,
                            Func &&f) {
    assert(nullptr != name);
    auto it = vm.find(name);
    if (it != vm.end()) std::forward<Func>(f)(it->second.as<T>());
  }

  const std::string def_rpc_http_host = "0.0.0.0";
  const std::string def_rpc_ws_host = "0.0.0.0";
  const uint16_t def_rpc_http_port = 40363;
  const uint16_t def_rpc_ws_port = 40364;
  const uint16_t def_p2p_port = 30363;
  const int def_verbosity = 2;
  const bool def_is_only_finalizing = false;
  const bool def_is_already_synchronized = false;
  const bool def_is_unix_slots_strategy = false;
}  // namespace

namespace kagome::application {

  AppConfigurationImpl::AppConfigurationImpl(common::Logger logger)
      : p2p_port_(def_p2p_port),
        verbosity_(static_cast<spdlog::level::level_enum>(def_verbosity)),
        is_only_finalizing_(def_is_only_finalizing),
        is_already_synchronized_(def_is_already_synchronized),
        max_blocks_in_response_(kAbsolutMaxBlocksInResponse),
        is_unix_slots_strategy_(def_is_unix_slots_strategy),
        logger_(std::move(logger)),
        rpc_http_host_(def_rpc_http_host),
        rpc_ws_host_(def_rpc_ws_host),
        rpc_http_port_(def_rpc_http_port),
        rpc_ws_port_(def_rpc_ws_port) {}

  fs::path AppConfigurationImpl::genesis_path() const {
    return genesis_path_.native();
  }

  boost::filesystem::path AppConfigurationImpl::chain_path(
      std::string chain_id) const {
    return base_path_ / chain_id;
  }

  fs::path AppConfigurationImpl::database_path(std::string chain_id) const {
    return chain_path(chain_id) / "db";
  }

  fs::path AppConfigurationImpl::keystore_path(std::string chain_id) const {
    return chain_path(chain_id) / "keystore";
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
    uint16_t v{};
    if (load_u16(val, "verbosity", v) && v <= SPDLOG_LEVEL_OFF)
      verbosity_ = static_cast<spdlog::level::level_enum>(v);
  }

  void AppConfigurationImpl::parse_blockchain_segment(rapidjson::Value &val) {
    std::string genesis_path_str;
    load_str(val, "genesis", genesis_path_str);
    genesis_path_ = fs::path(genesis_path_str);
  }

  void AppConfigurationImpl::parse_storage_segment(rapidjson::Value &val) {
    std::string base_path_str;
    load_str(val, "base_path", base_path_str);
    base_path_ = fs::path(base_path_str);
  }

  void AppConfigurationImpl::parse_network_segment(rapidjson::Value &val) {
    load_ma(val, "bootnodes", bootnodes_);
    load_u16(val, "p2p_port", p2p_port_);
    load_str(val, "rpc_http_host", rpc_http_host_);
    load_u16(val, "rpc_http_port", rpc_http_port_);
    load_str(val, "rpc_ws_host", rpc_ws_host_);
    load_u16(val, "rpc_ws_port", rpc_ws_port_);
  }

  void AppConfigurationImpl::parse_additional_segment(rapidjson::Value &val) {
    load_bool(val, "single_finalizing_node", is_only_finalizing_);
    load_bool(val, "already_synchronized", is_already_synchronized_);
    load_u32(val, "max_blocks_in_response", max_blocks_in_response_);
    load_bool(val, "is_unix_slots_strategy", is_unix_slots_strategy_);
  }

  bool AppConfigurationImpl::validate_config(
      AppConfiguration::LoadScheme scheme) {
    if (not fs::exists(genesis_path_)) {
      logger_->error("Path to genesis {} does not exist.", genesis_path_);
      return false;
    }

    if (not fs::exists(base_path_)) {
      logger_->error("Base path {} does not exist.", base_path_);
      return false;
    }

    if (p2p_port_ == 0) {
      logger_->error("p2p port is 0.");
      return false;
    }

    if (rpc_ws_port_ == 0) {
      logger_->error("RPC ws port is 0.");
      return false;
    }

    if (rpc_http_port_ == 0) {
      logger_->error("RPC http port is 0.");
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
      logger_->error("Configuration file path is invalid: {}", filepath);
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
      logger_->error("Configuration file {} parse failed, with error {}",
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

  bool AppConfigurationImpl::initialize_from_args(
      AppConfiguration::LoadScheme scheme, int argc, char **argv) {
    namespace po = boost::program_options;

    // clang-format off
    po::options_description desc("General options");
    desc.add_options()
        ("help,h", "show this help message")
        ("verbosity,v", po::value<int>(), "Log level: 0 - trace, 1 - debug, 2 - info, 3 - warn, 4 - error, 5 - critical, 6 - no log")
        ("config_file,c", po::value<std::string>(), "Filepath to load configuration from.")
        ;

    po::options_description blockhain_desc("Blockchain options");
    blockhain_desc.add_options()
        ("genesis,g", po::value<std::string>(), "required, configuration file path")
        ;

    po::options_description storage_desc("Storage options");
    storage_desc.add_options()
        ("base_path,d", po::value<std::string>(),
            "required, node base path (keeps storage and keys for known chains)")
        ;

    po::options_description network_desc("Network options");
    network_desc.add_options()
        ("bootnodes", po::value<std::vector<std::string>>()->multitoken(), "multiaddresses of bootnodes")
        ("p2p_port,p", po::value<uint16_t>(), "port for peer to peer interactions")
        ("rpc_http_host", po::value<std::string>(), "address for RPC over HTTP")
        ("rpc_http_port", po::value<uint16_t>(), "port for RPC over HTTP")
        ("rpc_ws_host", po::value<std::string>(), "address for RPC over Websocket protocol")
        ("rpc_ws_port", po::value<uint16_t>(), "port for RPC over Websocket protocol")
        ;

    po::options_description additional_desc("Additional options");
    additional_desc.add_options()
        ("single_finalizing_node,f", "if this is the only finalizing node")
        ("already_synchronized,s", "if need to consider synchronized")
        ("max_blocks_in_response", "max block per response while syncing")
        ("unix_slots,u", "if slots are calculated from unix epoch")
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

    try {
      po::store(po::parse_command_line(argc, argv, desc), vm);
      po::store(parsed, vm);
      po::notify(vm);
    } catch (std::exception &e) {
      std::cerr << "Error: " << e.what() << '\n'
                << "Try run with option '--help' for more information"
                << std::endl;

      return false;
    }

    find_argument<std::string>(vm, "config_file", [&](std::string const &path) {
      read_config_from_file(path);
    });

    /// aggregate data from command line args
    if (vm.end() != vm.find("single_finalizing_node"))
      is_only_finalizing_ = true;

    if (vm.end() != vm.find("already_synchronized"))
      is_already_synchronized_ = true;

    if (vm.end() != vm.find("unix_slots"))
      is_unix_slots_strategy_ = true;

    find_argument<std::string>(
        vm, "genesis", [&](std::string const &val) { genesis_path_ = val; });

    find_argument<std::string>(
        vm, "base_path", [&](std::string const &val) { base_path_ = val; });

    find_argument<std::vector<std::string>>(
        vm, "bootnodes", [&](std::vector<std::string> const &val) {
          for (auto &s : val) {
            if (auto ma_res = libp2p::multi::Multiaddress::create(s)) {
              bootnodes_.emplace_back(std::move(ma_res.value()));
            }
          }
        });

    find_argument<uint16_t>(
        vm, "p2p_port", [&](uint16_t val) { p2p_port_ = val; });

    find_argument<uint32_t>(vm, "max_blocks_in_response", [&](uint32_t val) {
      max_blocks_in_response_ = val;
    });

    find_argument<int32_t>(vm, "verbosity", [&](int32_t val) {
      if (val >= SPDLOG_LEVEL_TRACE && val <= SPDLOG_LEVEL_OFF)
        verbosity_ = static_cast<spdlog::level::level_enum>(val);
    });

    find_argument<std::string>(
        vm, "rpc_http_host", [&](std::string const &val) {
          rpc_http_host_ = val;
        });

    find_argument<std::string>(
        vm, "rpc_ws_host", [&](std::string const &val) { rpc_ws_host_ = val; });

    find_argument<uint16_t>(
        vm, "rpc_http_port", [&](uint16_t val) { rpc_http_port_ = val; });

    find_argument<uint16_t>(
        vm, "rpc_ws_port", [&](uint16_t val) { rpc_ws_port_ = val; });

    rpc_http_endpoint_ = get_endpoint_from(rpc_http_host_, rpc_http_port_);
    rpc_ws_endpoint_ = get_endpoint_from(rpc_ws_host_, rpc_ws_port_);

    // if something wrong with config print help message
    if (not validate_config(scheme)) {
      std::cout << desc << std::endl;
      return false;
    }
    return true;
  }

}  // namespace kagome::application
