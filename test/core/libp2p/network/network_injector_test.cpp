/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/network/network_injector.hpp"

#include "libp2p/transport/tcp.hpp"
#include "libp2p/muxer/yamux.hpp"
#include "libp2p/security/plaintext.hpp"

#include <gtest/gtest.h>

using namespace libp2p;
using namespace network;
using namespace injector;
using namespace crypto;

/**
 * @when make default injector
 * @then test compiles
 */
TEST(NetworkBuilder, DefaultBuilds){
  // clang-format off
  auto injector = makeNetworkInjector(
    useTransportAdaptors<transport::TcpTransport>(),
    useMuxerAdaptors<muxer::Yamux>(),
    useSecurityAdaptors<security::Plaintext>()
  );
  // clang-format on
  auto nw = injector.create<std::shared_ptr<Network>>();
  ASSERT_TRUE(nw != nullptr);
}


TEST(NetworkBuilder, CustomKeyPairBuilds) {
  KeyPair keyPair{
      {{Key::Type::ED25519, {1}}},
      {{Key::Type::ED25519, {2}}},
  };

  auto injector = makeNetworkInjector(
      useTransportAdaptors<transport::TcpTransport>(),
      useMuxerAdaptors<muxer::Yamux>(),
      useSecurityAdaptors<security::Plaintext>(),
      useKeyPair(keyPair)
  );

  auto nw = injector.create<std::shared_ptr<Network>>();
  ASSERT_TRUE(nw != nullptr);
}
