/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_APP_CONFIG_IMPL_HPP
#define KAGOME_APP_CONFIG_IMPL_HPP

#include "application/app_config.hpp"

#define RAPIDJSON_NO_SIZETYPEDEFINE
namespace rapidjson {
  typedef ::std::size_t SizeType;
}
#include <rapidjson/document.h>
#undef RAPIDJSON_NO_SIZETYPEDEFINE

#include <cstdio>
#include <memory>

#include "common/logger.hpp"

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

  class AppConfigurationImpl final : public AppConfiguration {
    using FilePtr = std::unique_ptr<std::FILE, decltype(&std::fclose)>;

   private:
    void parse_general_segment(rapidjson::Value &val);
    void parse_blockchain_segment(rapidjson::Value &val);
    void parse_storage_segment(rapidjson::Value &val);
    void parse_authority_segment(rapidjson::Value &val);
    void parse_network_segment(rapidjson::Value &val);
    void parse_additional_segment(rapidjson::Value &val);

    /// TODO(iceseer): make handler calls via lambda-calls, remove
    /// member-function ptrs
    static constexpr struct {
      using Handler = void (kagome::application::AppConfigurationImpl::*)(
          rapidjson::Value &);
      char const *segment_name;
      Handler handler;
    } handlers[] = {
        {"general", &AppConfigurationImpl::parse_general_segment},
        {"blockchain", &AppConfigurationImpl::parse_blockchain_segment},
        {"storage", &AppConfigurationImpl::parse_storage_segment},
        {"authority", &AppConfigurationImpl::parse_authority_segment},
        {"network", &AppConfigurationImpl::parse_network_segment},
        {"additional", &AppConfigurationImpl::parse_additional_segment},
    };

   private:
    kagome::common::Logger logger_;
    std::string rpc_http_host_;
    std::string rpc_ws_host_;
    uint16_t rpc_http_port_;
    uint16_t rpc_ws_port_;

    // clang-format off
    /*
     *      COMMAND LINE ARGUMENTS          <- max priority
     *                V
     *        CONFIGURATION FILE
     *                V
     *          DEFAULT VALUES              <- low priority
     */
    // clang-format on

   private:
    void validate_config(AppConfiguration::LoadScheme scheme);
    void read_config_from_file(const std::string &filepath);

    bool load_str(const rapidjson::Value &val,
                  char const *name,
                  std::string &target);
    bool load_u16(const rapidjson::Value &val,
                  char const *name,
                  uint16_t &target);
    bool load_bool(const rapidjson::Value &val, char const *name, bool &target);

    boost::asio::ip::tcp::endpoint get_endpoint_from(const std::string &host,
                                                     uint16_t port);
    FilePtr open_file(const std::string &filepath);
    [[maybe_unused]] size_t get_file_size(const std::string &filepath);

   public:
    AppConfigurationImpl(kagome::common::Logger logger);
    AppConfigurationImpl(const AppConfigurationImpl &) = delete;
    AppConfigurationImpl &operator=(const AppConfigurationImpl &) = delete;

    bool initialize_from_args(AppConfiguration::LoadScheme scheme,
                              int argc,
                              char **argv);

    DECLARE_PROPERTY(std::string, genesis_path);
    DECLARE_PROPERTY(std::string, keystore_path);
    DECLARE_PROPERTY(std::string, leveldb_path);
    DECLARE_PROPERTY(uint16_t, p2p_port);
    DECLARE_PROPERTY(boost::asio::ip::tcp::endpoint, rpc_http_endpoint);
    DECLARE_PROPERTY(boost::asio::ip::tcp::endpoint, rpc_ws_endpoint);
    DECLARE_PROPERTY(spdlog::level::level_enum, verbosity);
    DECLARE_PROPERTY(bool, is_only_finalizing);
  };

}  // namespace kagome::application

#undef DECLARE_PROPERTY

#endif  // KAGOME_APP_CONFIG_IMPL_HPP