/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "application/impl/app_config_impl.hpp"

#include <rapidjson/document.h>
#include <rapidjson/filereadstream.h>
#include <boost/program_options.hpp>
#include <iostream>

namespace {
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
}  // namespace

namespace kagome::application {

  AppConfigurationImpl::AppConfigurationImpl(kagome::common::Logger logger)
      : logger_(std::move(logger)),
        rpc_http_host_(def_rpc_http_host),
        rpc_ws_host_(def_rpc_ws_host),
        rpc_http_port_(def_rpc_http_port),
        rpc_ws_port_(def_rpc_ws_port),
        p2p_port_(def_p2p_port),
        verbosity_(static_cast<spdlog::level::level_enum>(def_verbosity)),
        is_only_finalizing_(def_is_only_finalizing) {}

  AppConfigurationImpl::FilePtr AppConfigurationImpl::open_file(
      const std::string &filepath) {
    assert(!filepath.empty());
    return AppConfigurationImpl::FilePtr(std::fopen(filepath.c_str(), "r"),
                                         &std::fclose);
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
    auto m = val.FindMember(name);
    if (val.MemberEnd() != m && m->value.IsInt()) {
      const auto v = m->value.GetInt();
      const auto in_range = (v & ~std::numeric_limits<uint16_t>::max()) == 0;

      if (in_range) {
        target = static_cast<uint16_t>(v);
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
    load_str(val, "genesis", genesis_path_);
  }

  void AppConfigurationImpl::parse_storage_segment(rapidjson::Value &val) {
    load_str(val, "leveldb", leveldb_path_);
  }

  void AppConfigurationImpl::parse_authority_segment(rapidjson::Value &val) {
    load_str(val, "keystore", keystore_path_);
  }

  void AppConfigurationImpl::parse_network_segment(rapidjson::Value &val) {
    load_u16(val, "p2p_port", p2p_port_);
    load_str(val, "rpc_http_host", rpc_http_host_);
    load_u16(val, "rpc_http_port", rpc_http_port_);
    load_str(val, "rpc_ws_host", rpc_ws_host_);
    load_u16(val, "rpc_ws_port", rpc_ws_port_);
  }

  void AppConfigurationImpl::parse_additional_segment(rapidjson::Value &val) {
    load_bool(val, "single_finalizing_node", is_only_finalizing_);
  }

  void AppConfigurationImpl::validate_config(
      AppConfiguration::LoadScheme scheme) {
    if (genesis_path_.empty()) {
      logger_->error("Node configuration must contain 'genesis' option.");
      exit(EXIT_FAILURE);
    }

    if (leveldb_path_.empty()) {
      logger_->error("Node configuration must contain 'leveldb_path' option.");
      exit(EXIT_FAILURE);
    }

    if (p2p_port_ == 0) {
      logger_->error("p2p port is 0.");
      exit(EXIT_FAILURE);
    }

    if (rpc_ws_port_ == 0) {
      logger_->error("RPC ws port is 0.");
      exit(EXIT_FAILURE);
    }

    if (rpc_http_port_ == 0) {
      logger_->error("RPC http port is 0.");
      exit(EXIT_FAILURE);
    }

    const auto need_keystore =
        (AppConfiguration::LoadScheme::kBlockProducing == scheme)
        || (AppConfiguration::LoadScheme::kValidating == scheme);

    if (need_keystore && keystore_path_.empty()) {
      logger_->error("Node configuration must contain 'keystore_path' option.");
      exit(EXIT_FAILURE);
    }
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
    FileReadStream input_stream(file.get(), buffer_size.data(), buffer_size.size());

    Document document;
    document.ParseStream(input_stream);
    if (document.HasParseError()) {
      logger_->error("Configuration file {} parse failed, with error {}",
                     filepath,
                     document.GetParseError());
      return;
    }

    for (auto &handler : handlers) {
      auto it = document.FindMember(handler.segment_name);
      if (document.MemberEnd() != it) {
        (this->*handler.handler)(it->value);
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
        ("verbosity,v", po::value<int>(), "Log level: 0 - trace, 1 - debug, 2 - info, 3 - warn, 4 - error, 5 - crit, 6 - no log")
        ("config_file,c", po::value<std::string>(), "Filepath to load configuration from.")
        ;

    po::options_description blockhain_desc("Blockchain options");
    blockhain_desc.add_options()
        ("genesis,g", po::value<std::string>(), "required, configuration file path")
        ;

    po::options_description storage_desc("Storage options");
    storage_desc.add_options()
        ("leveldb,l", po::value<std::string>(), "required, leveldb directory path")
        ;

    po::options_description authority_desc("Authority options");
    authority_desc.add_options()
        ("keystore,k", po::value<std::string>(), "required, keystore file path")
        ;

    po::options_description network_desc("Network options");
    network_desc.add_options()
        ("p2p_port,p", po::value<uint16_t>(), "port for peer to peer interactions")
        ("rpc_http_host", po::value<std::string>(), "address for RPC over HTTP")
        ("rpc_http_port", po::value<uint16_t>(), "port for RPC over HTTP")
        ("rpc_ws_host", po::value<std::string>(), "address for RPC over Websocket protocol")
        ("rpc_ws_port", po::value<uint16_t>(), "port for RPC over Websocket protocol")
        ;

    po::options_description additional_desc("Additional options");
    additional_desc.add_options()
        ("single_finalizing_node,f", po::value<bool>(), "if this is the only finalizing node")
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
        .add(authority_desc)
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

    find_argument<bool>(vm, "single_finalizing_node", [&](bool val) {
      is_only_finalizing_ = val;
    });

    find_argument<std::string>(
        vm, "genesis", [&](std::string const &val) { genesis_path_ = val; });

    find_argument<std::string>(
        vm, "leveldb", [&](std::string const &val) { leveldb_path_ = val; });

    find_argument<std::string>(
        vm, "keystore", [&](std::string const &val) { keystore_path_ = val; });

    find_argument<uint16_t>(
        vm, "p2p_port", [&](uint16_t val) { p2p_port_ = val; });

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
    validate_config(scheme);
    return true;
  }

}  // namespace kagome::application
