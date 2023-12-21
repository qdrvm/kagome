/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/asio/ssl/context.hpp>
#include <boost/asio/ssl/rfc2818_verification.hpp>

namespace kagome {
  // TODO(turuslan): move to qtils, reuse for libp2p "/wss"
  struct AsioSslContextClient : boost::asio::ssl::context {
    AsioSslContextClient(const std::string &host)
        : context{context::tlsv13_client} {
      set_options(context::default_workarounds | context::no_sslv2
                  | context::no_sslv3 | context::no_tlsv1 | context::no_tlsv1_1
                  | context::no_tlsv1_2 | context::single_dh_use);
      set_default_verify_paths();
      set_verify_mode(boost::asio::ssl::verify_peer);
      set_verify_callback(boost::asio::ssl::rfc2818_verification{host});
    }
  };
}  // namespace kagome
