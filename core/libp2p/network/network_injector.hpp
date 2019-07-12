/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_NETWORK_INJECTOR_HPP
#define KAGOME_NETWORK_INJECTOR_HPP

#include <boost/di.hpp>

// interfaces
#include "libp2p/muxer/muxer_adaptor.hpp"
#include "libp2p/network/network.hpp"
#include "libp2p/network/router.hpp"
#include "libp2p/peer/peer_repository.hpp"
#include "libp2p/protocol_muxer/protocol_muxer.hpp"
#include "libp2p/security/security_adaptor.hpp"
#include "libp2p/transport/transport_adaptor.hpp"
#include "libp2p/transport/upgrader.hpp"

namespace libp2p::network::injector {

  inline auto useKeyPair(crypto::KeyPair keyPair) {
    // clang-format off
    return boost::di::make_injector(
      boost::di::bind<crypto::KeyPair>.to(std::move(keyPair))[boost::di::override]
    );
    // clang-format on
  }

  template <typename C>
  auto useConfig(C c) {
    // TODO: decay Config type
    return boost::di::bind<C>.to(std::forward<C>(c));
  }

  template <typename... SecImpl>
  auto useSecurityAdaptors() {
    return boost::di::make_injector(
        boost::di::bind<security::SecurityAdaptor *[]>.to<SecImpl...>())
        [boost::di::override];
  }

  template <typename... MuxerImpl>
  auto useMuxerAdaptors() {
    return boost::di::make_injector(
        boost::di::bind<muxer::MuxerAdaptor *[]>.to<MuxerImpl...>())
        [boost::di::override];
  }

  template <typename... TransportImpl>
  auto useTransportAdaptors() {
    return boost::di::make_injector(
        boost::di::bind<transport::TransportAdaptor *[]>.to<TransportImpl...>())
        [boost::di::override];
  }

  boost::di::injector<std::shared_ptr<Network>> makeDefaultNetwork();

  template <typename... Ts>
  boost::di::injector<std::shared_ptr<Network>> makeNetworkInjector(
      Ts &&... args) {
    using namespace boost;  // NOLINT
    // clang-format off
    return di::make_injector(
        makeDefaultNetwork(),
        std::forward<decltype(args)>(args)...
    );
    // clang-format on
  }

}  // namespace libp2p::network::injector

#endif  // KAGOME_NETWORK_INJECTOR_HPP
