/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TCP_CONNECTION_UTIL_HPP
#define KAGOME_TCP_CONNECTION_UTIL_HPP

#include <sstream>
#include <system_error>  // for std::errc

#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>
#include <gsl/span>
#include <outcome/outcome.hpp>
#include "libp2p/multi/multiaddress.hpp"

namespace libp2p::transport::detail {
  template <typename T>
  inline outcome::result<multi::Multiaddress> make_endpoint(T &&endpoint) {
    auto address = endpoint.address();
    auto port = endpoint.port();

    // TODO(warchant): PRE-181 refactor to use builder instead
    std::ostringstream s;
    if (address.is_v4()) {
      s << "/ip4/" << address.to_v4().to_string();
    } else {
      s << "/ip6/" << address.to_v6().to_string();
    }

    s << "/tcp/" << port;

    return multi::Multiaddress::create(s.str());
  }

  inline auto make_buffer(gsl::span<uint8_t> s) {
    return boost::asio::buffer(s.data(), s.size());
  }

  inline auto make_buffer(gsl::span<const uint8_t> s) {
    return boost::asio::buffer(s.data(), s.size());
  }

  inline bool supports_ip_tcp(const multi::Multiaddress &ma) {
    using P = multi::Protocol::Code;
    return (ma.hasProtocol(P::IP4) || ma.hasProtocol(P::IP6))
        && ma.hasProtocol(P::TCP);
  }

  // @throws boost::bad_lexical_cast
  // @throws boost::system::system_error
  inline boost::asio::ip::tcp::endpoint make_endpoint(
      const multi::Multiaddress &ma) {
    using P = multi::Protocol::Code;
    using namespace boost::asio;    // NOLINT
    using namespace boost::system;  // NOLINT

    auto v = ma.getProtocolsWithValues();
    auto it = v.begin();
    if (!(it->first.code == P::IP4 || it->first.code == P::IP6)) {
      boost::throw_exception(
          system_error(make_error_code(errc::address_family_not_supported)));
    }

    auto addr = ip::make_address(it->second);
    ++it;

    if (it->first.code != P::TCP) {
      boost::throw_exception(
          system_error(make_error_code(errc::address_family_not_supported)));
    }

    auto port = boost::lexical_cast<uint16_t>(it->second);

    return ip::tcp::endpoint{addr, port};
  }
}  // namespace libp2p::transport::detail

#endif  // KAGOME_TCP_CONNECTION_UTIL_HPP
