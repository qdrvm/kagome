/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/asio/ssl/context.hpp>
#include <boost/asio/ssl/rfc2818_verification.hpp>
#include <filesystem>

namespace kagome {
  // TODO(turuslan): move to qtils, reuse for libp2p "/wss"
  struct AsioSslContextClient : boost::asio::ssl::context {
    AsioSslContextClient(const std::string &host)
        : context{context::tlsv13_client} {
      // X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT
      // X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY
      [[maybe_unused]] static bool find_system_certificates = [] {
        // SSL_CERT_FILE
        if (getenv(X509_get_default_cert_file_env()) != nullptr) {
          return true;
        }
        // SSL_CERT_DIR
        if (getenv(X509_get_default_cert_dir_env()) != nullptr) {
          return true;
        }
        constexpr auto extra = "/etc/ssl/cert.pem";
        if (std::string_view{X509_get_default_cert_file()} != extra
            and std::filesystem::exists(extra)) {
          setenv(X509_get_default_cert_file_env(), extra, true);
          return true;
        }
        constexpr auto extra_dir = "/etc/ssl/certs";
        std::error_code ec;
        if (std::string_view{X509_get_default_cert_dir()} != extra_dir
            and std::filesystem::directory_iterator{extra_dir, ec}
                    != std::filesystem::directory_iterator{}) {
          setenv(X509_get_default_cert_dir_env(), extra_dir, true);
          return true;
        }
        return true;
      }();
      set_options(context::default_workarounds | context::no_sslv2
                  | context::no_sslv3 | context::no_tlsv1 | context::no_tlsv1_1
                  | context::no_tlsv1_2 | context::single_dh_use);
      set_default_verify_paths();
      set_verify_mode(boost::asio::ssl::verify_peer);
      set_verify_callback(boost::asio::ssl::rfc2818_verification{host});
    }
  };
}  // namespace kagome
